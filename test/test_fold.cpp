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
		
		ex::Threadpool<16> threadpool{};
		auto scheduler = threadpool.scheduler();
		
		auto add = [](auto a, auto b){
			return a + b;
		};
		
		std::size_t min = 0;
		std::size_t max = 1000000;
		
		auto sender_range = std::views::iota(min, max)
			| std::views::transform(ex::value);
			
		auto result = ex::fold_on<2>(scheduler, sender_range, ex::value, min, add)
			| ex::sync_wait;
		
		auto control = std::ranges::fold_left(std::views::iota(min, max), min, add);
		
		CHECK(result == control);
    
    }

    
	TEST_CASE("Async accumulate with bounded concurrency of 1 case"){
		
		ex::Threadpool<16> threadpool{};
		auto scheduler = threadpool.scheduler();
		
		auto add = [](auto a, auto b){
			return a + b;
		};
		
		std::size_t min = 0;
		std::size_t max = 1000000;
		
		auto sender_range = std::views::iota(min, max)
			| std::views::transform(ex::value);
			
		auto result = ex::fold_on<1>(scheduler, sender_range, ex::value, min, add)
			| ex::sync_wait;
		
		auto control = std::ranges::fold_left(std::views::iota(min, max), min, add);
		
		CHECK(result == control);
    
    }
	
	TEST_CASE("Async accumulate inline scheduler"){
		
		ex::Threadpool<0> threadpool{};
		auto scheduler = threadpool.scheduler();
		
		auto add = [](auto a, auto b){
			return a + b;
		};
		
		std::size_t min = 0;
		std::size_t max = 1000000;
		
		auto sender_range = std::views::iota(min, max)
			| std::views::transform(ex::value);
			
		auto result = ex::fold_on<2>(scheduler, sender_range, ex::value, min, add)
			| ex::sync_wait;
		
		auto control = std::ranges::fold_left(std::views::iota(min, max), min, add);
		
		CHECK(result == control);
    
    }

	TEST_CASE("Chunk accumulate"){
		
		ex::Threadpool<16> threadpool{};
		auto scheduler = threadpool.scheduler();
		
		auto add = [](auto a, auto b){
			return a + b;
		};
		
		std::size_t min = 0;
		std::size_t max = 1uz << 15uz;
		
		//Range of Senders of chunks views
		auto sender_range = std::views::iota(min, max)
		    | std::views::chunk(max/16)
			| std::views::transform(ex::value);

		//Synchronous folding
		auto fold_chunk = [=](auto chunk_view){
			return std::ranges::fold_left(chunk_view, min, add);
		};

		//Async folding on each chunk
		auto async_fold_chunk = [=](auto chunk_view){
			return ex::value(chunk_view) 
			     | ex::map_value(fold_chunk);
		};
			
		auto result = ex::fold_on<16>(scheduler, sender_range, async_fold_chunk, min, add)
			| ex::sync_wait;
		
		auto control = std::ranges::fold_left(std::views::iota(min, max), min, add);
		
		CHECK(result == control);
    
    }    
	
	
	
	TEST_CASE("Fold tail call optimisation"){
		
		ex::Threadpool<0> threadpool{};
		auto scheduler = threadpool.scheduler();
		
		auto add = [](auto a, auto b){
			return a + b;
		};
		
		std::size_t min = 0;
		std::size_t max = 10;
		
		auto sender_range = std::views::iota(min, max)
			| std::views::transform(ex::value);
			
		auto result = ex::fold_on<1>(scheduler, sender_range, ex::value, min, add)
		    | ex::repeat_n_value(10'000'000)
			| ex::sync_wait;
		
		auto control = std::ranges::fold_left(std::views::iota(min, max), min, add);
		
		CHECK(result == control);
    
    }
	
	

}//namespace


