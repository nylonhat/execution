#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "../src/bind.hpp"
#include "../src/repeat.hpp"
#include "../src/map.hpp"
#include "../src/stay_if.hpp"
#include "../src/branch.hpp"
#include "../src/pure.hpp"
#include "../src/sync.hpp"
#include "../src/benchmark.hpp"
#include "../src/threadpool.hpp"
#include "../src/inline_scheduler.hpp"
#include "../src/inline.hpp"
#include <print>
#include <array>
#include <algorithm>

constexpr inline auto add = [](auto... v) {
	return (... + v);
};

template<size_t N>
constexpr auto pure_stress_generator(){
	//turn on link time optimisation -flto
	//value(v) >= value >= ... >= value;
	return []<size_t... I>(std::index_sequence<I...>){
		return [](auto v){
			return (ex::value(v) >=...>= ([](auto){return ex::value;}(I)) ); 
		};
	}(std::make_index_sequence<N>{});
};

template<size_t N>
inline constexpr auto pure_stress = pure_stress_generator<N>();


template<size_t N>
auto fib(auto& pool){
	if constexpr(N < 2){ 
		return ex::value(N);
	}else{
		return ex::branch_all(pool, fib<N-1>(pool), fib<N-2>(pool)) > add ;
	}
}


auto permute(){
	auto array = std::array{0,1,2,3,4,5,6,7,8,9};
	while(std::ranges::next_permutation(array).found){
		
	}
	return array.at(0);
}


TEST_CASE("monadic"){
	auto result = ex::value(42) > ex::value >= ex::identity | ex::sync_wait;
	CHECK(result == 42);
}


TEST_CASE("bind stress"){
	auto result = ex::value(5, 7) > add >= pure_stress<20> > ex::identity | ex::sync_wait;
	CHECK(result == 12);
}


TEST_CASE("map test"){
	auto result = ex::value(42) >= ex::value > add >= pure_stress<1> | ex::repeat_n(10'000'000) | ex::sync_wait;
	CHECK(result == 10'000'000);
}

TEST_CASE("repeat test"){
	auto result = ex::value(42) | ex::repeat_n(100) | ex::repeat_n(100) | ex::sync_wait;
	CHECK(result == 100);
}

TEST_CASE("conditional test"){
	
	auto result = ex::value(5) 
		| ex::stay_if([](auto i){
			return i > 9;
		  }) 
	    | ex::bind_error([](auto i){
			return ex::value(8);
		  })
		| ex::sync_wait;
		
	CHECK(result == 8);
}


TEST_CASE("branch benchmark"){
	
	InlineScheduler scheduler{};

	auto result = ex::value(4)
		| ex::branch(scheduler, ex::value(42)) 
		| ex::map_value(add)
		| ex::repeat_n(10'000'000) 
		| ex::benchmark 
		| ex::sync_wait;

	CHECK(result < 18);
}

TEST_CASE("threadpool benchmark"){
	
	Threadpool<1> scheduler{};

	auto result = ex::value(4)
		| ex::branch(scheduler, ex::value(42)) 
		| ex::map_value(add)
		| ex::repeat_n(10'000'000) 
		| ex::benchmark 
		| ex::sync_wait;

	CHECK(result < 26);
}
