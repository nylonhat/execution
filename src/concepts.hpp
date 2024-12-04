#ifndef CONCEPTS_HPP
#define CONCEPTS_HPP

#include "op_state.hpp"
#include "sender.hpp"
#include "receiver.hpp"

namespace ex {

    constexpr inline  auto identity = [](auto i){return i;};
    
    constexpr inline auto add = [](auto... v) {
    	return (... + v);
    };

    template<typename Function, IsSender Sender>
    using apply_values_t = std::invoke_result_t<
        decltype(&std::apply<Function, typename Sender::value_t>), 
        Function, typename Sender::value_t
    >; 

    template<typename Function, IsSender Sender>
    using apply_errors_t = std::invoke_result_t<
        decltype(&std::apply<Function, typename Sender::error_t>), 
        Function, typename Sender::error_t
    >;       

    template<IsSender... Senders>
    using values_join_t =  std::invoke_result_t<
        decltype(&std::tuple_cat<typename Senders::value_t...>), 
        typename Senders::value_t...
    >;        

    template<IsSender Sender>
    using single_value_t = apply_values_t<decltype(identity), Sender>;

    template<IsSender Sender, IsReceiver Receiver>
    using connect_t = std::invoke_result_t<decltype(ex::connect), Sender, Receiver>;

    
    enum class Channel {value, error};  
    
}//namespace ex

constexpr auto operator | (ex::IsSender auto value, auto func){
	return func(value);
}

#endif//CONCEPTS_HPP
