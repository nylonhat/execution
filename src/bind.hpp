#ifndef BIND_H
#define BIND_H

#include "concepts.hpp"

namespace ex::algorithms::bind {

	template<IsReceiver NextReceiver, class MonadicFunction>
	struct Receiver {
		[[no_unique_address]] NextReceiver next_receiver;
		[[no_unique_address]] MonadicFunction monadic_function;

		template<IsOpState... Cont, class... Args>
		auto set_value(Cont&... cont, Args... args){
			using Sender2 = std::invoke_result_t<MonadicFunction, Args...>;
			using Op2 = connect_t<Sender2, NextReceiver>;
		
			Sender2 sender2 = std::invoke(monadic_function, args...);
			Op2* op2_ptr = reinterpret_cast<Op2*>(this);
			Op2& op2 = *new (op2_ptr) Op2 (ex::connect(sender2, next_receiver));
			ex::start(op2, cont...);
		}
	};


	template<IsSender Sender1, class MonadicFunction, IsReceiver SuffixReceiver>
	struct OpState {
		using InfixReceiver = Receiver<SuffixReceiver, MonadicFunction>;
		using Op1 = connect_t<Sender1, InfixReceiver>;

		using Sender2 = apply_values_t<MonadicFunction, Sender1>; 
		using Op2 = connect_t<Sender2, SuffixReceiver>;

		union {
			Op1 op1;
			Op2 op2;
		};

		OpState(Sender1 sender1, MonadicFunction monadic_function, SuffixReceiver suffix_receiver)
			: op1{ex::connect(sender1, InfixReceiver{suffix_receiver, monadic_function})}
		{}

		auto start(IsOpState auto&... cont){
			ex::start(op1, cont...);
		}
	};

	template<class MonadicFunction, IsSender Sender1>
	struct Sender {
		using value_t = apply_values_t<MonadicFunction, Sender1>::value_t;

		[[no_unique_address]] Sender1 sender1;
		[[no_unique_address]] MonadicFunction monadic_function;

		auto connect(auto suffix_receiver){
			return OpState{sender1, monadic_function, suffix_receiver};
		}
	};

}//namespace ex::algorithms::bind

namespace ex {

	inline constexpr auto bind = [](IsSender auto sender, auto mfunc){
		return algorithms::bind::Sender{sender, mfunc};
	};

}//namespace ex


auto operator >= (ex::IsSender auto sender, auto mfunc){
	return ex::bind(sender, mfunc);
}

#endif//BIND_H
