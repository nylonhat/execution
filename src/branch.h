#ifndef BRANCH_H
#define BRANCH_H


#include <atomic>
#include "bind.h"
#include "scheduler.h"

template<typename F, typename Tuple>
using apply_result_t = std::invoke_result_t<decltype(&std::apply<F, Tuple>), F, Tuple>;	

template<typename... Tuples>
using tuple_cat_result_t =  std::invoke_result_t<decltype(&std::tuple_cat<Tuples...>), Tuples...>;	

template<typename S>
using tuple_t = std::tuple_element_t<0, typename S::value_t>;



template<typename BOp>
struct branch_recvr1 {

	auto set_value(auto v1){
		auto* byte_p = reinterpret_cast<std::byte*>(this) - offsetof(BOp, op1);
		auto& bop = *reinterpret_cast<BOp*>(byte_p);
		
		bop.r1 = v1;
		auto old = bop.counter.fetch_sub(1);

		if(old == std::numeric_limits<std::int8_t>::max()){
			return ::start(bop.op2);
		}

		if(old == 0){
			return bop.end_recvr.set_value(bop.r1, bop.r2);
		}

		return;

	}

};

template<typename BOp>
struct branch_recvr2 {

	auto set_value(auto v2){
		auto* byte_p = reinterpret_cast<std::byte*>(this) - offsetof(BOp, op2);
		auto& bop = *reinterpret_cast<BOp*>(byte_p);
		
		bop.r2 = v2;
		auto old = bop.counter.fetch_sub(1);

		if(old == 0){
			return bop.end_recvr.set_value(bop.r1, bop.r2);
		}

		return;

	}

};



template<Sender S1, Sender S2, typename ER>
struct branch_op {
	using R1 = int; //tuple_t<S1>;
	using R2 = int; //tuple_t<S2>;

	using Self = branch_op<S1, S2, ER>;
	using BR1 = branch_recvr1<Self>;
	using Op1 = std::invoke_result_t<decltype(connect), S1, BR1>;
	using BR2 = branch_recvr2<Self>;
	using Op2 = std::invoke_result_t<decltype(connect), S2, BR2>;

	[[no_unique_address]] ER end_recvr;

	struct Param {
		[[no_unique_address]] SchedulerHandle scheduler;
		[[no_unique_address]] S1 sender1;
		[[no_unique_address]] S2 sender2;
	};

	union {
		Param param;
		Op1 op1;
		R1 r1;
	};

	union {
		Op2 op2;
		R2 r2;
	};
	
	std::atomic<std::int8_t> counter = 1;
	
	branch_op(SchedulerHandle scheduler, S1 sender1, S2 sender2, ER end_recvr)
		: end_recvr{end_recvr}
		, param{scheduler, sender1, sender2}
	{}


	auto start(){
		auto p = param;
		op2 = ::connect(p.sender2, BR2{});
		if(!p.scheduler.schedule(op2)){
			counter.store(std::numeric_limits<std::int8_t>::max());
		}
		op1 = ::connect(p.sender1, BR1{});
		::start(op1);
	}

};

template<Sender S1, Sender S2>
struct branch_sender {
	using value_t = tuple_cat_result_t<typename S1::value_t, typename S2::value_t>;
	//using value_t = std::tuple<int, int>;
	[[no_unique_address]] SchedulerHandle scheduler;
	[[no_unique_address]] S1 sender1;
	[[no_unique_address]] S2 sender2;
	


	auto connect(auto end_recvr){
		return branch_op{scheduler, sender1, sender2, end_recvr};
	}
};

auto branch = [](Scheduler auto& scheduler, Sender auto sender1, Sender auto sender2){
	return branch_sender{scheduler, sender1, sender2};
};

#endif//BRANCH_H
