#ifndef BRANCH_H
#define BRANCH_H

#include <atomic>
#include "sender.h"
#include "scheduler.h"

template<typename BOP>
struct BranchRecvr1 {
	
	auto set_value(auto&& cont, auto v1){
		auto* byte_p = reinterpret_cast<std::byte*>(this) - offsetof(BOP, op1);
		auto& bop = *reinterpret_cast<BOP*>(byte_p);
		
		bop.r1 = v1;
		auto old = bop.counter.fetch_sub(1);

		if(old == 0){
			return bop.end_recvr.set_value(std::forward<decltype(cont)>(cont), bop.r1, bop.r2);
		}

		return ::start(cont, Noop{});
	}
};

template<typename BOP>
struct BranchRecvr2 {

	auto set_value(auto&& cont, auto v2){
		auto* byte_p = reinterpret_cast<std::byte*>(this) - offsetof(BOP, op2);
		auto& bop = *reinterpret_cast<BOP*>(byte_p);

		bop.r2 = v2;
		auto old = bop.counter.fetch_sub(1);

		if(old == 0){
			return bop.end_recvr.set_value(std::forward<decltype(cont)>(cont), bop.r1, bop.r2);
		}

		return start(cont, Noop{});
	}
};

template<class BOP>
struct Loop1 {

	auto start(auto&&){
		auto* byte_p = reinterpret_cast<std::byte*>(this) - offsetof(BOP, loop1);
		auto& bop = *reinterpret_cast<BOP*>(byte_p);

		auto shortcut = bop.scheduler.schedule(bop.loop2);
		
		return ::start(bop.op1, std::move(shortcut));
	}
};

template<class BOP>
struct Loop2 {

	auto start(auto&&){
		auto* byte_p = reinterpret_cast<std::byte*>(this) - offsetof(BOP, loop2);
		auto& bop = *reinterpret_cast<BOP*>(byte_p);

		auto cont_copy = bop.cont;
		::start(bop.op2, std::move(cont_copy));
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
	
	SchedulerHandle scheduler;
	OpHandle cont;

	union {
		OP1 op1;
		R1 r1;
	};

	union {
		OP2 op2;
		R2 r2;
	};

	[[no_unique_address]] Loop1<SELF> loop1 = {};
	[[no_unique_address]] Loop2<SELF> loop2 = {};

	std::atomic<std::int8_t> counter = 1;

	BranchOp(SchedulerHandle scheduler, S1 sender1, S2 sender2, ER end_recvr)
		: end_recvr{end_recvr}
		, scheduler{scheduler}
		, op1{::connect(sender1, BR1{})}
		, op2{::connect(sender2, BR2{})}
	{}

	auto start(auto&& continuation){
		//scheduler.schedule({});
		//auto shortcut = scheduler.schedule(op2);
		//cont = op_handle;
		//::start(op1, shortcut);
		
		cont = continuation;
		auto shortcut = scheduler.schedule(loop1);
		return ::start(shortcut, Noop{});
		//return ::start(loop1, Noop{});
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

/*
#include <vector>



template<typename SR, typename ER>
struct BranchRangeOp {
	using SENDER_T = SR::range_value_t;
	using RES_T = SENDER_T::value_t;
	using SIZE = SR::size; 
	using SELF = BranchRangeOp<SR, ER>;
	
	using OP = connect_t<SENDER_T, ???>;


	[[no_unique_address]] ER end_recvr;

	std::array<OP> ops;
	std::array<RES_T> results;

	SchedulerHandle scheduler;
	int schedule_count = 0;
	std::atomic<std::int8_t> counter = SIZE -1;
	

	BranchRangeOp(SchedulerHandle scheduler, SR sender_range, ER end_recvr)
		: end_recvr{end_recvr}
		, scheduler{scheduler}
	{
	}


	auto start(){
		//construct ops[schedule_count];
		int idx = schedule_count++;
		scheduler.schedule(*this);
		::start(ops.at(idx));
	}

};




template<typename SR>
struct BranchRangeSender {
	using value_t = std::tuple<std::array<typename SR::range_value_t, SR::range_size_t>>;

	[[no_unique_address]] SchedulerHandle scheduler;
	[[no_unique_address]] SR sender_range;
	
	template<typename ER>
	auto connect(ER end_recvr){
		return BranchRangeOp{scheduler, sender_range, end_recvr};
	}
};


auto branch_range = [](Scheduler auto& scheduler, auto sender_range){
	return BranchRangeSender{scheduler, sender_range};
};

*/


#endif//BRANCH_H
