#include "doctest.h"
#include "../src/branch.hpp"
#include "../src/map.hpp"
#include "../src/pure.hpp"
#include "../src/repeat.hpp"
#include "../src/sync.hpp"
#include "../src/benchmark.hpp"
#include "../src/threadpool.hpp"
#include "../src/inline_scheduler.hpp"
#include "../src/timer.hpp"
#include <atomic>
#include <iostream>

namespace {

    constexpr inline auto add = [](auto... v) {
    	return (... + v);
    };

    struct fetch {
        __attribute__((noinline))
        static auto operator()(auto... v){
            volatile std::atomic<int8_t> a = 1;
            a.fetch_sub(1);
            a.fetch_sub(1);
            a.fetch_sub(1);
            return (... + v);
        }
    };

    
    TEST_CASE("branch: simple fork join with inline scheduler benchmark"){

        const size_t iterations = 10'000'000;

        
        auto control = ex::value(4)
            | ex::map_value(fetch{})
            | ex::repeat_n(iterations)
            | ex::benchmark
            | ex::sync_wait;
	
    	InlineScheduler scheduler{};

    	auto result = ex::value(4)
    		| ex::branch(scheduler, ex::value(42)) 
    		| ex::map_value(add)
    		| ex::repeat_n(iterations) 
    		| ex::benchmark 
    		| ex::sync_wait;

    	CHECK(result == control);
    }

    // TEST_CASE("branch on threadpool 1 thread benchmark"){
    // 	Threadpool<1> scheduler{};
	
    //     const size_t iterations = 10'000'000;
    //     // Deque<size_t, 4> deque{};
        
    //     // Timer timer{};
    //     // timer.start();
    //     // for(size_t i = 0; i < iterations; i++){
    //     //     std::atomic<int8_t> ref_count = 1;
    //     //     ref_count.fetch_sub(1);
    //     //     deque.try_local_push(1);
    //     //     size_t data;
    //     //     deque.try_local_pop(data);
    //     //     ref_count.fetch_sub(1);
    //     // }
    //     // timer.stop();
    //     // auto control = (timer.count()/iterations);

    //     // auto warm = ex::value(4)
    //     //     | ex::map_value([](auto i){
    //     //         std::atomic<int8_t> a = 1;
    //     //         a.fetch_sub(1);
    //     //         a.fetch_sub(1);
    //     //         return i;               
    //     //     })
    //     //     | ex::repeat_n(iterations)
    //     //     | ex::benchmark
    //     //     | ex::sync_wait;

    //     // auto control = ex::value(4)
    //     //     | ex::map_value([](auto i){
    //     //         std::atomic<int8_t> a = 1;
    //     //         a.fetch_sub(1);
    //     //         a.fetch_sub(1);
    //     //         return i;               
    //     //     })
    //     //     | ex::repeat_n(iterations)
    //     //     | ex::benchmark
    //     //     | ex::sync_wait;

    // 	auto result = ex::value(4)
    // 		| ex::branch(scheduler, ex::value(42)) 
    // 		| ex::map_value(add)
    // 		| ex::repeat_n(iterations) 
    // 		| ex::benchmark 
    // 		| ex::sync_wait;

    // 	// CHECK(result <= 0);
    // }

}//namespace


