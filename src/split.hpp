#ifndef SPLIT_H
#define SPLIT_H

#include <atomic>
#include "scheduler.hpp"
#include "concepts.hpp"
#include "inlined_receiver.hpp"
#include "child_variant.hpp"
#include "child_storage.hpp"
#include "split_node.hpp"

namespace ex {
inline namespace split_algorithm {
	
	struct SourceTag {};
	
	struct DependentTag {};


	template<IsReceiver NextRx, IsScheduler Sched, IsSender SrcSndr, class SndrFn>
	struct Op
		: InlinedReceiver<Op<NextRx, Sched, SrcSndr, SndrFn>, NextRx>
		, ChildVariant<Op<NextRx, Sched, SrcSndr, SndrFn>, 0, typename Sched::sender_t, typename Sched::sender_t>
		, ChildStorage<Op<NextRx, Sched, SrcSndr, SndrFn>, SourceTag{}, SrcSndr>
		, ChildStorage<Op<NextRx, Sched, SrcSndr, SndrFn>, DependentTag{}, std::invoke_result_t<SndrFn, SplitNodeSender<Op<NextRx, Sched, SrcSndr, SndrFn>, SrcSndr>>>
	{	
		using OpStateOptIn = ex::OpStateOptIn;
		using Receiver = InlinedReceiver<Op, NextRx>;
		using DependentSender = std::invoke_result_t<SndrFn, SplitNodeSender<Op, SrcSndr>>;
		
		using SchedulerOp = ChildVariant<Op, 0, typename Sched::sender_t, typename Sched::sender_t>;
		using SourceOp = ChildStorage<Op, SourceTag{}, SrcSndr>;
		using DependentOp = ChildStorage<Op, DependentTag{}, DependentSender>;
		
		[[no_unique_address]] Sched scheduler;
		[[no_unique_address]] SndrFn sender_fn;
		
		std::atomic<void*> head = nullptr;
		NodeOp* current_node_ptr = nullptr;
		std::atomic<std::int8_t> counter = 1;

		Op(NextRx next_receiver, Sched scheduler, SrcSndr source_sender, SndrFn sender_fn)
			: Receiver{next_receiver}
			, SchedulerOp{scheduler.sender()}
			, SourceOp{source_sender}
			, DependentOp{}
			, scheduler{scheduler}
			, sender_fn{sender_fn}
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
			auto dependent_sender = sender_fn(SplitNodeSender<Op, SrcSndr>{this});
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

	
	template<IsScheduler Sched, IsSender SrcSndr, class SndrFn>
	struct Sender {
		using SenderOptIn = ex::SenderOptIn;
		using value_t = std::invoke_result_t<SndrFn, SrcSndr>::value_t;
		using error_t = std::invoke_result_t<SndrFn, SrcSndr>::error_t;

		[[no_unique_address]] Sched scheduler;
		[[no_unique_address]] SrcSndr source_sender;
		[[no_unique_address]] SndrFn sender_fn;

		template<IsReceiver NextRx>
		auto connect(NextRx next_receiver){
			return Op{next_receiver, scheduler, source_sender, sender_fn};
		}
	};

	struct FnObj {

		static auto operator()(IsScheduler auto scheduler, IsSender auto source_sender, auto sender_fn){
			return Sender{scheduler, source_sender, sender_fn};
		}
		
		static auto operator()(IsScheduler auto scheduler, auto sender_fn){
			return [=](IsSender auto source_sender){
				return Sender{scheduler, source_sender, sender_fn};
			};
		}
		
		
	};

	
}}//namespace ex::split_algorithm

namespace ex {

		inline constexpr auto split = split_algorithm::FnObj{};

}//namespace ex



#endif//SPLIT_H
