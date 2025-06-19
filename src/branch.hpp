#ifndef BRANCH_H
#define BRANCH_H

#include <atomic>
#include "scheduler.hpp"
#include "child_storage.hpp"
#include "concepts.hpp"
#include "signature.hpp"
#include "inlined_receiver.hpp"
#include "child_variant.hpp"

namespace ex {
inline namespace branch_all_algorithm {
	
	struct ChildTag {
		std::size_t child_index = 0;
	};

	template<class...>
	struct OpBase;

	template<IsReceiver NextRx, IsScheduler Sched, IsSender... Senders, std::size_t... I>
	struct OpBase<std::index_sequence<I...>, NextRx, Sched, Senders...>
		: InlinedReceiver<OpBase<std::index_sequence<I...>, NextRx, Sched, Senders...>, NextRx>
		, ChildVariant<
			OpBase<std::index_sequence<I...>, NextRx, Sched, Senders...>, 
			0, 
			std::conditional_t<true, typename Sched::sender_t, decltype(I)>...
		  >
		, ChildStorage<OpBase<std::index_sequence<I...>, NextRx, Sched, Senders...>, ChildTag{I}, Senders>...
	{
		
		using OpStateOptIn = ex::OpStateOptIn;
		using Receiver = InlinedReceiver<OpBase, NextRx>;
		using SchedulerSender = Sched::sender_t;
		using SchedulerOp = ChildVariant<
			OpBase, 
			0, 
			std::conditional_t<true, SchedulerSender, decltype(I)>...
		>;
		
		template<std::size_t Index> using ChildOp = ChildStorage<OpBase, ChildTag{Index}, Senders...[Index]>;

		static constexpr std::size_t size = sizeof...(Senders);
		
		Sched scheduler;
		std::atomic<std::int8_t> counter = size - 1;

		OpBase(NextRx next_receiver, Sched scheduler, Senders... senders)
			: Receiver{next_receiver}
			, SchedulerOp{scheduler.sender()}
			, ChildOp<I>{senders}...
			, scheduler{scheduler}
		{}

		template<class... Cont>
		auto start(Cont&... cont){
			if constexpr(size == 0){
				[[gnu::musttail]] return ex::set_value<Cont...>(this->get_receiver(), cont...);
			}
			//return SchedulerOp::template start<0, Cont...>(cont...);
			[[gnu::musttail]] return set_value<size, 0, Cont...>(cont...); 
		}
		
		//Sched Callback
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

	template<IsReceiver NextRx, IsScheduler Sched, IsSender... Senders>
	struct Op 
		: OpBase<std::index_sequence_for<Senders...>, NextRx, Sched, Senders...>
	{
		Op(NextRx next_receiver, Sched scheduler, Senders... senders)
			: OpBase<std::index_sequence_for<Senders...>, NextRx, Sched, Senders...>{next_receiver, scheduler, senders...}
		{}
	};
	
	template<IsScheduler Sched, IsSender... ChildSenders>
	struct Sender {
		using SenderOptIn = ex::SenderOptIn;
		using value_t = sig_values_join_t<ChildSenders...>;
		using error_t = ChildSenders...[0]::error_t;

		Sched scheduler;
		std::tuple<ChildSenders...> tuple;

		Sender(const Sender&) = default;

		Sender(Sched scheduler, ChildSenders... child_senders)
			: scheduler{scheduler}
			, tuple{child_senders...}
		{}

		Sender(Sched scheduler, Sender<Sched, ChildSenders...> branch_sender)
			: scheduler{scheduler}
			, tuple{branch_sender.tuple}
		{}
		
		template<IsReceiver NextRx>
		auto connect(NextRx next_receiver){
			auto lambda = [&](auto... child_senders){
				return Op{next_receiver, scheduler, child_senders...};
			};

			return std::apply(lambda, tuple);
		}
	};

	struct FnObj {

		static auto operator()(IsScheduler auto scheduler, IsSender auto... senders){
			
			auto lambda = [&](auto... senders){
				return Sender{scheduler, senders...};
			};
			
			//Combine branches together if same scheduler
			return std::apply(lambda, std::tuple_cat((Sender{scheduler, senders}.tuple)...));		
		}
		
	};

	
}}//namespace ex::branch_all_algorithm

namespace ex {

		inline constexpr auto branch_all = branch_all_algorithm::FnObj{};

}//namespace ex

namespace ex {
inline namespace branch_algorithm {
	
	struct FnObj {
		
		template<IsScheduler Sched, IsSender Sender1, IsSender Sender2>
		static auto operator()(Sched scheduler, Sender1 sender1, Sender2 sender2){
			return ex::branch_all(scheduler, sender1, sender2);
		}

		
		template<IsScheduler Sched, IsSender Sender2>
		static auto operator()(Sched scheduler, Sender2 sender2){
			return [scheduler, sender2]<IsSender Sender1>(Sender1 sender1){
				return ex::branch_all(scheduler, sender1, sender2);
			};
		}
	};
	
}}//namespace ex::branch_algorithm

namespace ex {

	inline constexpr auto branch = branch_algorithm::FnObj{};
	
}//namespace ex

#endif//BRANCH_H
