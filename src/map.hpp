#ifndef MAP_H
#define MAP_H

#include "concepts.hpp"

namespace ex::algorithms::map {

	template<IsReceiver NextReceiver, class Function>
	struct Receiver {
		[[no_unique_address]] NextReceiver next_receiver;
	    [[no_unique_address]] Function function;

		template<IsOpState... Cont>
		void set_value(Cont&... cont, auto... args){
			return ex::set_value.operator()<NextReceiver, Cont...>(
			    next_receiver, 
			    cont..., 
			    function(args...)
			);
		}

		
		template<IsOpState... Cont>
		void set_error(Cont&... cont, auto... args){
			return ex::set_error.operator()<NextReceiver, Cont...>(
			    next_receiver, 
			    cont..., 
			    args...
			);
		}
	};        


	template<IsSender ChildSender, class Function>
	struct Sender {
		using value_t = std::tuple<apply_values_t<Function, ChildSender>>;
	    [[no_unique_address]] ChildSender child_sender;
	    [[no_unique_address]] Function function;

	    auto connect(auto suffix_receiver){
	        auto map_receiver = Receiver{suffix_receiver, function};
	        return ex::connect(child_sender, map_receiver);
	    }

	};

}//namespace map

namespace ex {

	inline constexpr auto map = [](IsSender auto child_sender, auto function){
	    return algorithms::map::Sender{child_sender, function};
	};

	

}//namespace ex


auto operator > (ex::IsSender auto sender, auto function){
    return ex::map(sender, function);
}


#endif//MAP_H
