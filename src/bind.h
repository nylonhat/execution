#ifndef BIND_H
#define BIND_H

#include <functional>
#include <semaphore>
#include <print>


struct noop_recvr {	
	auto set_value(auto... args){}
};


template<typename S>
concept Sender = requires(S sender, noop_recvr recvr){
	sender.connect(recvr);
};



template<typename F, typename Tuple>
using apply_result_t = std::invoke_result_t<decltype(&std::apply<F, Tuple>), F, Tuple>;	

template<typename... Tuples>
using tuple_cat_result_t =  std::invoke_result_t<decltype(&std::tuple_cat<Tuples...>), Tuples...>;	

template<typename S>
using tuple_t = std::tuple_element_t<0, typename S::value_t>;

auto start = [](auto& op){
	op.start();
};

auto connect = [](Sender auto sender, auto recvr){
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
	using T = tuple_t<S>;
	T value;
	std::binary_semaphore flag{0};
	auto op = ::connect(sender, sync_recvr{&value, &flag});
	//std::println("op size: {} bytes", sizeof(op));
	start(op);
	flag.acquire();
	return value;
};


auto set_value = [](auto recvr, auto... args){
	recvr.set_value(args...);
};

template<class R, class T>
struct pure_op {
    [[no_unique_address]] R recvr;
	T value;

	auto start(){
		recvr.set_value(value);
	}
};

template<class T>
struct pure_sender {
	using value_t = std::tuple<T>;
	T value;

	template<class R>
	auto connect(R recvr){
		return pure_op{recvr, value};
	}
};

auto pure = [](auto value){
	return pure_sender{value};
};

template<class R, class A, class B>
struct pure2_op {
    [[no_unique_address]] R recvr;
	A a;
	B b;

	auto start(){
		recvr.set_value(a, b);
	}
};

template<class A, class B>
struct pure2_sender {
	using value_t = std::tuple<A, B>;
	A a;
	B b;

	template<class R>
	auto connect(R recvr){
		return pure2_op{recvr, a, b};
	}
};

auto pure2 = [](auto a, auto b){
	return pure2_sender{a, b};
};


template<class F, class R>
struct bind_recvr {
    [[no_unique_address]] R end_recvr;
	[[no_unique_address]] F mfunc;

	template<class... Args>
	auto set_value(Args... args){
        using S2 = std::invoke_result_t<F, Args...>;
	    using Op2 = std::invoke_result_t<decltype(connect), S2, R>;
		
        S2 sender2 = std::invoke(mfunc, args...);
        Op2& op2 = *reinterpret_cast<Op2*>(this);
		op2 = connect(sender2, end_recvr);
		start(op2);
	}
};


template<class S1, class F, class R>
struct bind_op {
	using BR = bind_recvr<F, R>;
	using Op1 = std::invoke_result<decltype(::connect), S1, BR>::type;

	using S2 = apply_result_t<F, typename S1::value_t>; 
	using Op2 = std::invoke_result_t<decltype(::connect), S2, R>;

	struct Param {
        [[no_unique_address]] S1 sender1 = {};
		[[no_unique_address]] F mfunc = {};
		[[no_unique_address]] R end_recvr = {};
	};

	union {
		Param init = {};
		Op1 op1;
		Op2 op2;
	};

    bind_op(){}

	bind_op(S1 sender1, F mfunc, R end_recvr)
		: init{sender1, mfunc, end_recvr}
	{}

	auto start(){
		BR recvr = BR{init.end_recvr, init.mfunc};
		op1 = ::connect(init.sender1, recvr);
		::start(op1);
	}
};

template<class F, class S>
struct bind_sender {
	using value_t = apply_result_t<F, typename S::value_t>::value_t;

	[[no_unique_address]] S sender1;
    [[no_unique_address]] F mfunc;

    //template<class R>
	auto connect(auto end_recvr){
		return bind_op{sender1, mfunc, end_recvr};
	}
};


template<class ER, class F>
struct map_recvr {
	[[no_unique_address]] ER end_recvr;
	[[no_unique_address]] F func;
	
	auto set_value(auto... args){
		end_recvr.set_value(func(args...));
	}
};	


template<class S, class F>
struct map_sender {
	using value_t = std::tuple< apply_result_t<F, typename S::value_t> >;
	[[no_unique_address]] S sender1;
    [[no_unique_address]] F func;
	
	//template<class R>
	auto connect(auto end_recvr){
		return ::connect(sender1, map_recvr{end_recvr, func});
	}

};




auto map = [](Sender auto sender, auto func){
	return map_sender{sender, func};
};


auto bind = [](Sender auto sender, auto mfunc){
	return bind_sender{sender, mfunc};
};

auto operator > (Sender auto sender, auto func){
	return map(sender, func);
}

auto operator >> (Sender auto sender, auto mfunc){
	return bind(sender, mfunc);
}

auto operator | (Sender auto value, auto func){
	return func(value);
}

#endif//BIND_H
