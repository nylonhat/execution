#ifndef REPEAT_H
#define REPEAT_H

#include <print>

#include "sender.h"
#include "timer.h"

template<typename ER, Sender S>
struct RepeatRecvr {
	[[no_unique_address]] ER end_recvr;
	[[no_unique_address]] S sender;
	int count = 0;

	void set_value(auto... args){
		using SELF = RepeatRecvr<ER,S>;
		using OP = connect_t<S, SELF>; 
		auto& op = *reinterpret_cast<OP*>(this);
		
		count++;
		auto recvr = SELF{end_recvr, sender, count};
		if(count < 1000'000'000){
			new (&op) OP (::connect(recvr.sender, recvr));
			return ::start(op);
			
		}

		return end_recvr.set_value(args...);
	}
};

template<typename ER, Sender S>
struct RepeatOp {
	using RR = RepeatRecvr<ER, S>;
	using OP = connect_t<S, RR>;
	
	OP op;

	RepeatOp(S sender, ER end_recvr)
		: op {::connect(sender, RR{end_recvr, sender, 0})}
	{}
	
	auto start(){
		::start(op);
	}
	
};

template<Sender S>
struct RepeatSender {
	using value_t = S::value_t;
	S sender;

	auto connect(auto end_recvr){
		return RepeatOp{sender, end_recvr};
	}
};

auto repeat = [](Sender auto sender){
	return RepeatSender{sender};
};

#endif//REPEAT_H
