#ifndef BIND_H
#define BIND_H

#include "concepts.hpp"
#include "inline.hpp"

namespace ex::algorithms::bind {
		
	template<Channel channel, IsReceiver SuffixReceiver, IsSender Sender1, class MonadicFunction>
	struct OpState 
		: InlinedReceiver<OpState<channel, SuffixReceiver, Sender1, MonadicFunction>, SuffixReceiver>
		, ManualChildOp<OpState<channel, SuffixReceiver, Sender1, MonadicFunction>, 0, Sender1, apply_values_t<MonadicFunction, Sender1>>
	{

		using OpStateOptIn = ex::OpStateOptIn;
		using Sender2 = apply_values_t<MonadicFunction, Sender1>;
		using Receiver = InlinedReceiver<OpState, SuffixReceiver>;
		using ChildOps = ManualChildOp<OpState, 0, Sender1, Sender2>;

		[[no_unique_address]] MonadicFunction monadic_function;
		
		OpState(SuffixReceiver suffix_receiver, Sender1 sender1, MonadicFunction monadic_function)
			: Receiver{suffix_receiver}
			, ChildOps{sender1}
			, monadic_function{monadic_function}
		{}

		auto start(IsOpState auto&... cont){
			return ChildOps::template start<0>(cont...);
		}

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
			requires (VariantIndex == 0)
        auto set_value(Cont&... cont, Arg... args){
			if constexpr(channel == Channel::value){
				ChildOps::template construct_from<1>(monadic_function(args...));
				return ChildOps::template start<1>(cont...);
			} else if (channel == Channel::error){
				return ex::set_value.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., args...);			
			}
        }

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
			requires (VariantIndex == 0)
        auto set_error(Cont&... cont, Arg... args){
			if constexpr(channel == Channel::value){
				return ex::set_error.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., args...);			
			} else if (channel == Channel::error){
				ChildOps::template construct_from<1>(monadic_function(args...));
				return ChildOps::template start<1>(cont...);
			}
        }

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
			requires (VariantIndex == 1)
        auto set_value(Cont&... cont, Arg... args){
			return ex::set_value.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., args...);			
        }

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
			requires (VariantIndex == 1)
        auto set_error(Cont&... cont, Arg... args){
			return ex::set_error.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., args...);			
        }
		
	};

	template<Channel channel, IsSender Sender1, class MonadicFunction>
	struct Sender {
		using SenderOptIn = ex::SenderOptIn;
		using value_t = apply_values_t<MonadicFunction, Sender1>::value_t;
		//using error_t = apply_errors_t<MonadicFunction, Sender1>::error_t;

		[[no_unique_address]] Sender1 sender1;
		[[no_unique_address]] MonadicFunction monadic_function;

		constexpr Sender(Sender1 sender1, MonadicFunction monadic_function)
			: sender1{sender1}
			, monadic_function{monadic_function}
		{}
		
		template<IsReceiver SuffixReceiver>
		constexpr auto connect(SuffixReceiver suffix_receiver){
			return OpState<channel, SuffixReceiver, Sender1, MonadicFunction>{suffix_receiver, sender1, monadic_function};
		}
	};

	template<Channel channel>
	struct Function {
		template<IsSender Sender1, class MonadicFunction>
		constexpr auto operator()(this auto&&, Sender1 sender1, MonadicFunction monadic_function){
			return Sender<channel, Sender1, MonadicFunction>{sender1, monadic_function};
		}

		template<class MonadicFunction>
		constexpr auto operator()(this auto&&, MonadicFunction monadic_function){
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
