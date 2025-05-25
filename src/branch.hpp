#ifndef BRANCH_H
#define BRANCH_H

#include <atomic>
#include "scheduler.hpp"
#include "branch_child.hpp"
#include "concepts.hpp"
#include "inlined_receiver.hpp"
#include "variant_child.hpp"

namespace ex::algorithms::branch_all {
	
	struct ChildTag {
		std::size_t child_index = 0;
	};

	template<class...>
	struct OpStateBase;

	template<IsReceiver SuffixReceiver, IsScheduler Scheduler, IsSender... Senders, std::size_t... I>
	struct OpStateBase<std::index_sequence<I...>, SuffixReceiver, Scheduler, Senders...>
		: InlinedReceiver<SuffixReceiver>
		, VariantChildOp<
			OpStateBase<std::index_sequence<I...>, SuffixReceiver, Scheduler, Senders...>, 
			0, 
			std::conditional_t<true, typename Scheduler::sender_t, decltype(I)>...
		  >
		, BranchChildOp<OpStateBase<std::index_sequence<I...>, SuffixReceiver, Scheduler, Senders...>, ChildTag{I}, Senders>...
	{
		
		using OpStateOptIn = ex::OpStateOptIn;
		using Receiver = InlinedReceiver<SuffixReceiver>;
		using SchedulerSender = Scheduler::sender_t;
		using SchedulerOp = VariantChildOp<
			OpStateBase, 
			0, 
			std::conditional_t<true, SchedulerSender, decltype(I)>...
		>;
		
		template<std::size_t Index> using ChildOp = BranchChildOp<OpStateBase, ChildTag{Index}, Senders...[Index]>;

		static constexpr std::size_t size = sizeof...(Senders);
		
		Scheduler scheduler;
		std::atomic<std::int8_t> counter = size - 1;

		OpStateBase(SuffixReceiver suffix_receiver, Scheduler scheduler, Senders... senders)
			: Receiver{suffix_receiver}
			, SchedulerOp{scheduler.sender()}
			, ChildOp<I>{senders}...
			, scheduler{scheduler}
		{}

		template<class... Cont>
		auto start(Cont&... cont){
			//return SchedulerOp::template start<0, Cont...>(cont...);
			[[gnu::musttail]] return set_value<size, 0, Cont...>(cont...); 
		}
		
		//Scheduler Callback
		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont>
		auto set_value(Cont&... cont){
			auto& child_sender = ChildOp<VariantIndex>::get_sender();
			auto& child_op = ChildOp<VariantIndex>::construct_from(child_sender);
			
			if constexpr(!same_index<VariantIndex, size - 1>){
				auto& scheduler_op = SchedulerOp::template construct_from<VariantIndex+1>(scheduler.sender());
				[[gnu::musttail]] return ex::start(scheduler_op, child_op, cont...);
			} else {
				[[gnu::musttail]] return ex::start(child_op, cont...);
			}
		}
		

		//Child Result Callback (No schedule)
		template<ChildTag tag, class... Cont, class... Args>
			requires first_same_as<typename SchedulerOp::template VariantOp<tag.child_index+1>::Loopback, Cont...> 
			|| first_same_as<typename ChildOp<tag.child_index+1>::ChildOp, Cont...>
		auto set_value(Cont&... cont, Args... args){
        	ChildOp<tag.child_index>::construct_result(args...);
	        counter.fetch_sub(1);
			[[gnu::musttail]] return ex::start(cont...);
        }
        
		//Child Result Callback
		template<ChildTag tag, class... Cont, class... Args>
		auto set_value(Cont&... cont, Args... args){
        	ChildOp<tag.child_index>::construct_result(args...);
	        auto old = counter.fetch_sub(1);
			
			if(old == 0){
				static_assert(sizeof...(cont) == 0);
				[[gnu::musttail]] return ex::set_value<Cont...>(this->get_receiver(), cont..., ChildOp<I>::get_result()...);
			}

			[[gnu::musttail]] return ex::start(cont...);
        }

	};

	template<IsReceiver SuffixReceiver, IsScheduler Scheduler, IsSender... Senders>
	struct OpState 
		: OpStateBase<std::index_sequence_for<Senders...>, SuffixReceiver, Scheduler, Senders...>
	{
		OpState(SuffixReceiver suffix_receiver, Scheduler scheduler, Senders... senders)
			: OpStateBase<std::index_sequence_for<Senders...>, SuffixReceiver, Scheduler, Senders...>{suffix_receiver, scheduler, senders...}
		{}
	};
	
	template<IsScheduler Scheduler, IsSender... ChildSenders>
	struct Sender {
		using SenderOptIn = ex::SenderOptIn;
		using value_t = values_join_t<ChildSenders...>;
		using error_t = ChildSenders...[0]::error_t;

		Scheduler scheduler;
		std::tuple<ChildSenders...> tuple;

		Sender(const Sender&) = default;

		Sender(Scheduler scheduler, ChildSenders... child_senders)
			: scheduler{scheduler}
			, tuple{child_senders...}
		{}

		Sender(Scheduler scheduler, Sender<Scheduler, ChildSenders...> branch_sender)
			: scheduler{scheduler}
			, tuple{branch_sender.tuple}
		{}
		
		template<IsReceiver SuffixReceiver>
		auto connect(SuffixReceiver suffix_receiver){
			auto lambda = [&](auto... child_senders){
				return OpState{suffix_receiver, scheduler, child_senders...};
			};

			return std::apply(lambda, tuple);
		}
	};

	struct FunctionObject {

		auto operator()(this auto&&, IsScheduler auto scheduler, IsSender auto... senders){
			
			auto lambda = [&](auto... senders){
				return Sender{scheduler, senders...};
			};
			
			//Combine branches together if same scheduler
			return std::apply(lambda, std::tuple_cat((Sender{scheduler, senders}.tuple)...));		
		}
		
	};

	
}//namespace ex::algorithms::branch_all

namespace ex {

		inline constexpr auto branch_all = algorithms::branch_all::FunctionObject{};

}//namespace ex

namespace ex::algorithms::branch {
	
	struct FunctionObject {
		
		template<IsScheduler Scheduler, IsSender Sender1, IsSender Sender2>
		auto operator()(this auto&& self, Scheduler scheduler, Sender1 sender1, Sender2 sender2){
			return ex::branch_all(scheduler, sender1, sender2);
		}

		
		template<IsScheduler Scheduler, IsSender Sender2>
		auto operator()(this auto&& self, Scheduler scheduler, Sender2 sender2){
			return [scheduler, sender2]<IsSender Sender1>(Sender1 sender1){
				return ex::branch_all(scheduler, sender1, sender2);
			};
		}
	};
	
}//namespace ex::algorithms::branch

namespace ex {

	inline constexpr auto branch = algorithms::branch::FunctionObject{};
	
}//namespace ex

#endif//BRANCH_H
