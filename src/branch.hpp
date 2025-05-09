#ifndef BRANCH_H
#define BRANCH_H

#include <atomic>
#include "scheduler.hpp"
#include "branch_child.hpp"
#include "concepts.hpp"
#include "inlined_receiver.hpp"
#include "variant_child.hpp"

namespace ex::algorithms::branch_all {

	template<class...>
	struct OpStateBase;

	template<IsReceiver SuffixReceiver, IsScheduler Scheduler, IsSender... Senders, std::size_t... I>
	struct OpStateBase<std::index_sequence<I...>, SuffixReceiver, Scheduler, Senders...>
		: InlinedReceiver<OpStateBase<std::index_sequence<I...>, SuffixReceiver, Scheduler, Senders...>, SuffixReceiver>
		, VariantChildOp<
			OpStateBase<std::index_sequence<I...>, SuffixReceiver, Scheduler, Senders...>, 
			sizeof...(Senders), 
			std::conditional_t<true, typename Scheduler::sender_t, decltype(I)>...
		  >
		, BranchChildOp<OpStateBase<std::index_sequence<I...>, SuffixReceiver, Scheduler, Senders...>, I, Senders>...
	{
		
		using OpStateOptIn = ex::OpStateOptIn;
		using Receiver = InlinedReceiver<OpStateBase, SuffixReceiver>;
		using SchedulerSender = Scheduler::sender_t;
		using SchedulerOp = VariantChildOp<
			OpStateBase, 
			sizeof...(Senders), 
			std::conditional_t<true, SchedulerSender, decltype(I)>...
		>;
		
		template<std::size_t ChildIndex> using ChildOp = BranchChildOp<OpStateBase, ChildIndex, Senders...[ChildIndex]>;

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
			return set_value<size, 0, Cont...>(cont...); 
		}
		
		//Scheduler Callback
		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont>
			requires same_index<ChildIndex, size>
		auto set_value(Cont&... cont){
			auto& child_sender = ChildOp<VariantIndex>::get_sender();
			auto& child_op = ChildOp<VariantIndex>::construct_from(child_sender);
			
			if constexpr(!same_index<VariantIndex, size - 1>){
				auto& scheduler_op = SchedulerOp::template construct_from<VariantIndex+1>(scheduler.sender());
				return ex::start(cont..., scheduler_op, child_op);
			} else {
				return ex::start(cont..., child_op);
			}
		}
		

		//Child Result Callback (No schedule)
		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Args>
			requires not_same_index<ChildIndex, size> 
			&& first_same_as<typename ChildOp<ChildIndex+1>::ChildOp, Cont...>
		auto set_value(Cont&... cont, Args... args){
        	ChildOp<ChildIndex>::construct_result(args...);
	        counter.fetch_sub(1);
			return ex::start(cont...);
        }
        
		//Child Result Callback
		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Args>
			requires not_same_index<ChildIndex, size>
		auto set_value(Cont&... cont, Args... args){
        	
        	ChildOp<ChildIndex>::construct_result(args...);
	        auto old = counter.fetch_sub(1);
			
			if(old == 0){
				return ex::set_value<Cont...>(this->get_receiver(), cont..., ChildOp<I>::get_result()...);
			}

			return ex::start(cont...);
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

		auto operator()(this auto&&, IsScheduler auto& scheduler, IsSender auto... senders){
			auto handle = SchedulerHandle{scheduler};
			
			auto lambda = [&](auto... senders){
				return Sender{handle, senders...};
			};

			return std::apply(lambda, std::tuple_cat((Sender{handle, senders}.tuple)...));		
		}
		
	};

	
}//namespace ex::algorithms::branch

namespace ex {

		inline constexpr auto branch_all = algorithms::branch_all::FunctionObject{};

}//namespace ex

namespace ex::algorithms::branch {
	
	struct FunctionObject {
		
		template<IsScheduler Scheduler, IsSender Sender1, IsSender Sender2>
		auto operator()(this auto&& self, Scheduler& scheduler, Sender1 sender1, Sender2 sender2){
			return ex::branch_all(scheduler, sender1, sender2);
		}

		
		template<IsScheduler Scheduler, IsSender Sender2>
		auto operator()(this auto&& self, Scheduler& scheduler, Sender2 sender2){
			return [&scheduler, sender2]<IsSender Sender1>(Sender1 sender1){
				return ex::branch_all(scheduler, sender1, sender2);
			};
		}
	};
	
}//namespace ex::algorithms::branch::single

namespace ex {

	inline constexpr auto branch = algorithms::branch::FunctionObject{};
	
}//namespace ex

#endif//BRANCH_H
