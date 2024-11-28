#ifndef PURE_H
#define PURE_H

#include "concepts.hpp"
#include "inline.hpp"

namespace ex::algorithms::pure {
    
    template<Channel channel, class Tuple, std::size_t... I, class Recvr, class... Cont>
    constexpr auto apply_set_impl(Tuple&& t, std::index_sequence<I...>, Recvr&& recvr, Cont&... cont){
        if constexpr(channel == Channel::value){
            return ex::set_value.operator()<Recvr, Cont...>(std::forward<Recvr>(recvr), cont..., std::get<I>(std::forward<Tuple>(t))...);    
        }else if (channel == Channel::error){
            return ex::set_error.operator()<Recvr, Cont...>(std::forward<Recvr>(recvr), cont..., std::get<I>(std::forward<Tuple>(t))...);    
        }
    }

    template<Channel channel, class Tuple, class Recvr, class... Cont>
    constexpr auto apply_set(Tuple&& t, Recvr&& recvr, Cont&... cont){
        return apply_set_impl<channel>(std::forward<Tuple>(t), std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>{}, std::forward<Recvr>(recvr), cont...);
    }
    
    template<Channel channel, class NextReceiver, class... Values>
    struct OpState 
        : public InlinedReceiver<OpState<channel, NextReceiver, Values...>, NextReceiver>
    {
        
        //[[no_unique_address]] NextReceiver next_receiver;
        [[no_unique_address]] std::tuple<Values...> values;

        constexpr OpState(NextReceiver next_receiver, std::tuple<Values...> values)
            //: next_receiver{next_receiver}
            : InlinedReceiver<OpState<channel, NextReceiver, Values...>, NextReceiver>{next_receiver}
            , values{values}
        {}

        template<class... Cont>
        constexpr void start(Cont&... cont){
            return apply_set<channel>(values, this->get_receiver(), cont...);
        }
    };

    template<Channel channel, class... Values>
    struct Sender {
        using value_t = std::tuple<Values...>;
        
        std::tuple<Values...> values;

        constexpr Sender(auto... values)
            : values{values...}
        {}

        template<class NextReceiver>
        constexpr auto connect(NextReceiver next_receiver){
            return OpState<channel, NextReceiver, Values...>{next_receiver, values};
        }
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

    inline constexpr auto value = algorithms::pure::FunctionObject<Channel::value>{};
    inline constexpr auto error = algorithms::pure::FunctionObject<Channel::error>{};

}//namespace ex

#endif//PURE_H
