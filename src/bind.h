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
		//flag->release();
	}
};

auto sync_start = []<class S>(S sender){
	using T = S::value_t;
	T value;
	std::binary_semaphore flag{0};
	auto op = connect(sender, sync_recvr{&value, &flag});
	start(op);
	//flag.acquire();
	return value;
};


template<class T, class R>
struct pure_op {
    [[no_unique_address]] R recvr;
	 T value;


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
		return pure_op{ recvr, value};
	}
};

auto pure = [](auto value){
	return pure_sender{value};
};

template<class F, class R>
struct bind_recvr {
    [[no_unique_address]] R end_recvr;
	[[no_unique_address]] F mfunc;

	template<class T>
	auto set_value(T value){
        using S2 = std::invoke_result_t<F, T>;
	    using Op2 = std::invoke_result_t<decltype(connect), S2, R>;
		
        auto sender2 = std::invoke(mfunc, value);
        auto& op2 = *reinterpret_cast<Op2*>(this);
		op2 = connect(sender2, end_recvr);
		start(op2);
	}
};


template<class S, class F, class R>
struct bind_op {
	using S2 = std::invoke_result_t<F, typename S::value_t>;
	using Op2 = std::invoke_result_t<decltype(connect), S2, R>;
	using BR = bind_recvr<F, R>;
	using Op1 = std::invoke_result_t<decltype(connect), S, BR>;

	struct Param {
        [[no_unique_address]] S sender1 = {};
		[[no_unique_address]] F mfunc = {};
		[[no_unique_address]] R end_recvr = {};
	};

	union {
		Param init = {};
		Op1 op1;
		Op2 op2;
	};

    bind_op(){}

	bind_op(S sender1, F mfunc, R end_recvr)
		: init{sender1, mfunc, end_recvr}
	{}

	auto start(){
		auto recvr = BR{init.end_recvr, init.mfunc};
		op1 = connect(init.sender1, recvr);
		::start(op1);
	}
};

template<class F, class S>
struct bind_sender {
	using value_t = std::invoke_result_t<F, typename S::value_t>::value_t;

	[[no_unique_address]] S sender1;
    [[no_unique_address]] F mfunc;

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
	return func(value);
}

#endif//BIND_H
