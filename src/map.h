#ifndef MAP_H
#define MAP_H

#include "sender.h"

template<class ER, class F>
struct MapRecvr {
	[[no_unique_address]] ER end_recvr;
    [[no_unique_address]] F func;

	template<class Cont>
	void set_value(Cont&& cont, auto... args){
		return ::set_value(end_recvr, std::forward<Cont>(cont), func(args...));
	}
};        


template<Sender S, class F>
struct MapSender {
	using value_t = std::tuple<apply_values_t<F, S>>;
    [[no_unique_address]] S sender;
    [[no_unique_address]] F func;

    auto connect(auto end_recvr){
        auto map_recvr = MapRecvr{end_recvr, func};
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
