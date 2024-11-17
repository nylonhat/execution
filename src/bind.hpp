#ifndef BIND_H
#define BIND_H

#include "concepts.hpp"

namespace ex::algorithms::bind {
	
	template<Channel channel, IsReceiver NextReceiver, class MonadicFunction>
	struct Receiver {
		[[no_unique_address]] NextReceiver next_receiver;
		[[no_unique_address]] MonadicFunction monadic_function;

		template<IsOpState... Cont, class... Args>
		auto set_value(Cont&... cont, Args... args){
			if constexpr(channel == Channel::value){
				//return start_next<Cont...>(cont..., args...);

			using Sender2 = std::invoke_result_t<MonadicFunction, Args...>;
			using Op2 = connect_t<Sender2, NextReceiver>;
		
			Sender2 sender2 = std::invoke(monadic_function, args...);
			Op2* op2_ptr = reinterpret_cast<Op2*>(this);
			Op2& op2 = *new (op2_ptr) Op2 (ex::connect(sender2, next_receiver));
			return ex::start(op2, cont...);
			} else if (channel == Channel::error){
				return ex::set_value.operator()<NextReceiver, Cont...>(next_receiver, cont..., args...);			
			}
		}

		template<IsOpState... Cont, class... Args>
		auto set_error(Cont&... cont, Args... args){
			if constexpr(channel == Channel::value){
				return ex::set_error.operator()<NextReceiver, Cont...>(next_receiver, cont..., args...);
			} else if(channel == Channel::error){
				return start_next<Cont...>(cont..., args...);
			}
		}

		template<IsOpState... Cont, class... Args>
		auto start_next(Cont&... cont, Args... args){
			using Sender2 = std::invoke_result_t<MonadicFunction, Args...>;
			using Op2 = connect_t<Sender2, NextReceiver>;
		
			Sender2 sender2 = std::invoke(monadic_function, args...);
			Op2* op2_ptr = reinterpret_cast<Op2*>(this);
			Op2& op2 = *new (op2_ptr) Op2 (ex::connect(sender2, next_receiver));
			return ex::start(op2, cont...);
		}
	};
	
	template<Channel channel, IsSender Sender1, class MonadicFunction, IsReceiver SuffixReceiver>
	struct OpState {
		using InfixReceiver = Receiver<channel, SuffixReceiver, MonadicFunction>;
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
			return ex::start(op1, cont...);
		}
	};

	template<Channel channel, IsSender Sender1, class MonadicFunction>
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
			return OpState<channel, Sender1, MonadicFunction, SuffixReceiver>{sender1, monadic_function, suffix_receiver};
		}
	};

	template<Channel channel>
	struct Function {
		template<IsSender Sender1, class MonadicFunction>
		auto operator()(this auto&&, Sender1 sender1, MonadicFunction monadic_function){
			return Sender<channel, Sender1, MonadicFunction>{sender1, monadic_function};
		}

		template<class MonadicFunction>
		auto operator()(this auto&&, MonadicFunction monadic_function){
			return [=]<IsSender Sender1>(Sender1 sender1){
				return Sender<channel, Sender1, MonadicFunction>{sender1, monadic_function};
			};
		}
	};

}//namespace ex::algorithms::bind

namespace ex {

	inline constexpr auto bind_value = algorithms::bind::Function<Channel::value>{};
	inline constexpr auto bind_error = algorithms::bind::Function<Channel::error>{};

}//namespace ex


auto operator >= (ex::IsSender auto sender, auto mfunc){
	return ex::bind_value(sender, mfunc);
}

#endif//BIND_H
