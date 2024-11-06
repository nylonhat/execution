#ifndef SYNC_H
#define SYNC_H

#include <semaphore>
#include <print>
#include "op_state.hpp"
#include "sender.hpp"


namespace ex::algorithms::sync_wait {

    template<class T>
    struct Recvr {
        T* value;
        std::binary_semaphore* flag;

        auto set_value(auto&... cont, auto v){
            *value = v;
            ex::start(cont...);
            flag->release();
    		return;
        }
    };

}//namespace ex::algorithms::sync

namespace ex {

    inline constexpr auto sync_wait = []<class S>(S sender){
        using T = single_value_t<S>;
        T value;
        std::binary_semaphore flag{0};
    
    	auto sync_recvr = algorithms::sync_wait::Recvr{&value, &flag};
        auto op = ex::connect(sender, sync_recvr);
    	//std::println("final op size: {} bytes", sizeof(op));
	
    	ex::start(op);

        flag.acquire();
        return value;
    };

}//namespace ex

#endif//SYNC_H
