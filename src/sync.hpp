#ifndef SYNC_H
#define SYNC_H

#include <semaphore>
#include <print>
#include "concepts.hpp"

namespace ex::algorithms::sync_wait {

    template<class Result>
    struct Receiver {
        Result* result;
        std::binary_semaphore* flag;

        auto set_value(auto&... cont, auto value){
            *result = value;
            ex::start(cont...);
            flag->release();
    		return;
        }
    };

}//namespace ex::algorithms::sync

namespace ex {

    inline constexpr auto sync_wait = []<IsSender ChildSender>(ChildSender sender){
        using Result = single_value_t<ChildSender>;
        Result result;
        std::binary_semaphore flag{0};
    
    	auto sync_receiver = algorithms::sync_wait::Receiver{&result, &flag};
        auto op = ex::connect(sender, sync_receiver);
    	//std::println("final op size: {} bytes", sizeof(op));
	
    	ex::start(op);

        flag.acquire();
        return result;
    };

}//namespace ex

#endif//SYNC_H
