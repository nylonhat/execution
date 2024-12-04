#ifndef REPEAT_H
#define REPEAT_H

#include "concepts.hpp"
#include "inline.hpp"

namespace ex::algorithms::repeat2 {

	template<Channel channel, IsReceiver SuffixReceiver, IsSender ChildSender1, class Predicate, class MonadicFunction>
	struct OpState 
		: InlinedReceiver<OpState<channel, SuffixReceiver, ChildSender1, Predicate, MonadicFunction>, SuffixReceiver>
		, ManualChildOp<OpState<channel, SuffixReceiver, ChildSender1, Predicate, MonadicFunction>, 0, ChildSender1, apply_values_t<MonadicFunction, ChildSender1>>
	{

		using OpStateOptIn = ex::OpStateOptIn;
		using Receiver = InlinedReceiver<OpState, SuffixReceiver>;
		using ChildOps = ManualChildOp<OpState, 0, ChildSender1, apply_values_t<MonadicFunction, ChildSender1>>; 

		[[no_unique_address]] Predicate predicate;
		[[no_unique_address]] MonadicFunction monadic_function;
		
		OpState(SuffixReceiver suffix_receiver, ChildSender1 child_sender1, Predicate predicate, MonadicFunction monadic_function)
			: Receiver{suffix_receiver}
			, ChildOps{child_sender1} 
			, predicate{predicate}
			, monadic_function{monadic_function}
		{}
	
		template<class... Cont>
		void start(Cont&... cont){
			return ChildOps::template start<0>(cont...);
		}

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
        void set_value(Cont&... cont, Arg... args){
        	if constexpr(channel == Channel::value){
	        	if(predicate(args...)){
	        		auto child_sender2 = monadic_function(args...);
	        		auto& child_op2 = ChildOps::template construct_from<1>(child_sender2);
	        		return ex::start(child_op2, cont...);
	        	}
        	}

			return ex::set_value.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., args...);
        }

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
        auto set_error(Cont&... cont, Arg... args){
			if constexpr(channel == Channel::error){
	        	if(predicate(args...)){
	        		auto child_sender2 = monadic_function(args...);
	        		auto& child_op2 = ChildOps::template construct_from<1>(child_sender2);
	        		return ex::start(child_op2, cont...);
	        	}
        	}
        	
			return ex::set_error.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., args...);
        }
	};

	template<Channel channel, IsSender ChildSender1, class Predicate, class MonadicFunction>
	struct Sender {
		using SenderOptIn = ex::SenderOptIn;
		using value_t = apply_values_t<MonadicFunction, ChildSender1>::value_t;

		ChildSender1 child_sender1;
		Predicate predicate;
		MonadicFunction monadic_function;

		template<IsReceiver SuffixReceiver>
		auto connect(SuffixReceiver suffix_receiver){
			return OpState<channel, SuffixReceiver, ChildSender1, Predicate, MonadicFunction>{suffix_receiver, child_sender1, predicate, monadic_function};
		}
	};


}//namespace ex::algorithms::repeat


namespace ex {

	inline constexpr auto repeat2 = []<IsSender Sender1, class Predicate, class MonadicFunction>(Sender1 sender1, Predicate predicate, MonadicFunction  monadic_function){
		//explicit template to prevent copy constructor ambiguity
		return algorithms::repeat2::Sender<Channel::value, Sender1, Predicate, MonadicFunction>{sender1, predicate, monadic_function};
	};

}//namespace ex


#endif//REPEAT_H
