#ifndef INLINED_RECEIVER_HPP
#define INLINED_RECEIVER_HPP

#include "concepts.hpp"
#include <algorithm>

namespace ex {

    template<class Derived, class Receiver>
    concept InlinableReceiver = requires(){
        typename Receiver::template InlinedReceiver<Derived>;
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
        requires InlinableReceiver<Derived, Receiver>
    struct InlinedReceiver<Derived, Receiver> 
		: Receiver::template InlinedReceiver<Derived>
	{
        
		InlinedReceiver(Receiver){}

    };

}

#endif//INLINED_RECEIVER_HPP
