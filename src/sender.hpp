#ifndef SENDER_HPP
#define SENDER_HPP

#include <utility>
#include <tuple>
#include <functional>
#include "op_state.hpp"
#include "receiver.hpp"



namespace ex {
    struct SenderOptIn {};
        
    template<class T>
    concept IsSender = requires(T t){
        typename T::SenderOptIn;
    };
    
}//namespace ex

namespace ex::concepts::connect_cpo {
    
    template<class T, class R>
    concept HasMember = requires(T t, R r){
        {t.connect(r)} -> IsOpState<>;  
    };

    template<class T, class R>
    concept HasFree = requires(T t, R r){
        {connect(t, r)} -> IsOpState<>;  
    };

    template<class T, class R>
    concept HasAll = HasMember<T, R> && HasFree<T, R>;
 
    
    struct FunctionObject {
        template<class Sender, class Receiver>
        requires HasMember<Sender, Receiver>
        constexpr static auto operator()(Sender&& sender, Receiver&& recvr){
            return sender.connect(recvr);
        }

        template<class Sender, class Receiver>
        requires HasFree<Sender, Receiver>
        constexpr static auto operator()(Sender&& sender, Receiver&& recvr){
            return connect(sender, recvr);
        }

        template<class Sender, class Receiver>
        requires HasAll<Sender, Receiver>
        constexpr static auto operator()(Sender&& sender, Receiver&& recvr){
            return sender.connect(recvr);
        }
        
    };
    
}//namespace ex::concepts::sender::connect

namespace ex {
    
    inline constexpr auto connect = concepts::connect_cpo::FunctionObject{};    
    
}//namespace ex

#endif//SENDER_HPP
