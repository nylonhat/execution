#ifndef SPLIT_H
#define SPLIT_H

#include <atomic>
#include "scheduler.hpp"
#include "concepts.hpp"
#include "inlined_receiver.hpp"
#include "variant_child.hpp"
#include "branch_child.hpp"
#include "split_node.hpp"

namespace ex::algorithms::split {
	
	struct SourceTag {};
	
	struct DependentTag {};


	template<IsReceiver SuffixReceiver, IsScheduler Scheduler, IsSender SourceSender, class MonadicFunction>
	struct OpState
		: InlinedReceiver<OpState<SuffixReceiver, Scheduler, SourceSender, MonadicFunction>, SuffixReceiver>
		, VariantChildOp<OpState<SuffixReceiver, Scheduler, SourceSender, MonadicFunction>, 0, typename Scheduler::sender_t, typename Scheduler::sender_t>
		, branch_all::BranchChildOp<OpState<SuffixReceiver, Scheduler, SourceSender, MonadicFunction>, SourceTag{}, SourceSender>
		, branch_all::BranchChildOp<OpState<SuffixReceiver, Scheduler, SourceSender, MonadicFunction>, DependentTag{}, std::invoke_result_t<MonadicFunction, SplitNodeSender<OpState<SuffixReceiver, Scheduler, SourceSender, MonadicFunction>, SourceSender>>>
	{	
		using OpStateOptIn = ex::OpStateOptIn;
		using Receiver = InlinedReceiver<OpState, SuffixReceiver>;
		using SourceSenderT = SourceSender;
		using DependentSender = std::invoke_result_t<MonadicFunction, SplitNodeSender<OpState, SourceSender>>;
		
		using SchedulerOp = VariantChildOp<OpState, 0, typename Scheduler::sender_t, typename Scheduler::sender_t>;
		using SourceOp = branch_all::BranchChildOp<OpState, SourceTag{}, SourceSender>;
		using DependentOp = branch_all::BranchChildOp<OpState, DependentTag{}, DependentSender>;
		
		[[no_unique_address]] Scheduler scheduler;
		[[no_unique_address]] MonadicFunction monadic_function;
		
		std::atomic<void*> head = nullptr;
		NodeOp* current_node_ptr = nullptr;
		std::atomic<std::int8_t> counter = 1;

		OpState(SuffixReceiver suffix_receiver, Scheduler scheduler, SourceSender source_sender, MonadicFunction monadic_function)
			: Receiver{suffix_receiver}
			, SchedulerOp{scheduler.sender()}
			, SourceOp{source_sender}
			, DependentOp{}
			, scheduler{scheduler}
			, monadic_function{monadic_function}
		{}

		template<class... Cont>
		auto start(Cont&... cont){
			auto& scheduler_op = SchedulerOp::template get<0>();
			auto& source_sender = SourceOp::get_sender();
			auto& source_op = SourceOp::construct_from(source_sender);
			ex::start(scheduler_op, source_op, cont...);
		}
		
		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont>
			requires (VariantIndex == 0)
        auto set_value(Cont&... cont){
			auto dependent_sender = monadic_function(SplitNodeSender<OpState, SourceSender>{this});
			auto& dependent_op = DependentOp::construct_from(dependent_sender);
			ex::start(dependent_op, cont...);
		}
		
		auto& get_scheduler_op(){
			return SchedulerOp::template construct_from<1>(scheduler.sender());
		}
		
		auto& get_source_result(){
			return SourceOp::get_result();
		}
		
		template<std::size_t ChildIndex, std::size_t VariantIndex>
			requires (VariantIndex == 1)
        auto set_value(){
			auto& node_op = *current_node_ptr;
			current_node_ptr = current_node_ptr->next_node_ptr;
			return ex::start(node_op);
		}
		
		
		template<SourceTag tag, class... Cont, class... Args>
		auto set_value(Cont&... cont, Args... args){
			//Set done and wake first type erased waiter
			SourceOp::construct_result(args...);
			
			auto old_counter = counter.fetch_sub(1);
			
			if(old_counter == 0){
				//Last to finish; no waiter guaranteed
				return ex::set_value<Cont...>(this->get_receiver(), cont..., DependentOp::get_result());
			}
			
			auto* old_head = head.exchange(this);
			
			if(old_head == nullptr){
				//No waiters
				return ex::start(cont...);
			}
			
			//There are waiters to wakeup
			current_node_ptr = static_cast<NodeOp*>(old_head);
			auto& node_op = *current_node_ptr;
			current_node_ptr = current_node_ptr->next_node_ptr;
			return ex::start(cont..., node_op);
			
		}
		
		template<DependentTag tag, class... Cont, class... Args>
		auto set_value(Cont&... cont, Args... args){
			DependentOp::construct_result(args...);
			
			auto old_counter = counter.fetch_sub(1);
			
			if(old_counter == 0){
				return ex::set_value<Cont...>(this->get_receiver(), cont..., DependentOp::get_result());
			}
			
			return ex::start(cont...);
		}


	};

	
	template<IsScheduler Scheduler, IsSender SourceSender, class MonadicFunction>
	struct Sender {
		using SenderOptIn = ex::SenderOptIn;
		using value_t = std::invoke_result_t<MonadicFunction, SourceSender>::value_t;
		using error_t = std::invoke_result_t<MonadicFunction, SourceSender>::error_t;

		[[no_unique_address]] Scheduler scheduler;
		[[no_unique_address]] SourceSender source_sender;
		[[no_unique_address]] MonadicFunction monadic_function;

		template<IsReceiver SuffixReceiver>
		auto connect(SuffixReceiver suffix_receiver){
			return OpState{suffix_receiver, scheduler, source_sender, monadic_function};
		}
	};

	struct FunctionObject {

		static auto operator()(IsScheduler auto scheduler, IsSender auto source_sender, auto monadic_function){
			return Sender{scheduler, source_sender, monadic_function};
		}
		
	};

	
}//namespace ex::algorithms::split

namespace ex {

		inline constexpr auto split = algorithms::split::FunctionObject{};

}//namespace ex



#endif//SPLIT_H
