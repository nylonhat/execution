#ifndef BIND_H
#define BIND_H

#include "sender.h"

template<class R, class F>
struct BindRecvr {
	[[no_unique_address]] R end_recvr;
	[[no_unique_address]] F mfunc;

	template<class... Args>
	auto set_value(Args... args){
		using S2 = std::invoke_result_t<F, Args...>;
		using OP2 = connect_t<S2, R>;
		
		S2 sender2 = std::invoke(mfunc, args...);
		OP2* op2_p = reinterpret_cast<OP2*>(this);
		OP2& op2 = *new (op2_p) OP2 (::connect(sender2, end_recvr));
		::start(op2);
	}
};


template<Sender S1, class F, class R>
struct BindOp {
	using BR = BindRecvr<R, F>;
	using OP1 = connect_t<S1, BR>;

	using S2 = apply_values_t<F, S1>; 
	using OP2 = connect_t<S2, R>;

	union {
		OP1 op1;
		OP2 op2;
	};

	BindOp(){}

	BindOp(S1 sender1, F mfunc, R end_recvr)
		: op1{::connect(sender1, BR{end_recvr, mfunc})}
	{}

	auto start(){
		::start(op1);
	}
};

template<class F, Sender S>
struct BindSender {
	using value_t = apply_values_t<F, S>::value_t;

	[[no_unique_address]] S sender1;
	[[no_unique_address]] F mfunc;

	auto connect(auto end_recvr){
		return BindOp{sender1, mfunc, end_recvr};
	}
};

auto bind = [](Sender auto sender, auto mfunc){
	return BindSender{sender, mfunc};
};


auto operator >= (Sender auto sender, auto mfunc){
	return bind(sender, mfunc);
}

#endif//BIND_H
