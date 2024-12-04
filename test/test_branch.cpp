#include "doctest.h"
#include "../src/branch.hpp"
#include "../src/map.hpp"
#include "../src/pure.hpp"
#include "../src/repeat.hpp"
#include "../src/sync.hpp"
#include "../src/benchmark.hpp"
#include "../src/threadpool.hpp"
#include "../src/inline_scheduler.hpp"

namespace {

    constexpr inline auto add = [](auto... v) {
    	return (... + v);
    };


    TEST_CASE("branch: simple fork join with inline scheduler benchmark"){
	
    	InlineScheduler scheduler{};

    	auto result = ex::value(4)
    		| ex::branch(scheduler, ex::value(42)) 
    		| ex::map_value(add)
    		| ex::repeat_n(10'000'000) 
    		| ex::benchmark 
    		| ex::sync_wait;

    	CHECK(result <= 18);
    }

    TEST_CASE("branch on threadpool 1 thread benchmark"){
	
    	Threadpool<1> scheduler{};

    	auto result = ex::value(4)
    		| ex::branch(scheduler, ex::value(42)) 
    		| ex::map_value(add)
    		| ex::repeat_n(10'000'000) 
    		| ex::benchmark 
    		| ex::sync_wait;

    	CHECK(result <= 28);
    }

}//namespace


