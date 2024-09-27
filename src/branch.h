#ifndef BRANCH_H
#define BRANCH_H

#include <atomic>
#include "sender.h"
#include "scheduler.h"

template<typename BOP>
struct branch_recvr1 {
	auto set_value(auto v1){
		auto* byte_p = reinterpret_cast<std::byte*>(this) - offsetof(BOP, op1);
		auto& bop = *reinterpret_cast<BOP*>(byte_p);
		
		bop.r1 = v1;
		auto old = bop.counter.fetch_sub(1);

		if(old == std::numeric_limits<std::int8_t>::max()){
			bop.counter.store(0);
			return ::start(bop.op2);
		}

		if(old == 0){
			return bop.end_recvr.set_value(bop.r1, bop.r2);
		}

		return;
	}
};

template<typename BOP>
struct branch_recvr2 {
	auto set_value(auto v2){
		auto* byte_p = reinterpret_cast<std::byte*>(this) - offsetof(BOP, op2);
		auto& bop = *reinterpret_cast<BOP*>(byte_p);
		
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
	using R1 = single_value_t<S1>;
	using R2 = single_value_t<S2>;

	using SELF = branch_op<S1, S2, ER>;
	using BR1 = branch_recvr1<SELF>;
	using OP1 = connect_t<S1, BR1>;
	using BR2 = branch_recvr2<SELF>;
	using OP2 = connect_t<S2, BR2>;

	[[no_unique_address]] ER end_recvr;

	struct Param {
		[[no_unique_address]] SchedulerHandle scheduler;
		[[no_unique_address]] S1 sender1;
		[[no_unique_address]] S2 sender2;
	};

	union {
		Param param;
		OP1 op1;
		R1 r1;
	};

	union {
		OP2 op2;
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
	using value_t = values_cat_t<S1, S2>;

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
