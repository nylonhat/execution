#ifndef SENDER_H
#define SENDER_H

#include <functional>

struct noop_recvr {        
        auto set_value(auto... args){}
};

auto id = [](auto i){return i;};

template<typename S>
concept Sender = requires(S sender, noop_recvr recvr){
        sender.connect(recvr);
};

template<typename F, Sender S>
using apply_values_t = std::invoke_result_t<decltype(&std::apply<F, typename S::value_t>), F, typename S::value_t>;        

template<Sender... Ss>
using values_join_t =  std::invoke_result_t<decltype(&std::tuple_cat<typename Ss::value_t...>), typename Ss::value_t...>;        

template<Sender S>
using single_value_t = apply_values_t<decltype(id), S>;

auto start = [](auto& op){
        op.start();
};

auto connect = [](Sender auto&& sender, auto recvr){
        return sender.connect(recvr);
};

template<Sender S, typename R>
using connect_t = std::invoke_result_t<decltype(::connect), S, R>;

auto set_value = [](auto recvr, auto... args){
        recvr.set_value(args...);
};

auto operator | (Sender auto value, auto func){
        return func(value);
}

#endif//SENDER_H
