#ifndef MAP_H
#define MAP_H

#include "sender.h"

template<class ER, class F>
struct map_recvr {
        [[no_unique_address]] ER end_recvr;
        [[no_unique_address]] F func;

        auto set_value(auto... args){
                end_recvr.set_value(func(args...));
        }
};        


template<Sender S, class F>
struct map_sender {
        using value_t = std::tuple< apply_result_t<F, typename S::value_t> >;
        [[no_unique_address]] S sender1;
    [[no_unique_address]] F func;

        auto connect(auto end_recvr){
                return ::connect(sender1, map_recvr{end_recvr, func});
        }

};


auto map = [](Sender auto sender, auto func){
        return map_sender{sender, func};
};

auto operator > (Sender auto sender, auto func){
        return map(sender, func);
}


#endif//MAP_H