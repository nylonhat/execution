#ifndef INLINED_RECEIVER_HPP
#define INLINED_RECEIVER_HPP

#include "concepts.hpp"
#include <algorithm>

namespace ex {

    template<class Receiver>
    concept InlinableReceiver = requires(){
        {Receiver::make_for_child(nullptr)} -> std::same_as<Receiver>;
    };

    
    template<class Receiver>
    struct InlinedReceiver {
        [[no_unique_address]] Receiver receiver;
        
        InlinedReceiver(Receiver receiver)
            : receiver{receiver}
        {}

        Receiver& get_receiver(){
            return receiver;
        }
    };
    
    template<class Receiver>
        requires InlinableReceiver<Receiver>
    struct InlinedReceiver<Receiver> {
        InlinedReceiver(Receiver receiver){}

        Receiver get_receiver(this auto&& self){
            return Receiver::make_for_child(std::addressof(self));
        }
    };

}

#endif//INLINED_RECEIVER_HPP
