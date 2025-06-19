#ifndef PURE_H
#define PURE_H

#include "concepts.hpp"
#include "signature.hpp"
#include "inlined_receiver.hpp"

namespace ex {
inline namespace pure_algorithm {
    
    template<size_t I, class T>
    struct Leaf{
        [[no_unique_address]] T member;
    };

    template<Channel channel, class...>
    struct OpBase;
    
    template<Channel channel, size_t... I, class NextRx, class... Results>
    struct OpBase<channel, std::index_sequence<I...>, NextRx, Results...>
        : InlinedReceiver<OpBase<channel, std::index_sequence<I...>, NextRx, Results...>, NextRx>
        , Leaf<I, Results>...
    {
		using OpStateOptIn = ex::OpStateOptIn;
        using Receiver = InlinedReceiver<OpBase, NextRx>;
        
        constexpr OpBase(NextRx next_receiver, Results... results)
            : Receiver{next_receiver}
            , Leaf<I, Results>{results}...
        {}

        template<class... Cont>
        constexpr void start(Cont&... cont){
            if constexpr(channel == Channel::value){
                [[gnu::musttail]] return ex::set_value<Cont...>(this->get_receiver(), cont..., Leaf<I, Results>::member...);
            } else if(channel == Channel::error){
                [[gnu::musttail]] return ex::set_error<Cont...>(this->get_receiver(), cont..., Leaf<I, Results>::member...);
            }
        }
    };

    template<Channel channel, class NextRx, class... Results>
    struct Op 
        : OpBase<channel, std::index_sequence_for<Results...>, NextRx, Results...>
    {
        Op(NextRx next_receiver, Results... results)
            : OpBase<channel, std::index_sequence_for<Results...>, NextRx, Results...>{next_receiver, results...}
        {}    
    };

    template<Channel channel1, Channel channel2, class... Results>
    struct pure_channel;

    template<Channel channel1, Channel channel2, class... Results>
    requires (channel1 == channel2)
    struct pure_channel<channel1, channel2, Results...>{
        using type = std::tuple<std::tuple<Results...>>;
    };

    template<Channel channel1, Channel channel2, class... Results>
    requires (channel1 != channel2)
    struct pure_channel<channel1, channel2, Results...>{
        using type = std::tuple<>;
    };

    template<Channel channel, class...>
    struct SenderBase;

    template<Channel channel, size_t... I, class... Results>
    struct SenderBase<channel, std::index_sequence<I...>, Results...>
        : Leaf<I, Results>...
    {
        
        using SenderOptIn = ex::SenderOptIn;
        using value_t = pure_channel<Channel::value, channel, Results...>::type;        
        using error_t = pure_channel<Channel::error, channel, Results...>::type;        

        SenderBase(Results... results)
            : Leaf<I, Results>{results}...
        {}

        template<class NextRx>
        auto connect(NextRx next_receiver){
            return Op<channel, NextRx, Results...>{next_receiver, Leaf<I, Results>::member...};
        }
    };

    template<Channel channel, class... Results>
    struct Sender 
        : SenderBase<channel, std::index_sequence_for<Results...>, Results...>
    {
        Sender(Results... results)
            : SenderBase<channel, std::index_sequence_for<Results...>, Results...>{results...}
        {}

    };

    template<Channel channel>
    struct FnObj {

        template<class... Results>
        constexpr static auto operator()(Results... results){
            return Sender<channel, Results...>{results...};
        }
    };

}}//namespace ex::pure_algorithm

namespace ex {
	
    template<Channel channel>
    inline constexpr auto pure = pure_algorithm::FnObj<channel>{};
    
    inline constexpr auto value = pure_algorithm::FnObj<Channel::value>{};
    inline constexpr auto error = pure_algorithm::FnObj<Channel::error>{};

}//namespace ex

#endif//PURE_H
