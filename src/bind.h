#ifndef BIND_H
#define BIND_H

#include <functional>
#include <semaphore>

auto start = [](auto& op){
	op.start();
};

auto connect = [](auto sender, auto recvr){
	return sender.connect(recvr);
};

template<class T>
struct sync_recvr {
	T* value;	
	std::binary_semaphore* flag;

	auto set_value(auto v){
		*value = v;
		flag->release();
	}
};

auto sync_start = []<class S>(S sender){
	using T = S::value_t;
	T value;
	std::binary_semaphore flag{0};
	auto op = connect(sender, sync_recvr{&value, &flag});
	start(op);
	flag.acquire();
	return value;
};


template<class T, class R>
struct pure_op {
	T value;
	R recvr;

	auto start(){
		recvr.set_value(value);
	}
};

template<class T>
struct pure_sender {
	using value_t = T;
	T value;

	template<class R>
	auto connect(R recvr){
		return pure_op{value, recvr};
	}
};

auto pure = [](auto value){
	return pure_sender{value};
};

template<class F, class R, class Op2>
struct bind_recvr {
	F mfunc;
	R end_recvr;
	Op2* op2;

	auto set_value(auto value){
		auto sender2 = std::invoke(mfunc, value);
		*op2 = connect(sender2, end_recvr);
		start(*op2);
	}
};

template<class S, class F, class R>
struct bind_op {
	using S2 = std::invoke_result_t<F, typename S::value_t>;
	using Op2 = std::invoke_result_t<decltype(connect), S2, R>;
	using BR = bind_recvr<F, R, Op2>;
	using Op1 = std::invoke_result_t<decltype(connect), S, BR>;

	struct Param {
		S sender1 = {};
		F mfunc = {};
		R end_recvr = {};
	};

	union {	
		Param init = {};
		Op1 op1;
		Op2 op2;
	};

	bind_op(S sender1, F mfunc, R end_recvr)
		: init{sender1, mfunc, end_recvr}
	{}

	auto start(){
		auto recvr = bind_recvr{init.mfunc, init.end_recvr, &op2};
		op1 = connect(init.sender1, recvr);
		::start(op1);
	}
};

template<class S, class F>
struct bind_sender {
	using value_t = std::invoke_result_t<F, typename S::value_t>::value_t;

	S sender1;
	F mfunc;

	template<class R>
	auto connect(R end_recvr){
		return bind_op{sender1, mfunc, end_recvr};
	}
};

auto bind = [](auto sender, auto mfunc){
	return bind_sender{sender, mfunc};
};

auto operator >> (auto sender, auto mfunc){
	return bind(sender, mfunc);
}

auto operator | (auto value, auto func){
	return std::invoke(func, value);
}

#endif//BIND_H
