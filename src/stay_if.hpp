#ifndef STAY_IF_H
#define STAY_IF_H

#include "concepts.hpp"

namespace ex::algorithms::stay_if {

	template<IsReceiver NextReceiver, class Predicate>
	struct Receiver {
		[[no_unique_address]] NextReceiver next_receiver;
	    [[no_unique_address]] Predicate predicate;

		template<IsOpState... Cont>
		void set_value(Cont&... cont, auto... args){
			if(std::invoke(predicate, args...)){
				return ex::set_value.operator()<NextReceiver, Cont...>
				(next_receiver, cont..., args...);
			}

			return ex::set_error.operator()<NextReceiver, Cont...>
			(next_receiver, cont..., args...);
		}

		
		template<IsOpState... Cont>
		void set_error(Cont&... cont, auto... args){
			if(std::invoke(predicate, args...)){
				return ex::set_error.operator()<NextReceiver, Cont...>
				(next_receiver, cont..., args...);
			}
			
			return ex::set_value.operator()<NextReceiver, Cont...>
			(next_receiver, cont..., args...);
		}
	};        


	template<IsSender ChildSender, class Predicate>
	struct Sender {
		using value_t = ChildSender::value_t;
		using error_t = ChildSender::error_t;
		
	    [[no_unique_address]] ChildSender child_sender;
	    [[no_unique_address]] Predicate predicate;

	    auto connect(auto suffix_receiver){
	        auto infix_receiver = Receiver{suffix_receiver, predicate};
	        return ex::connect(child_sender, infix_receiver);
	    }

	};

	struct Function {
		auto operator()(this auto&&, IsSender auto child_sender, auto predicate){
			return Sender{child_sender, predicate};
		}

		auto operator()(this auto&&, auto predicate){
			return [=](IsSender auto child_sender){
				return Sender{child_sender, predicate};
			};
		}
	};

}//namespace map

namespace ex {

	inline constexpr auto stay_if = algorithms::stay_if::Function{};

}//namespace ex


#endif//STAY_IF_H
