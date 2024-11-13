#ifndef BRANCH_H
#define BRANCH_H

#include <atomic>
#include "scheduler.hpp"
#include "nested_op.hpp"
#include "concepts.hpp"

namespace ex::algorithms::branch {

	template<size_t Index, class BaseOp>
	struct Receiver {
	    constexpr static size_t index = Index;

		template<IsOpState... Cont>
		auto set_value(Cont&... cont, auto value){
			auto& base_op = get_base_op<BaseOp>(this);
	        get_result<Index>(base_op.tuple) = value;

	        auto old = base_op.counter.fetch_sub(1);

	        using Next = BaseOp::template IndexTuple<Index+1>;
        
	        if constexpr(!first_same_as<Next, Cont...>){
			    if(old == 0){
					auto lambda = [&](auto&&... values){
						return ex::set_value.operator()<typename BaseOp::NextReceiver, Cont...>(
					        base_op.next_receiver, cont..., 
					        std::forward<decltype(values)>(values)...
					    );					
					};

					return apply(lambda, base_op.tuple);
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

	    union{
	        Tuple tuple;
	    };

		std::atomic<std::int8_t> counter = size - 1;

		OpState(Scheduler scheduler, SuffixReceiver suffix_receiver, Senders... senders)
			: next_receiver{suffix_receiver}
			, scheduler{scheduler}
	        , tuple{senders...}
		{}

		auto start(auto&... cont){
		
	        //if(scheduler.try_schedule(tuple)){
	        //   return ::start(std::forward<decltype(cont)>(cont)...);
	        //}

	        return ex::start(tuple, cont...);
		}

	};

	/*
	template<IsScheduler Scheduler, IsSender S1, IsSender S2>
	struct Sender {
		using value_t = values_join_t<S1, S2>;

		Scheduler scheduler;
		[[no_unique_address]] S1 sender1;
		[[no_unique_address]] S2 sender2;
		
		template<IsReceiver SuffixReceiver>
		auto connect(SuffixReceiver suffix_receiver){
			return OpState{scheduler, suffix_receiver, sender1, sender2};
		}
	};
	*/

	
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

	struct Function {
		
		auto operator()(this auto&&, IsScheduler auto& scheduler, IsSender auto... senders){
			auto handle = SchedulerHandle{scheduler};
			
			auto lambda = [&](auto... senders){
				return Sender{handle, senders...};
			};

			return std::apply(lambda, std::tuple_cat((Sender{handle, senders}.tuple)...));
			
		}

		
		//auto operator()(this auto&&, IsScheduler auto& scheduler, IsSender auto sender){
			//TODO pipeable version??
		//}
		
	};

}//namespace ex::algorithms::branch

namespace ex {

	//inline constexpr auto branch = [](IsScheduler auto& scheduler, IsSender auto sender1, IsSender auto sender2){
	//	return ex::algorithms::branch::Sender{SchedulerHandle{scheduler}, sender1, sender2};
	//};

	inline constexpr auto branch = algorithms::branch::Function{};

}//namespace ex

#endif//BRANCH_H
