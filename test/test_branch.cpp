#include "doctest.h"

#include "../src/branch.hpp"
#include "../src/map.hpp"
#include "../src/pure.hpp"
#include "../src/repeat.hpp"
#include "../src/sync.hpp"
#include "../src/benchmark.hpp"
#include "../src/threadpool.hpp"
#include "../src/inline_scheduler.hpp"

#include <atomic>
#include <iostream>

namespace {

    constexpr inline auto add = [](auto... v) {
    	return (... + v);
    };

    
    template<size_t N>
    auto fib(auto& pool){
    	if constexpr(N < 2){ 
    		return ex::value(N);
    	}else{
    		return ex::branch_all(pool, fib<N-1>(pool), fib<N-2>(pool)) > add ;
    	}
    }

    struct Fetch3 {
        __attribute__((noinline))
        static auto operator()(auto... v){
            volatile std::atomic<int8_t> a = 1;
            a.fetch_sub(1);
            a.fetch_sub(1);
            a.fetch_sub(1);
            return (... + v);
        }
    };

    struct Fetch4 {
        __attribute__((noinline))
        static auto operator()(auto... v){
            volatile std::atomic<int8_t> a = 1;
            a.fetch_sub(1);
            a.fetch_sub(1);
            a.fetch_sub(1);
            a.fetch_sub(1);
            return (... + v);
        }
    };
    
    TEST_CASE("Benchmark: branch with inline scheduler"){

        const size_t iterations = 10'000'000;

        auto control = ex::value(4)
            | ex::map_value(Fetch3{})
            | ex::repeat_n_value(iterations)
            | ex::benchmark
            | ex::sync_wait;
	
    	InlineScheduler scheduler{};

    	auto result = ex::value(4)
    		| ex::branch(scheduler, ex::value(42)) 
    		| ex::map_value(add)
    		| ex::repeat_n_value(iterations) 
    		| ex::benchmark 
    		| ex::sync_wait;

    	CHECK((result/iterations) < (control/iterations));
    }

    TEST_CASE("Benchmark: branch on 1 thread"){
    	Threadpool<1> scheduler{};
	
        const size_t iterations = 10'000'000;
        
        auto control = ex::value(4)
            | ex::map_value(Fetch4{})
            | ex::repeat_n_value(iterations)
            | ex::benchmark
            | ex::sync_wait;

    	auto result = ex::value(4)
    		| ex::branch(scheduler, ex::value(42)) 
    		| ex::map_value(add)
    		| ex::repeat_n_value(iterations) 
    		| ex::benchmark 
    		| ex::sync_wait;

    	CHECK((result/iterations) <= (control/iterations));
    }

}//namespace


