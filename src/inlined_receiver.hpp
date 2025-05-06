#ifndef INLINED_RECEIVER_HPP
#define INLINED_RECEIVER_HPP

#include "concepts.hpp"
#include <algorithm>

namespace ex {

    template<class Receiver, class ChildOp>
    concept InlinableReceiver = requires(ChildOp* op){
        {Receiver::make_for_child(op)} -> std::same_as<Receiver>;
    };

    
    template<class Derived, class Receiver>
    struct InlinedReceiver {
        [[no_unique_address]] Receiver receiver;
        
        InlinedReceiver(Receiver receiver)
            : receiver{receiver}
        {}

        Receiver& get_receiver(){
            return receiver;
        }
    };
    
    template<class Derived, class Receiver>
        requires InlinableReceiver<Receiver, Derived>
    struct InlinedReceiver<Derived, Receiver> {
        InlinedReceiver(Receiver receiver){}

        Receiver get_receiver(){
            return Receiver::make_for_child(static_cast<Derived*>(this));
        }
    };

}

#endif//INLINED_RECEIVER_HPP
