#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "timer.h"
#include "sender.h"

template<class BaseOp>
struct BenchmarkRecvr {

	template<class... Cont>
	void set_value(Cont&... cont, auto count){
		auto* byte_p = reinterpret_cast<std::byte*>(this) - offsetof(BaseOp, op);
		auto& base_op = *reinterpret_cast<BaseOp*>(byte_p);

		base_op.timer.stop();

		return ::set_value.operator()<typename BaseOp::ER, Cont...>(base_op.end_recvr, cont..., base_op.timer.count()/count);
	}
};

template<class EndRecvr, Sender S>
struct BenchmarkOp {
	using Self = BenchmarkOp<EndRecvr, S>;
	using Recvr = BenchmarkRecvr<Self>;
	using Op = connect_t<S, Recvr>;
	using ER = EndRecvr;
	
	[[no_unique_address]] EndRecvr end_recvr;

	union{
		Op op;
	};

	Timer timer = {};
	
	BenchmarkOp(S sender, ER end_recvr)
		: end_recvr{end_recvr}
		, op{::connect(sender, Recvr{})}
	{}
	
	auto start(auto&... cont){
		timer.start();
		return ::start(op, cont...);
	}

};

template<Sender S>
struct BenchmarkSender {
	using value_t = std::tuple<std::size_t>;
	S sender;

	auto connect(auto end_recvr){
		return BenchmarkOp{sender, end_recvr};
	}
};

auto benchmark = []<Sender S>(S sender){
	//explicit template to prevent copy constructor ambiguity
	return BenchmarkSender<S>{sender};
};

#endif//REPEAT_H
