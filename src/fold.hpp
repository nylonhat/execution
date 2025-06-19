#ifndef FOLD_H
#define FOLD_H

#include <atomic>
#include <array>
#include <ranges>
#include "scheduler.hpp"
#include "child_array.hpp"
#include "concepts.hpp"
#include "inlined_receiver.hpp"
#include "child_variant.hpp"

namespace ex {
inline namespace fold_algorithm {
	
	struct ChildTag{};

	template<std::size_t Size, IsReceiver NextRx, IsScheduler Sched, std::ranges::range SndrRng, class SndrFn, class Init, class FoldFn>
		requires (Size > 1)
	struct Op
		: InlinedReceiver<Op<Size, NextRx, Sched, SndrRng, SndrFn, Init, FoldFn>, NextRx>
		, ChildVariant<Op<Size, NextRx, Sched, SndrRng, SndrFn, Init, FoldFn>, 0, typename Sched::sender_t, std::ranges::range_value_t<SndrRng>>
		, ChildArray<Size, Op<Size, NextRx, Sched, SndrRng, SndrFn, Init, FoldFn>, ChildTag{}, sender_of_apply_signature_t<SndrFn, channel_sig_t<Channel::value, std::ranges::range_value_t<SndrRng>>>>
		
	{
		
		using OpStateOptIn = ex::OpStateOptIn;
		using Receiver = InlinedReceiver<Op, NextRx>;
		using SchedulerSender = Sched::sender_t;
		using RangeSender = std::ranges::range_value_t<SndrRng>;
		using LoopOp = ChildVariant<Op, 0, SchedulerSender, RangeSender>;
		using ChildSender = sender_of_apply_signature_t<SndrFn, channel_sig_t<Channel::value, RangeSender>>;
		using ChildOps = ChildArray<Size, Op, ChildTag{}, ChildSender>;
		
		Sched scheduler;
		SndrRng sender_range;
		SndrFn sender_fn;
		Init folded_value;
		FoldFn fold_fn;
		
		std::ranges::iterator_t<SndrRng> iter;
		std::ranges::sentinel_t<SndrRng> sentinel;
		
		std::size_t head = 0;
		std::atomic<std::size_t> tail = 0;
		
		struct alignas(64) TicketCell {
			std::size_t ticket;
			std::atomic<std::size_t> tag;
		};
		
		std::array<TicketCell, Size> ticket_ring;
		
		Op(NextRx next_receiver, Sched scheduler, SndrRng sender_range, SndrFn sender_fn, Init init, FoldFn fold_fn)
			: Receiver{next_receiver}
			, LoopOp{scheduler.sender()}
			, ChildOps{}
			, scheduler{scheduler}
			, sender_range{sender_range}
			, sender_fn{sender_fn}
			, folded_value{init}
			, fold_fn{fold_fn}
			, iter{sender_range.begin()}
			, sentinel{sender_range.end()}
		{
			
			for(auto&& [index, cell] : std::views::enumerate(ticket_ring)){
				cell.ticket = index;
				cell.tag.store(index);
			}
		}

		template<class... Cont>
		void start(Cont&... cont){
			if(iter == sentinel){
				[[gnu::musttail]] return ex::set_value<Cont...>(this->get_receiver(), cont..., folded_value);
			}
			
			[[gnu::musttail]] return ex::start(LoopOp::template get<0>(), cont...);
			// static_assert(sizeof...(cont) == 0);
			// return set_value<0, 0, Cont...>(cont...);
		}
		
		//Sched Callback
		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont>
			requires same_index<ChildIndex, 0> && same_index<VariantIndex, 0>
		void set_value(Cont&... cont){
			//Make local copy of head
			auto head_now = head;
			
			if(iter == sentinel){
				//Signal that were finished
				auto old_tag = ticket_ring.at((head_now-1) % Size).tag.fetch_add(2);
				
				if (old_tag == head_now){
					[[gnu::musttail]] return ex::start(cont...);
				}

				if constexpr(!first_same_as<typename ChildOps::ChildOp, Cont...>){
					[[gnu::musttail]] return finish(cont...);
				}
			}
			
			//Acquire a slot
			auto old_tag = ticket_ring.at(head_now % Size).tag.fetch_add(1);

			if(old_tag == head_now){
				[[gnu::musttail]] return produce(cont...);
			}
			
			//Wait
			[[gnu::musttail]] return ex::start(cont...);
		}
		
		template<class... Cont>
		void produce(Cont&... cont){
			auto& range_op = LoopOp::template construct_from<1>(*(iter++));
			[[gnu::musttail]] return ex::start(range_op, cont...);
		}
		
		
		//Range sender callback
		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Args>
			requires same_index<ChildIndex, 0> && same_index<VariantIndex, 1>
		void set_value(Cont&... cont, Args... args){
			auto old_head = head++;
			
			auto ticket = ticket_ring.at(old_head % Size).ticket;
			
			if(old_head >= Size){
				auto value = ChildOps::get_result_at(ticket);
				folded_value = fold_fn(value, folded_value);
			}
			
			auto& child_op = ChildOps::construct_from_sender_at(sender_fn(args...), ticket);
			auto& scheduler_op = LoopOp::template construct_from<0>(scheduler.sender());
			[[gnu::musttail]] return ex::start(scheduler_op, child_op, cont...);
		}
		

		//Child Result Callback
		template<ChildTag Tag, class... Cont, class Arg>
		void set_value(std::size_t ticket, Cont&... cont, Arg arg){
			ChildOps::construct_result_at(arg, ticket);
			
			auto old_tail = tail.fetch_add(1);
			
			ticket_ring.at(old_tail % Size).ticket = ticket;
			
			//Release the slot
			auto old_tag = ticket_ring.at(old_tail % Size).tag.fetch_add(Size-1);
			
			if constexpr((!first_same_as<typename LoopOp::template VariantOp<0>::Loopback, Cont...>) && (!first_same_as<typename ChildOps::ChildOp, Cont...>)){
				//Wake up producer
				if(old_tag == old_tail + 2){
					[[gnu::musttail]] return produce(cont...);
				}
			
			
				if(old_tag == old_tail + 3){
					[[gnu::musttail]] return finish(cont...);
				}

			}
			
			[[gnu::musttail]] return ex::start(cont...);
        }
		
		template<class... Cont>
		void finish(Cont&... cont){
			
			static_assert(sizeof...(cont) == 0);
			for(std::size_t i = 0; i < std::min(tail.load(), Size); ++i){
				auto value = ChildOps::get_result_at(i);
				folded_value = fold_fn(value, folded_value);
			}
			
			[[gnu::musttail]] return ex::set_value<Cont...>(this->get_receiver(), cont..., folded_value);
		}
		
	};

	
	template<std::size_t Size, IsScheduler Sched, std::ranges::range SndrRng, class SndrFn, class Init, class FoldFn>
		requires (Size > 1)	
	struct Sender {
		using SenderOptIn = ex::SenderOptIn;
		using value_t = std::tuple<std::tuple<Init>>;
		using error_t = std::ranges::range_value_t<SndrRng>::error_t;

		Sched scheduler;
		SndrRng sender_range;
		SndrFn sender_fn;
		Init init;
		FoldFn fold_fn;

		Sender(const Sender&) = default;

		Sender(Sched scheduler, SndrRng sender_range, SndrFn sender_fn, Init init, FoldFn fold_fn)
			: scheduler{scheduler}
			, sender_range{sender_range}
			, sender_fn{sender_fn}
			, init{init}
			, fold_fn{fold_fn}
		{}

		template<IsReceiver NextRx>
		auto connect(NextRx next_receiver){
			return Op<Size, NextRx, Sched, SndrRng, SndrFn, Init, FoldFn>{next_receiver, scheduler, sender_range, sender_fn, init, fold_fn};
		}
	};

	template<std::size_t Size>
		requires (Size > 1)
	struct FnObj {
		
		template<IsScheduler Sched, std::ranges::range SndrRng, class SndrFn, class Init, class FoldFn>
		static auto operator()(Sched scheduler, SndrRng sender_range, SndrFn sender_fn, Init init, FoldFn fold_fn){
			return Sender<Size, Sched, SndrRng, SndrFn, Init, FoldFn>{scheduler, sender_range, sender_fn, init, fold_fn};	
		}
		
	};

	
}}//namespace ex::fold_algorithm

namespace ex {
		
		template<std::size_t Size>
			requires (Size > 1)
		inline constexpr auto fold_on = fold_algorithm::FnObj<Size>{};

}//namespace ex



#endif//FOLD_H
