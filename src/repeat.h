#ifndef REPEAT_H
#define REPEAT_H


#include "sender.h"

template<typename RO>
struct RepeatRecvr {

	void set_value(auto... args){
		auto* byte_p = reinterpret_cast<std::byte*>(this) - offsetof(RO, op);
		auto& repeat_op = *reinterpret_cast<RO*>(byte_p);
	
		::start(repeat_op);
	}
};

template<typename ER, Sender S>
struct RepeatOp {
	using RO = RepeatOp<ER, S>;
	using RR = RepeatRecvr<RO>;
	using OP = connect_t<S, RR>;
	
	[[no_unique_address]] ER end_recvr;

	union{
		OP op;
	};
	
	[[no_unique_address]] S sender;
	std::size_t count = 0;

	RepeatOp(S sender, ER end_recvr)
		: end_recvr{end_recvr}
		, sender{sender}
	{}
	
	auto start(){

		if(count < 1'000'000){
			count++;
			new (&op) OP (::connect(sender, RR{}));
			return ::start(op);
		}
		return end_recvr.set_value(count);
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
