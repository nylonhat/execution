#ifndef PURE_H
#define PURE_H

#include "concepts.hpp"

namespace ex::algorithms::pure {

    template<class R, class T>
    struct PureOp {
        [[no_unique_address]] R recvr;
        T value;

        template<class... Cont>
        void start(Cont&... cont){
            ex::set_value.operator()<R, Cont...>(recvr, cont..., value);
        }
    };

    template<class T>
    struct PureSender {
        using value_t = std::tuple<T>;
        using error_t = std::tuple<>;
        T value;

        template<class R>
        auto connect(R recvr){
            return PureOp{recvr, value};
        }
    };


    template<class R, class A, class B>
    struct Pure2Op {
        [[no_unique_address]] R recvr;
        A a;
        B b;

        template<class... Cont>
        void start(Cont&... cont){
            ex::set_value.operator()<R, Cont...>(recvr, cont..., a, b);
        }
    };

    template<class A, class B>
    struct Pure2Sender {
        using value_t = std::tuple<A, B>;
        A a;
        B b;

        template<class R>
        auto connect(R recvr){
            return Pure2Op{recvr, a, b};
        }
    };

}//namespace ex::algorithms::pure

namespace ex {

    inline constexpr auto pure = []<class V>(V value){
        return algorithms::pure::PureSender<V>{value};
    };

    inline constexpr auto pure2 = [](auto a, auto b){
        return algorithms::pure::Pure2Sender{a, b};
    };

}//namespace ex

#endif//PURE_H
