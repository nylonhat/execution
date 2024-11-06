#ifndef MAP_H
#define MAP_H

#include "sender.hpp"
#include "recvr.hpp"
#include "sender.hpp"

namespace ex::algorithms::map {

	template<class ER, class F>
	struct MapRecvr {
		[[no_unique_address]] ER end_recvr;
	    [[no_unique_address]] F func;

		template<class... Cont>
		void set_value(Cont&... cont, auto... args){
			return ex::set_value.operator()<ER, Cont...>(end_recvr, cont..., func(args...));
		}
	};        


	template<Sender S, class F>
	struct MapSender {
		using value_t = std::tuple<apply_values_t<F, S>>;
	    [[no_unique_address]] S sender;
	    [[no_unique_address]] F func;

	    auto connect(auto end_recvr){
	        auto map_recvr = MapRecvr{end_recvr, func};
	        return ex::connect(sender, map_recvr);
	    }

	};

}//namespace map

namespace ex {

	inline constexpr auto map = [](Sender auto sender, auto func){
	    return algorithms::map::MapSender{sender, func};
	};

	

}//namespace ex


auto operator > (ex::Sender auto sender, auto func){
    return ex::map(sender, func);
}


#endif//MAP_H
