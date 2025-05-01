#include "doctest.h"

#include "../src/fold.hpp"
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
#include <ranges>

namespace {


	TEST_CASE("Async accumulate with bounded concurrency"){
		
		ex::Threadpool<16> pool = {};
		
		auto add = [](auto a, auto b){
			return a + b;
		};
		
		std::size_t min = 0;
		std::size_t max = 1000000;
		
		
		//range of senders that send child sender
		auto sender_range = std::views::iota(min, max)
			| std::views::transform([](auto i){
				return ex::value(i) | ex::map_value(ex::value);
			});
			
		auto result = ex::fold_on<2>(pool, sender_range, min, add)
			| ex::sync_wait;
		
		auto control = std::ranges::fold_left(std::views::iota(min, max), min, add);
		
		CHECK(result == control);
    
    }

}//namespace


