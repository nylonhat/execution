#ifndef SYNC_H
#define SYNC_H

#include <semaphore>
#include <print>
#include "concepts.hpp"

namespace ex::algorithms::sync_wait {

    template<class ChildSender>
    struct OpState 
        : ManualChildOp<OpState<ChildSender>, 0, 0, ChildSender>
    {

        using Result = single_value_t<ChildSender>;

        Result* result;
        std::binary_semaphore* flag;
        
        OpState(ChildSender child_sender, Result* result, std::binary_semaphore* flag)
            : ManualChildOp<OpState<ChildSender>, 0, 0, ChildSender>{child_sender}
            , result{result}
            , flag{flag}
        {}

        template<class... Cont>
        auto start(Cont&... cont){
            return ManualChildOp<OpState<ChildSender>, 0, 0, ChildSender>::template start<0>(cont...);
        }

        template<std::size_t ChildIndex, std::size_t VariantIndex, std::size_t StageIndex, class... Cont, class... Arg>
        auto set_value(Cont&... cont, Arg... arg){
            *result = arg...[0];
            ex::start(cont...);
            flag->release();
            return;
        }
    };
    
    
}//namespace algorithms::sync_wait

namespace ex {

    inline constexpr auto sync_wait = []<IsSender ChildSender>(ChildSender child_sender){
        using Result = single_value_t<ChildSender>;
        Result result;
        std::binary_semaphore flag{0};
    
    	auto sync_op = algorithms::sync_wait::OpState{child_sender, &result, &flag};
    	//std::println("final op size: {} bytes", sizeof(op));
	
    	ex::start(sync_op);

        flag.acquire();
        return result;
    };

}//namespace ex

#endif//SYNC_H
