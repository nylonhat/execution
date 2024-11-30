#ifndef BRANCH_H
#define BRANCH_H

#include <atomic>
#include "scheduler.hpp"
#include "nested_op.hpp"
#include "concepts.hpp"

#include "inline.hpp"

namespace ex::algorithms::branch_all {
	template<size_t A, size_t B>
	concept same_index = (A == B);

	template<size_t A, size_t B>
	concept not_same_index = (A != B);

	template<class... Pack>
	concept not_empty_pack = sizeof...(Pack) != 0;

	template<class T, class... Pack>
	concept first_same_as2 = std::same_as<Pack...[0], T>;

	template<class...>
	struct OpStateBase;

	template<IsReceiver SuffixReceiver, IsScheduler Scheduler, IsSender... Senders, std::size_t... I>
	struct OpStateBase<std::index_sequence<I...>, SuffixReceiver, Scheduler, Senders...>
		: InlinedReceiver<OpStateBase<std::index_sequence<I...>, SuffixReceiver, Scheduler, Senders...>, SuffixReceiver>
		, ManualChildResultOp<OpStateBase<std::index_sequence<I...>, SuffixReceiver, Scheduler, Senders...>, I, Senders>...
	{
		
		using OpStateOptIn = ex::OpStateOptIn;
		using Receiver = InlinedReceiver<OpStateBase, SuffixReceiver>;
		template<std::size_t ChildIndex> using ChildOp = ManualChildResultOp<OpStateBase, ChildIndex, Senders...[ChildIndex]>;

		static constexpr std::size_t size = sizeof...(Senders);
		
		Scheduler scheduler;
		std::atomic<std::int8_t> counter = size - 1;

		OpStateBase(SuffixReceiver suffix_receiver, Scheduler scheduler, Senders... senders)
			: Receiver{suffix_receiver}
			, ChildOp<I>{senders}...
			, scheduler{scheduler}
		{}

		template<class... Cont>
		auto start(Cont&... cont){
			return ChildOp<0>::start_sender_op(cont...);
		}

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class ChildSender>
			requires same_index<VariantIndex, 0> && same_index<ChildIndex, size - 1> 
        auto set_value(Cont&... cont, ChildSender child_sender){
			
			auto& child_op = ChildOp<ChildIndex>::construct_from(child_sender);
			
			return ex::start(child_op, cont...);
        }

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class ChildSender>
			requires same_index<VariantIndex, 0> && (ChildIndex != size -1)
        auto set_value(Cont&... cont, ChildSender child_sender){

			auto& child_op = ChildOp<ChildIndex>::construct_from(child_sender);
        	auto& next_op = ChildOp<ChildIndex+1>::get_sender_op();

			if(scheduler.try_schedule(next_op)){
        		return ex::start(child_op, cont...);
        	}

        	return ex::start(child_op, next_op, cont...);	
        }

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Args>
			requires same_index<VariantIndex, 1> 
			&& not_same_index<ChildIndex, size - 1> //not last loop
			&& not_empty_pack<Cont...>
			&& first_same_as2<typename ChildOp<ChildIndex+1>::SenderOp, Cont...> //next loop was not scheduled
        auto set_value(Cont&... cont, Args... args){
        	ChildOp<ChildIndex>::construct_result(args...);
	        counter.fetch_sub(1);
			return ex::start(cont...);
        }
        
		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Args>
			requires same_index<VariantIndex, 1>
        auto set_value(Cont&... cont, Args... args){
        	ChildOp<ChildIndex>::construct_result(args...);
	        auto old = counter.fetch_sub(1);

			if(old == 0){
				return ex::set_value.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., ChildOp<I>::get_result()...);
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
				// return OpState{scheduler, suffix_receiver, child_senders...};
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
