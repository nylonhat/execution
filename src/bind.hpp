#ifndef BIND_H
#define BIND_H

#include "concepts.hpp"

namespace ex::algorithms::bind {
	
	template<IsReceiver NextReceiver, class MonadicFunction>
	struct ValueReceiver {
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

		
		template<IsOpState... Cont, class... Args>
		auto set_error(Cont&... cont, Args... args){
			ex::set_error.operator()<NextReceiver, Cont...>(
				next_receiver,
				cont...,
				args...                                       	
			);
		}
	};

	
	template<IsReceiver NextReceiver, class MonadicFunction>
	struct ErrorReceiver {
		[[no_unique_address]] NextReceiver next_receiver;
		[[no_unique_address]] MonadicFunction monadic_function;

		template<IsOpState... Cont, class... Args>
		auto set_value(Cont&... cont, Args... args){
			ex::set_value.operator()<NextReceiver, Cont...>(
				next_receiver,
				cont...,
				args...                                       	
			);			
		}

		template<IsOpState... Cont, class... Args>
		auto set_error(Cont&... cont, Args... args){
			using Sender2 = std::invoke_result_t<MonadicFunction, Args...>;
			using Op2 = connect_t<Sender2, NextReceiver>;
		
			Sender2 sender2 = std::invoke(monadic_function, args...);
			Op2* op2_ptr = reinterpret_cast<Op2*>(this);
			Op2& op2 = *new (op2_ptr) Op2 (ex::connect(sender2, next_receiver));
			ex::start(op2, cont...);
		}
	};


	template<template<class,class> class Channel, IsSender Sender1, class MonadicFunction, IsReceiver SuffixReceiver>
	struct OpState {
		using InfixReceiver = Channel<SuffixReceiver, MonadicFunction>;
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


	template<template<class,class> class Channel, IsSender Sender1, class MonadicFunction>
	struct Sender {
		
		using value_t = apply_values_t<MonadicFunction, Sender1>::value_t;
		//using error_t = apply_errors_t<MonadicFunction, Sender1>::error_t;


		[[no_unique_address]] Sender1 sender1;
		[[no_unique_address]] MonadicFunction monadic_function;

		Sender(Sender1 sender1, MonadicFunction monadic_function)
			: sender1{sender1}
			, monadic_function{monadic_function}
		{}
		
		template<IsReceiver SuffixReceiver>
		auto connect(SuffixReceiver suffix_receiver){
			return OpState<Channel, Sender1, MonadicFunction, SuffixReceiver>{sender1, monadic_function, suffix_receiver};
		}
	};

	template<template<class, class> class Channel>
	struct Function {
		template<IsSender Sender1, class MonadicFunction>
		auto operator()(this auto&&, Sender1 sender1, MonadicFunction monadic_function){
			return Sender<Channel, Sender1, MonadicFunction>{sender1, monadic_function};
		}

		
		template<class MonadicFunction>
		auto operator()(this auto&&, MonadicFunction monadic_function){
			return [=]<IsSender Sender1>(Sender1 sender1){
				return Sender<Channel, Sender1, MonadicFunction>{sender1, monadic_function};
			};
		}
	};

	using ValueFunction = Function<ValueReceiver>;
	using ErrorFunction = Function<ErrorReceiver>;
	
}//namespace ex::algorithms::bind

namespace ex {

	inline constexpr auto bind_value = algorithms::bind::ValueFunction{};
	inline constexpr auto bind_error = algorithms::bind::ErrorFunction{};

}//namespace ex


auto operator >= (ex::IsSender auto sender, auto mfunc){
	return ex::bind_value(sender, mfunc);
}

#endif//BIND_H
