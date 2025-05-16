#ifndef FOLD_H
#define FOLD_H

#include <atomic>
#include <array>
#include <ranges>
#include "scheduler.hpp"
#include "fold_child.hpp"
#include "concepts.hpp"
#include "inlined_receiver.hpp"
#include "variant_child.hpp"

namespace ex::algorithms::fold {
	
	struct ChildTag{};

	template<std::size_t Size, IsReceiver SuffixReceiver, IsScheduler Scheduler, std::ranges::range SenderRange, class MonadicFunction, class Init, class Function>
		requires (Size > 1)
	struct OpState
		: InlinedReceiver<OpState<Size, SuffixReceiver, Scheduler, SenderRange, MonadicFunction, Init, Function>, SuffixReceiver>
		, VariantChildOp<OpState<Size, SuffixReceiver, Scheduler, SenderRange, MonadicFunction, Init, Function>, 0, typename Scheduler::sender_t, std::ranges::range_value_t<SenderRange>>
		, FoldChildOp<Size, OpState<Size, SuffixReceiver, Scheduler, SenderRange, MonadicFunction, Init, Function>, ChildTag{}, ex::apply_values_t<MonadicFunction, std::ranges::range_value_t<SenderRange>>>
	{
		
		using OpStateOptIn = ex::OpStateOptIn;
		using Receiver = InlinedReceiver<OpState, SuffixReceiver>;
		using SchedulerSender = Scheduler::sender_t;
		using RangeSender = std::ranges::range_value_t<SenderRange>;
		using LoopOp = VariantChildOp<OpState, 0, SchedulerSender, RangeSender>;
		using ChildSender = ex::apply_values_t<MonadicFunction, RangeSender>;
		using ChildOps = FoldChildOp<Size, OpState, ChildTag{}, ChildSender>;
		
		Scheduler scheduler;
		SenderRange sender_range;
		MonadicFunction monadic_function;
		Init folded_value;
		Function function;
		
		std::ranges::iterator_t<SenderRange> iter;
		std::ranges::sentinel_t<SenderRange> sentinel;
		
		std::size_t head = 0;
		std::atomic<std::size_t> tail = 0;
		
		struct TicketCell {
			std::size_t ticket;
			std::atomic<std::size_t> tag;
		};
		
		std::array<TicketCell, Size> ticket_ring;
		
		OpState(SuffixReceiver suffix_receiver, Scheduler scheduler, SenderRange sender_range, MonadicFunction monadic_function, Init init, Function function)
			: Receiver{suffix_receiver}
			, LoopOp{scheduler.sender()}
			, ChildOps{}
			, scheduler{scheduler}
			, sender_range{sender_range}
			, monadic_function{monadic_function}
			, folded_value{init}
			, function{function}
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
				return ex::set_value<Cont...>(this->get_receiver(), cont..., folded_value);
			}
			
			return ex::start(LoopOp::template get<0>(), cont...);
			// static_assert(sizeof...(cont) == 0);
			// return set_value<0, 0, Cont...>(cont...);
		}
		
		//Scheduler Callback
		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont>
			requires same_index<ChildIndex, 0> && same_index<VariantIndex, 0>
		void set_value(Cont&... cont){
			//Make local copy of head
			auto head_now = head;
			
			if(iter == sentinel){
				//Signal that were finished
				auto old_tag = ticket_ring.at((head_now-1) % Size).tag.fetch_add(2);
				
				if (old_tag == head_now){
					return ex::start(cont...);
				}

				if constexpr(!first_same_as<typename ChildOps::ChildOp, Cont...>){
					return finish(cont...);
				}
			}
			
			//Acquire a slot
			auto old_tag = ticket_ring.at(head_now % Size).tag.fetch_add(1);

			if(old_tag == head_now){
				return produce(cont...);
			}
			
			//Wait
			return ex::start(cont...);
		}
		
		template<class... Cont>
		void produce(Cont&... cont){
			auto& range_op = LoopOp::template construct_from<1>(*(iter++));
			return ex::start(range_op, cont...);
		}
		
		
		//Range sender callback
		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Args>
			requires same_index<ChildIndex, 0> && same_index<VariantIndex, 1>
		void set_value(Cont&... cont, Args... args){
			auto old_head = head++;
			
			auto ticket = ticket_ring.at(old_head % Size).ticket;
			
			if(old_head >= Size){
				auto value = ChildOps::get_result_at(ticket);
				folded_value = function(value, folded_value);
			}
			
			auto& child_op = ChildOps::construct_from_sender_at(monadic_function(args...), ticket);
			auto& scheduler_op = LoopOp::template construct_from<0>(scheduler.sender());
			return ex::start(scheduler_op, child_op, cont...);
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
					return produce(cont...);
				}
			
			
				if(old_tag == old_tail + 3){
					return finish(cont...);
				}

			}
			
			return ex::start(cont...);
        }
		
		template<class... Cont>
		void finish(Cont&... cont){
			
			static_assert(sizeof...(cont) == 0);
			for(std::size_t i = 0; i < std::min(tail.load(), Size); ++i){
				auto value = ChildOps::get_result_at(i);
				folded_value = function(value, folded_value);
			}
			
			return ex::set_value<Cont...>(this->get_receiver(), cont..., folded_value);
		}
		
	};

	
	template<std::size_t Size, IsScheduler Scheduler, std::ranges::range SenderRange, class MonadicFunction, class Init, class Function>
		requires (Size > 1)	
	struct Sender {
		using SenderOptIn = ex::SenderOptIn;
		using value_t = std::tuple<Init>;
		using error_t = std::ranges::range_value_t<SenderRange>::error_t;

		Scheduler scheduler;
		SenderRange sender_range;
		MonadicFunction monadic_function;
		Init init;
		Function function;

		Sender(const Sender&) = default;

		Sender(Scheduler scheduler, SenderRange sender_range, MonadicFunction monadic_function, Init init, Function function)
			: scheduler{scheduler}
			, sender_range{sender_range}
			, monadic_function{monadic_function}
			, init{init}
			, function{function}
		{}

		template<IsReceiver SuffixReceiver>
		auto connect(SuffixReceiver suffix_receiver){
			return OpState<Size, SuffixReceiver, Scheduler, SenderRange, MonadicFunction, Init, Function>{suffix_receiver, scheduler, sender_range, monadic_function, init, function};
		}
	};

	template<std::size_t Size>
		requires (Size > 1)
	struct FunctionObject {
		
		template<IsScheduler Scheduler, std::ranges::range SenderRange, class MonadicFunction, class Init, class Function>
		auto operator()(this auto&&, Scheduler& scheduler, SenderRange sender_range, MonadicFunction monadic_function, Init init, Function function){
			auto handle = SchedulerHandle{scheduler};
			return Sender<Size, SchedulerHandle<Scheduler>, SenderRange, MonadicFunction, Init, Function>{handle, sender_range, monadic_function, init, function};	
		}
		
	};

	
}//namespace ex::algorithms::fold

namespace ex {
		
		template<std::size_t Size>
			requires (Size > 1)
		inline constexpr auto fold_on = algorithms::fold::FunctionObject<Size>{};

}//namespace ex



#endif//FOLD_H
