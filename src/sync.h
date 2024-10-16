#ifndef SYNC_H
#define SYNC_H

#include <semaphore>
#include <print>
#include "sender.h"

template<class T>
struct SyncRecvr {
        T* value;
        std::binary_semaphore* flag;

        auto set_value(OpHandle op_handle, auto v){
                *value = v;
                flag->release();
				return op_handle.start({});
        }
};

auto sync_wait = []<class S>(S sender){
        using T = single_value_t<S>;
        T value;
        std::binary_semaphore flag{0};
        
		auto sync_recvr = SyncRecvr{&value, &flag};
        auto op = ::connect(sender, sync_recvr);
		std::println("final op size: {} bytes", sizeof(op));
		
		::start(op, {});

        flag.acquire();
        return value;
};

#endif//SYNC_H
