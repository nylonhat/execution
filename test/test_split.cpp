#include "doctest.h"

#include "../src/pure.hpp"
#include "../src/sync.hpp"
#include "../src/map.hpp"
#include "../src/split.hpp"
#include "../src/inline_scheduler.hpp"
#include "../src/threadpool.hpp"
#include <chrono>
#include <thread>

namespace {
	
	TEST_CASE("inline scheduler split test"){
		
		auto scheduler = ex::InlineScheduler{};
		
		auto fn = [](auto split_node){
			return split_node 
			| ex::map_value([](auto i){return i + 3;});
		};
		
		auto result = ex::split(scheduler, ex::value(42), fn)
			| ex::sync_wait;
			
		CHECK(result == 45);

	}
	
	
	TEST_CASE("threadpool split test"){
		
		ex::Threadpool<16> threadpool{};
		auto scheduler = threadpool.scheduler();
		
		auto fn = [](auto split_node){
			return split_node 
			| ex::map_value([](auto i){
				return i + 3;
			});
		};
		
		auto result = ex::split(scheduler, ex::value(42), fn)
			| ex::sync_wait;
			
		CHECK(result == 45);

	}
	
	
	TEST_CASE("threadpool split test, function doesn't use split_node"){
		
		ex::Threadpool<16> threadpool{};
		auto scheduler = threadpool.scheduler();
		
		auto fn = [](auto split_node){
			return ex::value(3);
		};
		
		auto big_sender = ex::value(42) 
			| ex::map_value([](auto i){
				std::this_thread::sleep_for(std::chrono::seconds(2));
				return i + 3;
			});
		
		auto result = ex::split(scheduler, big_sender, fn)
			| ex::sync_wait;
			
		CHECK(result == 3);

	}
	
    

}//namespace


