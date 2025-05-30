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
			[[gnu::musttail]] return ex::start(scheduler_op, source_op, cont...);
		}
		
		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont>
			requires (VariantIndex == 0)
        auto set_value(Cont&... cont){
			auto dependent_sender = monadic_function(SplitNodeSender<OpState, SourceSender>{this});
			auto& dependent_op = DependentOp::construct_from(dependent_sender);
			[[gnu::musttail]] return ex::start(dependent_op, cont...);
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
			[[gnu::musttail]] return ex::start(node_op);
		}
		
		template<SourceTag tag, class... Cont, class... Args>
			requires first_same_as<typename SchedulerOp::template VariantOp<0>::Loopback, Cont...> 
		auto set_value(Cont&... cont, Args... args){
			//Scheduled syncronously
			SourceOp::construct_result(args...);
			counter.fetch_sub(1);
			head.store(this);
			[[gnu::musttail]] return ex::start(cont...);
		}
		
		
		template<SourceTag tag, class... Cont, class... Args>
		auto set_value(Cont&... cont, Args... args){
			//Set done and wake first type erased waiter
			SourceOp::construct_result(args...);
			auto* old_head = head.exchange(this);
				
			auto old_counter = counter.fetch_sub(1);
			//Danger line!! Operation state may no longer exist below here!!
				
			if(old_counter == 0){
				//Last to finish; no waiter guaranteed
				[[gnu::musttail]] return ex::set_value<Cont...>(this->get_receiver(), cont..., DependentOp::get_result());
			}
			
			//Caution!! Operation state may no longer exist after this point.
			//Do not access anything not local to function if there are no waiters!!
			
			if(old_head == nullptr){
				//No waiters
				[[gnu::musttail]] return ex::start(cont...);
			}
			
			//There are waiters to wakeup
			//Safe to access operation state members
			current_node_ptr = static_cast<NodeOp*>(old_head);
			auto& node_op = *current_node_ptr;
			current_node_ptr = current_node_ptr->next_node_ptr;
			[[gnu::musttail]] return ex::start(cont..., node_op);
			
		}
		
		template<DependentTag tag, class... Cont, class... Args>
			requires first_same_as<typename SchedulerOp::template VariantOp<1>::Loopback, Cont...> 
		auto set_value(Cont&... cont, Args... args){
			DependentOp::construct_result(args...);
			counter.fetch_sub(1);
			[[gnu::musttail]] return ex::start(cont...);
		}
		
		
		template<DependentTag tag, class... Cont, class... Args>
		auto set_value(Cont&... cont, Args... args){
			DependentOp::construct_result(args...);
			
			auto old_counter = counter.fetch_sub(1);
			
			if(old_counter == 0){
				[[gnu::musttail]] return ex::set_value<Cont...>(this->get_receiver(), cont..., DependentOp::get_result());
			}
			
			[[gnu::musttail]] return ex::start(cont...);
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
		
		static auto operator()(IsScheduler auto scheduler, auto monadic_function){
			return [=](IsSender auto source_sender){
				return Sender{scheduler, source_sender, monadic_function};
			};
		}
		
		
	};

	
}//namespace ex::algorithms::split

namespace ex {

		inline constexpr auto split = algorithms::split::FunctionObject{};

}//namespace ex



#endif//SPLIT_H
