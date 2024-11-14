#ifndef PURE_H
#define PURE_H

#include "concepts.hpp"

namespace ex::algorithms::pure {
    
    template<Channel channel, class NextReceiver, class... Values>
    struct OpState {
        
        NextReceiver next_receiver;
        [[no_unique_address]] std::tuple<Values...> values;

        template<class... Cont>
        void start(Cont&... cont){

            auto lambda = [&](auto... args){
                if constexpr(channel == Channel::value){
                    return ex::set_value.operator()<NextReceiver, Cont...>(next_receiver, cont..., args...);
                } else if (channel == Channel::error) {
                    return ex::set_error.operator()<NextReceiver, Cont...>(next_receiver, cont..., args...);
                }
            };

            return std::apply(lambda, values);
        }
    };

    template<Channel channel, class... Values>
    struct Sender {
        using value_t = std::tuple<Values...>;
        
        std::tuple<Values...> values;

        Sender(auto... values)
            : values{values...}
        {}

        template<class NextReceiver>
        auto connect(NextReceiver next_receiver){
            return OpState<channel, NextReceiver, Values...>{next_receiver, values};
        }
    };

    template<Channel channel>
    struct FunctionObject {

        template<class... Values>
        auto operator()(this auto&&, Values... values){
            return Sender<channel, Values...>{values...};
        }
    };

}//namespace ex::algorithms::pure

namespace ex {

    inline constexpr auto value = algorithms::pure::FunctionObject<Channel::value>{};
    inline constexpr auto error = algorithms::pure::FunctionObject<Channel::error>{};

}//namespace ex

#endif//PURE_H
