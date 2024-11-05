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
	const size_t max = 100'000'000;

	RepeatOp(S sender, ER end_recvr, size_t iterations)
		: end_recvr{end_recvr}
		, sender{sender}
		, max{iterations}
	{}
	
	template<class... Cont>
	auto start(Cont&... cont){
		if(count < max){
			count++;
			new (&op) OP (::connect(sender, RR{}));
			return ::start(op, cont...);
		}

		return ::set_value.operator()<ER, Cont...>(end_recvr, cont..., count);
	}

};

template<Sender S>
struct RepeatSender {
	using value_t = std::tuple<std::size_t>;
	S sender;
	size_t iterations;

	auto connect(auto end_recvr){
		return RepeatOp{sender, end_recvr, iterations};
	}
};

auto repeat = []<Sender S>(S sender, size_t iterations){
	//explicit template to prevent copy constructor ambiguity
	return RepeatSender<S>{sender, iterations};
};

auto repeat_n = [](size_t iterations){
	return [=]<Sender S>(S sender){
		return RepeatSender<S>{sender, iterations};
	};
};


#endif//REPEAT_H
