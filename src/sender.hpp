#ifndef SENDER_HPP
#define SENDER_HPP

#include <utility>
#include <tuple>
#include <functional>
#include "op_state.hpp"
#include "receiver.hpp"

namespace ex::concepts::connect_cpo {

    struct DummyRecvr{
        auto set_value(auto&..., auto&&...){}
        auto set_error(auto&..., auto&&...){}
    };
    
    template<class T>
    concept HasMember = requires(T t){
        {t.connect(DummyRecvr{})} -> IsOpState<>;  
    };

    template<class T>
    concept HasFree = requires(T t){
        {connect(t, DummyRecvr{})} -> IsOpState<>;  
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
    concept IsSender = requires(T t){
        {ex::connect(t, concepts::connect_cpo::DummyRecvr{})} -> IsOpState<>;  
    };
    
}//namespace ex

#endif//SENDER_HPP
