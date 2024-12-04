#ifndef STAY_IF_H
#define STAY_IF_H

#include "concepts.hpp"

namespace ex::algorithms::channel_else {

	template<Channel channel, class SuffixReceiver, class Predicate, class ChildSender>
	struct OpState 
		: InlinedReceiver<OpState<channel, SuffixReceiver, Predicate, ChildSender>, SuffixReceiver>
		, ManualChildOp<OpState<channel, SuffixReceiver, Predicate, ChildSender>, 0, ChildSender>
	{
		
		using OpStateOptIn = ex::OpStateOptIn;
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
        	if constexpr(channel == Channel::value){
	        	if(!std::invoke(predicate, arg...)){
	        		return ex::set_error.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., arg...);
	        	}
        	}
    		return ex::set_value.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., arg...);
        }

        
		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
        auto set_error(Cont&... cont, Arg... arg){
        	if constexpr(channel == Channel::error){
	        	if(!std::invoke(predicate, arg...)){
	        		return ex::set_value.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., arg...);
	        	}
        	}
    		return ex::set_error.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., arg...);
        }
		
	};     


	template<Channel channel, class Predicate, IsSender ChildSender>
	struct Sender {
		using SenderOptIn = ex::SenderOptIn;
		using value_t = ChildSender::value_t;
		using error_t = ChildSender::value_t;
		
	    [[no_unique_address]] Predicate predicate;
	    [[no_unique_address]] ChildSender child_sender;

		template<IsReceiver SuffixReceiver>
	    auto connect(SuffixReceiver suffix_receiver){
	    	return OpState<channel, SuffixReceiver, Predicate, ChildSender>{suffix_receiver, predicate, child_sender};
	    }

	};

	template<Channel channel>
	struct FunctionObject {

		template<class Predicate, IsSender ChildSender>
		static auto operator()(ChildSender child_sender, Predicate predicate){
			return Sender<channel, Predicate, ChildSender>{predicate, child_sender};
		}

		template<class Predicate>
		static auto operator()(Predicate predicate){
			return [=]<IsSender ChildSender>(ChildSender child_sender){
				return Sender<channel, Predicate, ChildSender>{predicate, child_sender};
			};
		}
	};

}//namespace map

namespace ex {

	inline constexpr auto value_else_error = algorithms::channel_else::FunctionObject<Channel::value>{};
	inline constexpr auto error_else_value = algorithms::channel_else::FunctionObject<Channel::error>{};

}//namespace ex


#endif//STAY_IF_H
