#ifndef SYNC_H
#define SYNC_H

#include <semaphore>
#include "sender.h"

template<class T>
struct SyncRecvr {
        T* value;
        std::binary_semaphore* flag;

        auto set_value(auto v){
                *value = v;
                flag->release();
        }
};

auto sync = []<class S>(S sender){
        using T = single_value_t<S>;
        T value;
        std::binary_semaphore flag{0};
        auto sync_recvr = SyncRecvr{&value, &flag};
        auto op = ::connect(sender, sync_recvr);
        ::start(op);
        flag.acquire();
        return value;
};

#endif//SYNC_H