#ifndef PURE_H
#define PURE_H

#include "sender.h"

template<class R, class T>
struct PureOp {
    [[no_unique_address]] R recvr;
    T value;

    void start(auto&& cont){
        ::set_value(recvr, std::forward<decltype(cont)>(cont), value);
    }
};

template<class T>
struct PureSender {
    using value_t = std::tuple<T>;
    T value;

    template<class R>
    auto connect(R recvr){
        return PureOp{recvr, value};
    }
};


auto pure = []<class V>(V value){
    return PureSender<V>{value};
};

template<class R, class A, class B>
struct Pure2Op {
    [[no_unique_address]] R recvr;
    A a;
    B b;

    auto start(auto&& cont){
        ::set_value(recvr, std::forward<decltype(cont)>(cont), a, b);
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

auto pure2 = [](auto a, auto b){
    return Pure2Sender{a, b};
};

#endif//PURE_H
