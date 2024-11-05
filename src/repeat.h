#ifndef REPEAT_H
#define REPEAT_H

#include "timer.h"
#include "sender.h"
#include <iostream>

template<typename RO>
struct RepeatRecvr {
	void set_value(auto&... cont, auto... args){
		auto* byte_p = reinterpret_cast<std::byte*>(this) - offsetof(RO, op);
		auto& repeat_op = *reinterpret_cast<RO*>(byte_p);

		return ::start(repeat_op, cont...);
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
	Timer timer = {};
	static constexpr size_t max = 100'000'000;

	RepeatOp(S sender, ER end_recvr)
		: end_recvr{end_recvr}
		, sender{sender}
		{timer.start();}
	
	template<class... Cont>
	auto start(Cont&... cont){
		if(count < max){
			count++;
			new (&op) OP (::connect(sender, RR{}));
			return ::start(op, cont...);
		}
		timer.stop();
		return ::set_value.operator()<ER, Cont...>(end_recvr, cont..., timer.count()/max);
	}

};

template<Sender S>
struct RepeatSender {
	using value_t = std::tuple<std::size_t>;
	S sender;

	auto connect(auto end_recvr){
		return RepeatOp{sender, end_recvr};
	}
};

auto repeat = []<Sender S>(S sender){
	//explicit template to prevent copy constructor ambiguity
	return RepeatSender<S>{sender};
};


#endif//REPEAT_H
