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

    enum class Channel {value, error};  

    template<Channel channel, class S>
    struct channel_t_impl;

    template<class S>
    struct channel_t_impl<Channel::value, S>{
        using type = S::value_t;
    };

    template<class S>
    struct channel_t_impl<Channel::error, S>{
        using type = S::error_t;
    };

    template<Channel channel, class S>
    using channel_t = channel_t_impl<channel, S>::type;
        
    template<Channel channel, class Function, IsSender Sender>
    using apply_channel_t = std::invoke_result_t<
        decltype(&std::apply<Function, channel_t<channel, Sender>>), 
        Function, channel_t<channel, Sender>
    >;
     
        
    template<typename Function, IsSender Sender>
    using apply_values_t = apply_channel_t<Channel::value, Function, Sender>;
    
    template<typename Function, IsSender Sender>
    using apply_errors_t = apply_channel_t<Channel::error, Function, Sender>;     

    template<IsSender... Senders>
    using values_join_t =  std::invoke_result_t<
        decltype(&std::tuple_cat<typename Senders::value_t...>), 
        typename Senders::value_t...
    >;        

    template<IsSender Sender>
    using single_value_t = apply_values_t<decltype(identity), Sender>;

    template<class Sender>
    concept IsSingleValueSender = requires(Sender sender){
        typename single_value_t<Sender>;
    };

    template<IsSender Sender, IsReceiver Receiver>
    using connect_t = std::invoke_result_t<decltype(ex::connect), Sender, Receiver>;

    
    
}//namespace ex

constexpr auto operator | (ex::IsSender auto value, auto func){
	return func(value);
}

#endif//CONCEPTS_HPP
