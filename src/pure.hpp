#ifndef PURE_H
#define PURE_H

#include "concepts.hpp"

namespace ex::algorithms::pure {

    
    template<class NextReceiver, class... Values>
    struct OpState {
        [[no_unique_address]] NextReceiver next_receiver;
        std::tuple<Values...> values;

        template<class... Cont>
        void start(Cont&... cont){

            auto lambda = [&](auto... args){
                return ex::set_value.operator()<NextReceiver, Cont...>(next_receiver, cont..., args...);
            };

            return std::apply(lambda, values);
        }
    };

    template<class... Values>
    struct Sender {
        using value_t = std::tuple<Values...>;
        std::tuple<Values...> values;

        Sender(auto... values)
            : values{values...}
        {}
        template<class NextReceiver>
        auto connect(NextReceiver next_receiver){
            return OpState{next_receiver, values};
        }
    };

}//namespace ex::algorithms::pure

namespace ex {

    inline constexpr auto pure = []<class... Values>(Values... values){
        return algorithms::pure::Sender<Values...>{values...};
    };

}//namespace ex

#endif//PURE_H
