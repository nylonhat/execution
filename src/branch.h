#ifndef BRANCH_H
#define BRANCH_H

#include <atomic>
#include "sender.h"
#include "scheduler.h"

template<size_t Index, class T>
constexpr auto& get_result(T& nested_op){
    if constexpr (T::index == Index){
        return nested_op.result;
    }else{
        return get_result<Index>(nested_op.next);
    }
}

template<size_t Index, class T>
consteval auto get_offset(){
    if constexpr (T::index == Index){
        return 0;
    }else{
        return offsetof(T, next) + get_offset<Index, typename T::Next>();
    }
}

template<size_t Index, class BOP, class... Sender>
struct NestedOp;


template<size_t Index, class BaseOp, class Sender, class... Senders>
struct NestedOp <Index, BaseOp, Sender, Senders...> {
    struct Empty{};

    using Next = std::conditional_t<Index == BaseOp::size-1, Empty , NestedOp<Index+1, BaseOp, Senders...> >;

    using Recvr =  BaseOp::template IndexRecvr<Index>;
    using Op = connect_t<Sender, Recvr>;
    using Result = single_value_t<Sender>;

    constexpr static size_t index = Index;

    union {
        Sender sender;
        Op op;
        Result result;
    };

    [[no_unique_address]] Next next;

    NestedOp(Sender sender, Senders... senders)
        : sender{sender}
        , next{senders...}
    {}
    
    auto start(auto&&... cont){
        auto* offset_ptr = reinterpret_cast<std::byte*>(this) - offsetof(BaseOp, tuple) - get_offset<Index, typename BaseOp::Tuple>();
		auto& base_op = *reinterpret_cast<BaseOp*>(offset_ptr);

        new (&op) Op (::connect(sender, Recvr{}));

        if constexpr(Index == BaseOp::size-1){
            return ::start(op, std::forward<decltype(cont)>(cont)...);
        }else{
            if(base_op.scheduler.try_schedule(next)){
                return ::start(op, std::forward<decltype(cont)>(cont)...);
            }

            return ::start(op, next, std::forward<decltype(cont)>(cont)...); 
        }
    }
    
};

template<class BOP, class... Sender>
using OpTuple = NestedOp<0, BOP, Sender...>;


template<size_t Index, class BaseOp>
struct BranchRecvr {
	
    template<class... Cont>
	auto set_value(Cont&&... cont, auto value){
        
		auto* offset_ptr = reinterpret_cast<std::byte*>(this) - offsetof(BaseOp, tuple) - get_offset<Index, typename BaseOp::Tuple>();
		auto& base_op = *reinterpret_cast<BaseOp*>(offset_ptr);
		
        get_result<Index>(base_op.tuple) = value;
		
        auto old = base_op.counter.fetch_sub(1);

		if(old == 0){
			return ::set_value.operator()<typename BaseOp::ER, Cont...>(base_op.end_recvr, std::forward<Cont>(cont)..., get_result<0>(base_op.tuple), get_result<1>(base_op.tuple));
		}

		return ::start(std::forward<decltype(cont)>(cont)...);
        
	}
};


template<Scheduler Sched, typename EndRecvr, Sender... Senders>
struct BranchOp {

	using Self = BranchOp<Sched, EndRecvr, Senders...>;
    using ER = EndRecvr;
    template<size_t Index>
    using IndexRecvr = BranchRecvr<Index, Self>;
    using Tuple = OpTuple<Self, Senders...>;
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
        , tuple{{senders}...}
	{}

	auto start(auto&&... cont){
		
        //if(scheduler.try_schedule(tuple)){
           //return ::start(std::forward<decltype(cont)>(cont)...);
        //}

        return ::start(tuple, std::forward<decltype(cont)>(cont)...);
	}

};

template<Scheduler Sched, Sender S1, Sender S2>
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

auto branch = [](Scheduler auto& scheduler, Sender auto sender1, Sender auto sender2){
	return BranchSender{SchedulerHandle{scheduler}, sender1, sender2};
};

#endif//BRANCH_H
