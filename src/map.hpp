#ifndef MAP_H
#define MAP_H

#include "concepts.hpp"

namespace ex::algorithms::map {

	template<Channel channel, IsReceiver NextReceiver, class Function>
	struct Receiver {
		[[no_unique_address]] NextReceiver next_receiver;
	    [[no_unique_address]] Function function;

		template<IsOpState... Cont>
		constexpr auto set_value(Cont&... cont, auto... args){
			if constexpr(channel == Channel::value){
				return ex::set_value.operator()<NextReceiver, Cont...>(next_receiver, cont..., function(args...));
			} else if (channel == Channel::error){
				return ex::set_value.operator()<NextReceiver, Cont...>(next_receiver, cont..., args...);
			}
		}
		
		template<IsOpState... Cont>
		constexpr auto set_error(Cont&... cont, auto... args){
			if constexpr(channel == Channel::value){
				return ex::set_error.operator()<NextReceiver, Cont...>(next_receiver, cont..., args...);
			} else if (channel == Channel::error){
				return ex::set_value.operator()<NextReceiver, Cont...>(next_receiver, cont..., function(args...));
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
			auto infix_receiver = Receiver<channel, SuffixReceiver, Function>{suffix_receiver, function};
	        return ex::connect(child_sender, infix_receiver);
	    }

	};

	template<Channel channel>
	struct FunctionObject {
		template<IsSender ChildSender, class Function>
		constexpr auto operator()(this auto&&, ChildSender child_sender, Function function){
			return Sender<channel, ChildSender, Function>{child_sender, function};
		}

		template<class Function>
		constexpr auto operator()(this auto&&, Function function){
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
