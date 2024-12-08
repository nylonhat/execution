#include "doctest.h"

#include "../src/pure.hpp"
#include "../src/sync.hpp"
#include "../src/map.hpp"
#include "../src/repeat.hpp"

#include <atomic>

namespace {

    TEST_CASE("repeat no times"){
        auto always_false = [](auto...){
            return false;
        };

        auto monadic_function = [](auto ...){
            return ex::value(100);
        };
        
        auto result = ex::repeat_while_value(ex::value(0), always_false, monadic_function) | ex::sync_wait;
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
        
        auto result = ex::repeat_while_value(ex::value(0), one_time, monadic_function) | ex::sync_wait;
        CHECK(result == 100);
        
    }

    
    TEST_CASE("repeat n times"){
        const auto max = 10;
        
        auto n_times = [i = decltype(max){0}](auto...) mutable {
            return i++ < max ? true : false; 
        };

        auto monadic_function = [count = 0](auto i) mutable {
            return ex::value(++count) ;
        };
        
        auto result = ex::repeat_while_value(ex::value(0), n_times, monadic_function) | ex::sync_wait;
        CHECK(result == 10);
        
    }

    
    constexpr auto add = [](auto... v) {
    	return (... + v);
    };

    TEST_CASE("repeat_n and map tail call test"){
    	auto result = ex::value(42) | ex::map_value(add) | ex::repeat_n_value(10'000'000) | ex::sync_wait;
    	CHECK(result == 42);
    }

    
}//namespace
