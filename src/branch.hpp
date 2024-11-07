#ifndef BRANCH_H
#define BRANCH_H

#include <atomic>
#include "scheduler.hpp"
#include "nested_op.hpp"
#include "concepts.hpp"

namespace ex::algorithms::branch {

	template<size_t Index, class BaseOp>
	struct BranchRecvr {
	    constexpr static size_t index = Index;

		template<class... Cont>
		auto set_value(Cont&... cont, auto value){
			auto& base_op = get_base_op<BaseOp>(this);
	        get_result<Index>(base_op.tuple) = value;

	        auto old = base_op.counter.fetch_sub(1);

	        using Next = BaseOp::template IndexTuple<Index+1>;
        
	        if constexpr(!first_same_as<Next, Cont...>){
			    if(old == 0){
				    return ex::set_value.operator()<typename BaseOp::ER, Cont...>(
				        base_op.end_recvr, cont..., 
				        get_result<0>(base_op.tuple), 
				        get_result<1>(base_op.tuple)
				    );
			    }
			}

			return ex::start(cont...);
		}
	};


	template<Scheduler Sched, typename EndRecvr, IsSender... Senders>
	struct BranchOp {

		using Self = BranchOp<Sched, EndRecvr, Senders...>;
	    using ER = EndRecvr;
	    template<size_t Index>
	    using IndexRecvr = BranchRecvr<Index, Self>;
	    template<size_t Index>
	    using IndexTuple = NestedOp<Index, Self>;
	    using Tuple = OpTuple<Self>;
	    using SenderTypeList = std::tuple<Senders...>;
	    static constexpr size_t size = sizeof...(Senders);


		[[no_unique_address]] EndRecvr end_recvr;
	
		Sched scheduler;

	    union{
	        Tuple tuple;
	    };

		std::atomic<std::int8_t> counter = 1;

		BranchOp(Sched scheduler, EndRecvr end_recvr, Senders... senders)
			: end_recvr{end_recvr}
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

template<Scheduler Sched, IsSender S1, IsSender S2>
struct BranchSender {
	using value_t = values_join_t<S1, S2>;

	Sched scheduler;
	[[no_unique_address]] S1 sender1;
	[[no_unique_address]] S2 sender2;
		
	template<typename ER>
	auto connect(ER end_recvr){
		return BranchOp{scheduler, end_recvr, sender1, sender2};
	}
};

}//namespace ex::algorithms::branch

namespace ex {

	inline constexpr auto branch = [](Scheduler auto& scheduler, IsSender auto sender1, IsSender auto sender2){
		return ex::algorithms::branch::BranchSender{SchedulerHandle{scheduler}, sender1, sender2};
	};

}//namespace ex

#endif//BRANCH_H
