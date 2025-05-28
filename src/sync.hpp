#ifndef SYNC_H
#define SYNC_H

#include <semaphore>
#include <print>
#include "concepts.hpp"
#include "variant_child.hpp"

namespace ex::algorithms::sync_wait {

    template<class ChildSender>
    struct OpState 
        : VariantChildOp<OpState<ChildSender>, 0, ChildSender>
    {
		using OpStateOptIn = ex::OpStateOptIn;
        using ChildOp = VariantChildOp<OpState, 0, ChildSender>;
        using Result = single_value_t<ChildSender>;

        Result* result;
        std::binary_semaphore* flag;
        
        OpState(ChildSender child_sender, Result* result, std::binary_semaphore* flag)
            : ChildOp{child_sender}
            , result{result}
            , flag{flag}
        {}

        template<class... Cont>
        auto start(Cont&... cont){
            [[gnu::musttail]] return ChildOp::template start<0>(cont...);
        }

        template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
        auto set_value(Cont&... cont, Arg... arg){
            *result = arg...[0];
            ex::start(cont...);
            flag->release();
            return;
        }

        template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
        auto set_error(Cont&... cont, Arg... arg){
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
    	// std::println("final op size: {} bytes", sizeof(sync_op));
	
    	ex::start(sync_op);

        flag.acquire();
        return result;
    };

}//namespace ex

#endif//SYNC_H
