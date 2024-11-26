#ifndef SYNC_H
#define SYNC_H

#include <semaphore>
#include <print>
#include "concepts.hpp"

namespace ex::algorithms::sync_wait {

    template<class Result>
    struct Receiver {
        Result* result;
        std::binary_semaphore* flag;

        auto set_value(auto&... cont, auto value){
            *result = value;
            ex::start(cont...);
            flag->release();
    		return;
        }
        
        auto set_error(auto&... cont, auto error){
                *result = Result{};
                ex::start(cont...);
                flag->release();
        		return;
            }
        };

}//namespace ex::algorithms::sync

namespace ex {

    inline constexpr auto sync_wait = []<IsSender ChildSender>(ChildSender sender){
        using Result = single_value_t<ChildSender>;
        Result result;
        std::binary_semaphore flag{0};
    
    	auto sync_receiver = algorithms::sync_wait::Receiver{&result, &flag};
        auto op = ex::connect(sender, sync_receiver);
    	//std::println("final op size: {} bytes", sizeof(op));
	
    	ex::start(op);

        flag.acquire();
        return result;
    };

}//namespace ex


namespace ex::algorithms::start_constexpr {

    template<class Result>
    struct Receiver {
        Result* result;

        constexpr auto set_value(auto&... cont, auto value){
            *result = value;
            ex::start(cont...);
    		return;
        }
        
        constexpr auto set_error(auto&... cont, auto error){
                *result = Result{};
                ex::start(cont...);
        		return;
            }
        };

}//namespace ex::algorithms::start_constexpr


namespace ex {

    inline constexpr auto start_constexpr = []<IsSender ChildSender>(ChildSender sender){
        using Result = single_value_t<ChildSender>;
        Result result;
    
    	auto receiver = algorithms::start_constexpr::Receiver{&result};
        auto op = ex::connect(sender, receiver);
	
    	ex::start(op);

        return result;
    };

}//namespace ex

namespace ex::algorithms::sync_wait2 {

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
    
    
}//


namespace ex {

    inline constexpr auto sync_wait2 = []<IsSender ChildSender>(ChildSender child_sender){
        using Result = single_value_t<ChildSender>;
        Result result;
        std::binary_semaphore flag{0};
    
    	auto sync_op = algorithms::sync_wait2::OpState{child_sender, &result, &flag};
    	//std::println("final op size: {} bytes", sizeof(op));
	
    	ex::start(sync_op);

        flag.acquire();
        return result;
    };

}//namespace ex

#endif//SYNC_H
