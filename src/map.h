#ifndef MAP_H
#define MAP_H

#include "sender.h"

template<class ER, class F>
struct MapRecvr {
        [[no_unique_address]] ER end_recvr;
        [[no_unique_address]] F func;

        auto set_value(auto... args){
                end_recvr.set_value(func(args...));
        }
};        


template<Sender S, class F>
struct MapSender {
        using value_t = std::tuple<apply_value_t<F, S>>;
        [[no_unique_address]] S sender;
        [[no_unique_address]] F func;

        auto connect(auto end_recvr){
                auto map_rcvr = MapRecvr{end_recvr, func};
                return ::connect(sender, map_recvr);
        }

};


auto map = [](Sender auto sender, auto func){
        return MapSender{sender, func};
};

auto operator > (Sender auto sender, auto func){
        return map(sender, func);
}


#endif//MAP_H