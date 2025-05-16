#include "doctest.h"

#include "../src/branch.hpp"
#include "../src/map.hpp"
#include "../src/pure.hpp"
#include "../src/bind.hpp"

#include "../src/repeat.hpp"
#include "../src/sync.hpp"
#include "../src/benchmark.hpp"
#include "../src/threadpool.hpp"
#include "../src/inline_scheduler.hpp"

#include <atomic>
#include <iostream>

namespace test{

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
            a.fetch_sub(1);
            return (... + v);
        }
    };
    
	
    TEST_CASE("Benchmark: branch with inline scheduler"){

    	ex::Threadpool<0> threadpool{};
		auto scheduler = threadpool.scheduler();
        
		const size_t iterations = 10'000'000;

        auto control = ex::value(4)
            | ex::map_value(Fetch3{})
            | ex::repeat_n_value(iterations)
            | ex::benchmark
            | ex::sync_wait;
	

    	auto result = ex::value(4)
    		| ex::branch(scheduler, ex::value(42)) 
    		| ex::map_value(add)
    		| ex::repeat_n_value(iterations) 
    		| ex::benchmark 
    		| ex::sync_wait;

    	CHECK((result/iterations) < (control/iterations));
    }
	

    TEST_CASE("Benchmark: branch on 1 thread"){
    	ex::Threadpool<1> threadpool{};
		auto scheduler = threadpool.scheduler();
	
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
	
	TEST_CASE("branch multiple times"){
    	ex::Threadpool<4> threadpool{};
		auto scheduler = threadpool.scheduler();
	
    	auto result = ex::value(4)
    		| ex::branch(scheduler, ex::value(1))
			| ex::branch(scheduler, ex::value(42)) 
			| ex::branch(scheduler, ex::value(69)) 
			| ex::branch(scheduler, ex::value(111)) 
    		| ex::map_value(add)
    		| ex::sync_wait;
			
		auto control = 4+1+42+69+111;

    	CHECK(result == control);
    }
	
	

	TEST_CASE("Schedule Sender"){
		
    	ex::Threadpool<1> threadpool{};
		auto scheduler = threadpool.scheduler();
		
		auto result = scheduler.sender()
			| ex::map_value([](){return 4;})
			| ex::sync_wait;

		CHECK(result == 4);
	}
	

	TEST_CASE("Schedule Error"){
		
    	ex::Threadpool<1> threadpool{};
		auto scheduler = threadpool.scheduler();
		
		auto result = scheduler.sender()
			| ex::bind_value([](){return ex::value(10);})
			| ex::bind_error([](){return ex::value(6);})
			| ex::sync_wait;

		CHECK(result == 10);
	}
	

	TEST_CASE("Start on"){
    	ex::Threadpool<1> threadpool{};
		auto scheduler = threadpool.scheduler();
		
		auto start_on = [](auto& scheduler, auto sender){
			return scheduler.sender()
				| ex::bind_value([=](){return sender;})
				| ex::bind_error([=](){return sender;});
		};

		auto a = start_on(scheduler, ex::value(47));
		auto result = a
			| ex::sync_wait; 
		CHECK(result == 47);
	}

	TEST_CASE("start on efficent"){
    	ex::Threadpool<1> threadpool{};
		auto scheduler = threadpool.scheduler();
		
		auto start_on = [](auto& scheduler, auto sender){
			return scheduler.sender()
				| ex::bind_error([](){return ex::value();})
				| ex::bind_value([=](){return sender;});
		};
		
		auto result = start_on(scheduler, ex::value(5)) | ex::sync_wait;
		
		CHECK(result == 5);
	}

	
	TEST_CASE("branch tail recursion"){
    	ex::Threadpool<0> threadpool{};
		auto scheduler = threadpool.scheduler();
	
    	auto result = ex::value(4)
    		| ex::branch(scheduler, ex::value(1))
			| ex::branch(scheduler, ex::value(42)) 
			| ex::branch(scheduler, ex::value(69)) 
			| ex::branch(scheduler, ex::value(111)) 
    		| ex::map_value(add)
    		| ex::repeat_n_value(1000001)
    		| ex::sync_wait;
			
		auto control = 4+1+42+69+111;

    	CHECK(result == control);
    }
	
	


}//namespace


