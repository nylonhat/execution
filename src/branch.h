#ifndef BRANCH_H
#define BRANCH_H

#include <atomic>
#include "sender.h"
#include "scheduler.h"

template<typename BOP>
struct BranchRecvr1 {
	auto set_value(auto v1){
		auto* byte_p = reinterpret_cast<std::byte*>(this) - offsetof(BOP, op1);
		auto& bop = *reinterpret_cast<BOP*>(byte_p);
		
		bop.r1 = v1;
		auto old = bop.counter.fetch_sub(1);

		if(old == 2){
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
struct BranchRecvr2 {
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
struct BranchOp {
	using R1 = single_value_t<S1>;
	using R2 = single_value_t<S2>;

	using SELF = BranchOp<S1, S2, ER>;
	using BR1 = BranchRecvr1<SELF>;
	using OP1 = connect_t<S1, BR1>;
	using BR2 = BranchRecvr2<SELF>;
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
	

	BranchOp(SchedulerHandle scheduler, S1 sender1, S2 sender2, ER end_recvr)
		: end_recvr{end_recvr}
		, param{scheduler, sender1, sender2}
	{}



	auto start(){
		auto p = param;
		new (&op2) OP2 (::connect(p.sender2, BR2{}));

		std::int8_t shortcut = !p.scheduler.schedule(op2);
		counter.fetch_add(shortcut);

		new (&op1) OP1 (::connect(p.sender1, BR1{}));
		::start(op1);
	}

};

template<Sender S1, Sender S2>
struct BranchSender {
	using value_t = values_join_t<S1, S2>;

	[[no_unique_address]] SchedulerHandle scheduler;
	[[no_unique_address]] S1 sender1;
	[[no_unique_address]] S2 sender2;
	
	template<typename ER>
	auto connect(ER end_recvr){
		return BranchOp{scheduler, sender1, sender2, end_recvr};
	}
};

auto branch = [](Scheduler auto& scheduler, Sender auto sender1, Sender auto sender2){
	return BranchSender{scheduler, sender1, sender2};
};

#endif//BRANCH_H
