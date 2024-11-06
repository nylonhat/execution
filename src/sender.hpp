#ifndef SENDER_HPP
#define SENDER_HPP

#include "op_state.hpp"

namespace ex::concepts::connect_cpo {

    struct DummyRecvr{
        auto set_value(auto&..., auto&&...){}
    };
    
    template<class T>
    concept HasMember = requires(T t){
        {t.connect(DummyRecvr{})} -> ex::OpState<>;  
    };

    template<class T>
    concept HasFree = requires(T t){
        {connects(t, DummyRecvr{})} -> ex::OpState<>;  
    };

    template<class T>
    concept HasAll = HasMember<T> && HasFree<T>;
 
    
    struct Function {
        auto operator()(this auto&&, HasMember auto&& sender, auto&& recvr){
            return sender.connect(recvr);
        }


        auto operator()(this auto&&, HasFree auto&& sender, auto&& recvr){
            return connect(sender, recvr);
        }

        
        auto operator()(this auto&&, HasAll auto&& sender, auto&& recvr){
            return sender.connect(recvr);
        }
        
    };
    
}//namespace ex::concepts::sender::connect

namespace ex {
    
    inline constexpr auto connect = concepts::connect_cpo::Function{};

    template<class T>
    concept Sender = requires(T t){
        {ex::connect(t, concepts::connect_cpo::DummyRecvr{})} -> ex::OpState<>;  
    };
    
}

#endif//SENDER_HPP
