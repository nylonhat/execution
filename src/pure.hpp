#ifndef PURE_H
#define PURE_H

#include "concepts.hpp"
#include "inlined_receiver.hpp"

namespace ex::algorithms::pure {
    
    template<size_t I, class T>
    struct Leaf{
        [[no_unique_address]] T member;
    };

    template<Channel channel, class...>
    struct OpStateBase;
    
    template<Channel channel, size_t... I, class NextReceiver, class... Values>
    struct OpStateBase<channel, std::index_sequence<I...>, NextReceiver, Values...>
        : InlinedReceiver<OpStateBase<channel, std::index_sequence<I...>, NextReceiver, Values...>, NextReceiver>
        , Leaf<I, Values>...
    {
		using OpStateOptIn = ex::OpStateOptIn;
        using Receiver = InlinedReceiver<OpStateBase, NextReceiver>;
        
        constexpr OpStateBase(NextReceiver next_receiver, Values... values)
            : Receiver{next_receiver}
            , Leaf<I, Values>{values}...
        {}

        template<class... Cont>
        constexpr void start(Cont&... cont){
            if constexpr(channel == Channel::value){
                return ex::set_value<Cont...>(this->get_receiver(), cont..., Leaf<I, Values>::member...);
            } else if(channel == Channel::error){
                return ex::set_error<Cont...>(this->get_receiver(), cont..., Leaf<I, Values>::member...);
            }
        }
    };

    template<Channel channel, class NextReceiver, class... Values>
    struct OpState 
        : OpStateBase<channel, std::index_sequence_for<Values...>, NextReceiver, Values...>
    {
        OpState(NextReceiver next_receiver, Values... values)
            : OpStateBase<channel, std::index_sequence_for<Values...>, NextReceiver, Values...>{next_receiver, values...}
        {}    
    };

    template<Channel channel1, Channel channel2, class... Values>
    struct pure_channel;

    template<Channel channel1, Channel channel2, class... Values>
    requires (channel1 == channel2)
    struct pure_channel<channel1, channel2, Values...>{
        using type = std::tuple<Values...>;
    };

    template<Channel channel1, Channel channel2, class... Values>
    requires (channel1 != channel2)
    struct pure_channel<channel1, channel2, Values...>{
        using type = std::tuple<>;
    };

    template<Channel channel, class...>
    struct SenderBase;

    template<Channel channel, size_t... I, class... Values>
    struct SenderBase<channel, std::index_sequence<I...>, Values...>
        : Leaf<I, Values>...
    {
        
        using SenderOptIn = ex::SenderOptIn;
        using value_t = pure_channel<Channel::value, channel, Values...>::type;        
        using error_t = pure_channel<Channel::error, channel, Values...>::type;        

        SenderBase(Values... values)
            : Leaf<I, Values>{values}...
        {}

        template<class NextReceiver>
        auto connect(NextReceiver next_receiver){
            return OpState<channel, NextReceiver, Values...>{next_receiver, Leaf<I, Values>::member...};
        }
    };

    template<Channel channel, class... Values>
    struct Sender 
        : SenderBase<channel, std::index_sequence_for<Values...>, Values...>
    {
        Sender(Values... values)
            : SenderBase<channel, std::index_sequence_for<Values...>, Values...>{values...}
        {}

    };

    template<Channel channel>
    struct FunctionObject {

        template<class... Values>
        constexpr static auto operator()(Values... values){
            return Sender<channel, Values...>{values...};
        }
    };

}//namespace ex::algorithms::pure

namespace ex {
    template<Channel channel>
    inline constexpr auto pure = algorithms::pure::FunctionObject<channel>{};
    
    inline constexpr auto value = algorithms::pure::FunctionObject<Channel::value>{};
    inline constexpr auto error = algorithms::pure::FunctionObject<Channel::error>{};

}//namespace ex

#endif//PURE_H
