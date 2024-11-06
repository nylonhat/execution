#ifndef BIND_H
#define BIND_H

#include "sender.hpp"

namespace ex::algorithms::bind {

	template<class R, class F>
	struct BindRecvr {
		[[no_unique_address]] R end_recvr;
		[[no_unique_address]] F mfunc;

		template<class... Cont, class... Args>
		auto set_value(Cont&... cont, Args... args){
			using S2 = std::invoke_result_t<F, Args...>;
			using OP2 = connect_t<S2, R>;
		
			S2 sender2 = std::invoke(mfunc, args...);
			OP2* op2_p = reinterpret_cast<OP2*>(this);
			OP2& op2 = *new (op2_p) OP2 (ex::connect(sender2, end_recvr));
			ex::start(op2, cont...);
		}
	};


	template<ex::Sender S1, class F, class R>
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
			: op1{ex::connect(sender1, BR{end_recvr, mfunc})}
		{}

		auto start(auto&... cont){
			ex::start(op1, cont...);
		}
	};

	template<class F, ex::Sender S>
	struct BindSender {
		using value_t = apply_values_t<F, S>::value_t;

		[[no_unique_address]] S sender1;
		[[no_unique_address]] F mfunc;

		auto connect(auto end_recvr){
			return BindOp{sender1, mfunc, end_recvr};
		}
	};

}//namespace ex::algorithms::bind

namespace ex {

	inline constexpr auto bind = [](Sender auto sender, auto mfunc){
		return algorithms::bind::BindSender{sender, mfunc};
	};

}//namespace ex


auto operator >= (ex::Sender auto sender, auto mfunc){
	return ex::bind(sender, mfunc);
}

#endif//BIND_H
