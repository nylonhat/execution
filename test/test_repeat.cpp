#include "doctest.h"

#include "../src/pure.hpp"
#include "../src/sync.hpp"
#include "../src/repeat2.hpp"

namespace {

    TEST_CASE("repeat no times"){
        auto always_false = [](auto...){
            return false;
        };

        auto monadic_function = [](auto ...){
            return ex::value(100);
        };
        
        auto result = ex::repeat2(ex::value(0), always_false, monadic_function) | ex::sync_wait;
        CHECK(result == 0);
        
    }

    
    TEST_CASE("repeat one time"){
        auto one_time = [i = int{0}](auto...) mutable {
            if(i == 1){
                return false;
            }
            i++;
            return true;
        };

        auto monadic_function = [](auto ...){
            return ex::value(100);
        };
        
        auto result = ex::repeat2(ex::value(0), one_time, monadic_function) | ex::sync_wait;
        CHECK(result == 100);
        
    }

    
    TEST_CASE("repeat n times"){
        const auto max = 10;
        
        auto n_times = [i = decltype(max){0}](auto...) mutable {
            return i++ < max ? true : false; 
        };

        auto monadic_function = [count = 0](auto i) mutable {
            return ex::value(++count);
        };
        
        auto result = ex::repeat2(ex::value(0), n_times, monadic_function) | ex::sync_wait;
        CHECK(result == 10);
        
    }

    
}//namespace
