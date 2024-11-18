#ifndef BRANCH_H
#define BRANCH_H

#include <atomic>
#include "scheduler.hpp"
#include "nested_op.hpp"
#include "concepts.hpp"

namespace ex::algorithms::branch_all {

	template<size_t Index, class BaseOp>
	struct Receiver {
        using Next = BaseOp::template IndexTuple<Index+1>;
	    constexpr static size_t index = Index;

		template<IsOpState... Cont>
		auto set_value(Cont&... cont, auto value){
			auto& base_op = get_base_op<BaseOp>(this);
	        get_result<Index>(base_op.tuple) = value;

	        auto old = base_op.counter.fetch_sub(1);
        
	        if constexpr(!first_same_as<Next, Cont...>){
			    if(old == 0){
					return apply_set_value(base_op.tuple, base_op.next_receiver, cont...);
			    }
			}

			return ex::start(cont...);
		}
	};


	template<IsScheduler Scheduler, IsReceiver SuffixReceiver, IsSender... Senders>
	struct OpState {

		using Self = OpState<Scheduler, SuffixReceiver, Senders...>;
	    using NextReceiver = SuffixReceiver;
	    template<size_t Index> using IndexRecvr = Receiver<Index, Self>;
	    template<size_t Index> using IndexTuple = NestedOp<Index, Self>;
	    using Tuple = OpTuple<Self>;
	    using SenderTypeList = std::tuple<Senders...>;
	    static constexpr size_t size = sizeof...(Senders);


		[[no_unique_address]] NextReceiver next_receiver;	
		Scheduler scheduler;
	    Tuple tuple;
		std::atomic<std::int8_t> counter = size - 1;

		OpState(Scheduler scheduler, SuffixReceiver suffix_receiver, Senders... senders)
			: next_receiver{suffix_receiver}
			, scheduler{scheduler}
	        , tuple{senders...}
		{}

		auto start(auto&... cont){
		
	        //if(scheduler.try_schedule(tuple)){
	        //   return ::start(cont...);
	        //}

	        return ex::start(tuple, cont...);
		}

	};
	
	template<IsScheduler Scheduler, IsSender... ChildSenders>
	struct Sender {
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
			auto lambda = [&](auto&&... child_senders){
				return OpState{scheduler, suffix_receiver, child_senders...};
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
