#ifndef MAP_H
#define MAP_H

#include "concepts.hpp"
#include "inline.hpp"

namespace ex::algorithms::map {
	
	template<Channel channel, class SuffixReceiver, IsSender ChildSender, class Function>
	struct OpState
		: InlinedReceiver<OpState<channel, SuffixReceiver, ChildSender, Function>, SuffixReceiver>
		, ManualChildOp<OpState<channel, SuffixReceiver, ChildSender, Function>, 0, 0, ChildSender> 
	{ 
		Function function;
		
		OpState(SuffixReceiver suffix_receiver, ChildSender child_sender, Function function)
			: InlinedReceiver<OpState<channel, SuffixReceiver, ChildSender, Function>, SuffixReceiver>{suffix_receiver}
			, ManualChildOp<OpState<channel, SuffixReceiver, ChildSender, Function>, 0, 0, ChildSender>{child_sender}
			, function{function}
		{}

		template<class... Cont>
		auto start(Cont&... cont){
			return ManualChildOp<OpState<channel, SuffixReceiver, ChildSender, Function>, 0, 0, ChildSender>::template start<0>(cont...);
		}


		template<std::size_t ChildIndex, std::size_t VariantIndex, std::size_t StageIndex, class... Cont, class... Arg>
        auto set_value(Cont&... cont, Arg... args){
			if constexpr(channel == Channel::value){
				return ex::set_value.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., function(args...));
			} else if (channel == Channel::error){
				return ex::set_value.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., args...);
			}
        }

        template<std::size_t ChildIndex, std::size_t VariantIndex, std::size_t StageIndex, class... Cont, class... Arg>
        auto set_error(Cont&... cont, Arg... args){
			if constexpr(channel == Channel::value){
				return ex::set_error.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., args...);
			} else if (channel == Channel::error){
				return ex::set_error.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., function(args...));
			}
        }		
	};       


	template<Channel channel, IsSender ChildSender, class Function>
	struct Sender {
		using value_t = std::tuple<apply_values_t<Function, ChildSender>>;
	    
	    [[no_unique_address]] ChildSender child_sender;
	    [[no_unique_address]] Function function;

		template<IsReceiver SuffixReceiver>
	    constexpr auto connect(SuffixReceiver suffix_receiver){
	        return OpState<channel, SuffixReceiver, ChildSender, Function>{suffix_receiver, child_sender, function};
	    }

	};

	template<Channel channel>
	struct FunctionObject {
		template<IsSender ChildSender, class Function>
		constexpr static auto operator()(ChildSender child_sender, Function function){
			return Sender<channel, ChildSender, Function>{child_sender, function};
		}

		template<class Function>
		constexpr static auto operator()(Function function){
			return [=]<IsSender ChildSender>(ChildSender child_sender){
				return Sender<channel, ChildSender, Function>{child_sender, function};
			};
		}
	};

}//namespace map

namespace ex {

	inline constexpr auto map_value = algorithms::map::FunctionObject<Channel::value>{};
	inline constexpr auto map_error = algorithms::map::FunctionObject<Channel::error>{};

}//namespace ex


constexpr auto operator > (ex::IsSender auto sender, auto function){
    return ex::map_value(sender, function);
}


#endif//MAP_H
