#ifndef STAY_IF_H
#define STAY_IF_H

#include "concepts.hpp"

namespace ex::algorithms::stay_if {

	template<class SuffixReceiver, class Predicate, class ChildSender>
	struct OpState 
		: InlinedReceiver<OpState<SuffixReceiver, Predicate, ChildSender>, SuffixReceiver>
		, ManualChildOp<OpState<SuffixReceiver, Predicate, ChildSender>, 0, ChildSender>
	{
		
		using Receiver = InlinedReceiver<OpState, SuffixReceiver>;
		using ChildOp =  ManualChildOp<OpState, 0, ChildSender>;

		Predicate predicate;

		OpState(SuffixReceiver suffix_receiver, Predicate predicate, ChildSender child_sender)
			: Receiver{suffix_receiver}
			, ChildOp{child_sender}
			, predicate{predicate}
		{}

		template<class... Cont>
		auto start(Cont&... cont){
			return ChildOp::template start<0>(cont...);
		}

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
        auto set_value(Cont&... cont, Arg... arg){
        	if(std::invoke(predicate, arg...)){
        		return ex::set_value.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., arg...);
        	}
    		return ex::set_error.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., arg...);
        }

        
		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
        auto set_error(Cont&... cont, Arg... arg){
        	if(std::invoke(predicate, arg...)){
        		return ex::set_error.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., arg...);
        	}
    		return ex::set_value.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., arg...);
        }
		
		
	};     


	template<IsSender ChildSender, class Predicate>
	struct Sender {
		using value_t = ChildSender::value_t;
		using error_t = ChildSender::value_t;
		
	    [[no_unique_address]] ChildSender child_sender;
	    [[no_unique_address]] Predicate predicate;

	    auto connect(auto suffix_receiver){
	    	return OpState{suffix_receiver, predicate, child_sender};
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
