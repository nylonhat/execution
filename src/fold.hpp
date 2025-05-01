#ifndef FOLD_H
#define FOLD_H

#include <atomic>
#include <array>
#include <ranges>
#include "scheduler.hpp"
#include "fold_child.hpp"
#include "concepts.hpp"
#include "inline.hpp"

namespace ex::algorithms::fold {
	template<size_t A, size_t B>
	concept same_index = (A == B);

	template<size_t A, size_t B>
	concept not_same_index = (A != B);

	template<class... Pack>
	concept not_empty_pack = sizeof...(Pack) != 0;

	template<class T, class... Pack>
	concept first_same_as = std::same_as<Pack...[0], T>;
	
	template<class A, class B, class... Cont>
	concept first_not_finished = first_same_as<typename A::template VariantOp<0>, Cont...> 
		|| first_same_as<typename B::ChildOp, Cont...>;
	
	template<class A, class B, class... Cont>
	concept not_finished = not_empty_pack<Cont...> 
		&& first_not_finished<A, B, Cont...>;


	template<std::size_t Size, IsReceiver SuffixReceiver, IsScheduler Scheduler, std::ranges::range SenderRange, class Init, class Function>
	struct OpState
		: InlinedReceiver<OpState<Size, SuffixReceiver, Scheduler, SenderRange, Init, Function>, SuffixReceiver>
		, ManualChildOp<OpState<Size, SuffixReceiver, Scheduler, SenderRange, Init, Function>, 0, typename Scheduler::sender_t>
		, ManualChildOp<OpState<Size, SuffixReceiver, Scheduler, SenderRange, Init, Function>, 1, std::ranges::range_value_t<SenderRange>>
		, FoldChildOp<Size, OpState<Size, SuffixReceiver, Scheduler, SenderRange, Init, Function>, 2, ex::single_value_t<std::ranges::range_value_t<SenderRange>>>
	{
		
		using OpStateOptIn = ex::OpStateOptIn;
		using Receiver = InlinedReceiver<OpState, SuffixReceiver>;
		using SchedulerSender = Scheduler::sender_t;
		using SchedulerOp = ManualChildOp<OpState, 0, SchedulerSender>;
		using LoopSender = std::ranges::range_value_t<SenderRange>;
		using LoopOp = ManualChildOp<OpState, 1, LoopSender>;
		using ChildSender = ex::single_value_t<LoopSender>;
		using ArrayOp = FoldChildOp<Size, OpState, 2, ChildSender>;
		
		Scheduler scheduler;
		SenderRange sender_range;
		Init folded_value;
		Function function;
		
		std::ranges::iterator_t<SenderRange> iter;
		std::ranges::sentinel_t<SenderRange> sentinel;
		
		std::atomic<std::int64_t> semaphore = Size;
		std::size_t head = 0;
		std::atomic<std::size_t> tail = 0;
		
		struct TicketCell {
			std::size_t ticket;
			std::atomic<bool> pending = false;
		};
		
		std::array<TicketCell, Size> ticket_ring;
		
		OpState(SuffixReceiver suffix_receiver, Scheduler scheduler, SenderRange sender_range, Init init, Function function)
			: Receiver{suffix_receiver}
			, SchedulerOp{scheduler.sender()}
			, scheduler{scheduler}
			, sender_range{sender_range}
			, folded_value{init}
			, function{function}
			, iter{sender_range.begin()}
			, sentinel{sender_range.end()}
		{
			std::ranges::iota(std::views::transform(ticket_ring, &TicketCell::ticket), 0); 	
		}

		template<class... Cont>
		void start(Cont&... cont){
			if(iter == sentinel){
				return ex::set_value<Cont...>(this->get_receiver(), cont..., folded_value);
			}
			
			return ex::start(SchedulerOp::template get<0>());
		}
		
		//Scheduler Callback
		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont>
			requires same_index<ChildIndex, 0>
		void set_value(Cont&... cont){
			
			if(iter == sentinel){
				auto old = semaphore.fetch_add(1);
				if (old == Size){
					return finish(cont...);
				}
				
				return ex::start(cont...);
			}
			
			auto old = semaphore.fetch_sub(1);
			
			if(old == 0){
				return ex::start(cont...);
			}
	
			return produce(cont...);
		}
		
		template<class... Cont>
		void produce(Cont&... cont){
			auto& loop_op = LoopOp::template construct_from<0>(*(iter++));
			return ex::start(loop_op, cont...);
		}
		
		
		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont>
			requires same_index<ChildIndex, 1>
		void set_value(Cont&... cont, ChildSender child_sender){
			auto old_head = head++;
			
			while(ticket_ring.at(old_head % Size).pending.load()){
				//Short spin that is guaranteed to proceed
			}
			
			auto& ticket_ring_cell = ticket_ring.at(old_head % Size);
			auto ticket = ticket_ring_cell.ticket; 
			ticket_ring_cell.pending.store(true); 
			
			if(old_head >= Size){
				auto value = ArrayOp::get_result_at(ticket);
				folded_value = function(value, folded_value);
			}
			
			auto& child_op = ArrayOp::construct_from_at(child_sender, ticket);
			auto& scheduler_op = SchedulerOp::template construct_from<0>(scheduler.sender());
			return ex::start(cont..., scheduler_op, child_op);
		}
		

		//Child Result Callback
		template<std::size_t ChildIndex, class... Cont, class Arg>
			requires same_index<ChildIndex, 2>
		void set_value(std::size_t ticket, Cont&... cont, Arg arg){
			ArrayOp::construct_result_at(arg, ticket);
			
			auto old_tail = tail.fetch_add(1);
			
			auto& ticket_ring_cell = ticket_ring.at(old_tail % Size);
			ticket_ring_cell.ticket = ticket;
			ticket_ring_cell.pending.store(false);
			
			auto old_sema = semaphore.fetch_add(1);
			
			if constexpr(!not_finished<SchedulerOp, ArrayOp, Cont...>){
				//Wake up producer
				if(old_sema == -1){
					return produce(cont...);
				}
			}
			
			if(old_sema == Size){
				return finish(cont...);
			}
			
			return ex::start(cont...);
        }
		
		template<class... Cont>
		void finish(Cont&... cont){
			for(std::size_t i = 0; i < std::min(tail.load(), Size); ++i){
				auto value = ArrayOp::get_result_at(i);
				folded_value = function(value, folded_value);
			}
			
			return ex::set_value<Cont...>(this->get_receiver(), cont..., folded_value);
		}
		
	};

	
	template<std::size_t Size, IsScheduler Scheduler, std::ranges::range SenderRange, class Init, class Function>	
	struct Sender {
		using SenderOptIn = ex::SenderOptIn;
		using value_t = std::tuple<Init>;
		using error_t = std::ranges::range_value_t<SenderRange>::error_t;

		Scheduler scheduler;
		SenderRange sender_range;
		Init init;
		Function function;

		Sender(const Sender&) = default;

		Sender(Scheduler scheduler, SenderRange sender_range, Init init, Function function)
			: scheduler{scheduler}
			, sender_range{sender_range}
			, init{init}
			, function{function}
		{}

		template<IsReceiver SuffixReceiver>
		auto connect(SuffixReceiver suffix_receiver){
			return OpState<Size, SuffixReceiver, Scheduler, SenderRange, Init, Function>{suffix_receiver, scheduler, sender_range, init, function};
		}
	};

	template<std::size_t Size>
	struct FunctionObject {
		
		template<IsScheduler Scheduler, std::ranges::range SenderRange, class Init, class Function>
		auto operator()(this auto&&, Scheduler& scheduler, SenderRange sender_range, Init init, Function function){
			auto handle = SchedulerHandle{scheduler};
			return Sender<Size, SchedulerHandle<Scheduler>, SenderRange, Init, Function>{handle, sender_range, init, function};	
		}
		
	};

	
}//namespace ex::algorithms::fold

namespace ex {
		
		template<std::size_t Size>
		inline constexpr auto fold_on = algorithms::fold::FunctionObject<Size>{};

}//namespace ex



#endif//FOLD_H
