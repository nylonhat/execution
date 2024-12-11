#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "../src/bind.hpp"
#include "../src/map.hpp"
#include "../src/stay_if.hpp"
#include "../src/pure.hpp"
#include "../src/sync.hpp"
#include <print>
#include <array>
#include <algorithm>

constexpr auto add = [](auto... v) {
	return (... + v);
};


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


TEST_CASE("map"){
	auto result = ex::value(42) | ex::map_value([](auto i){return i * 2;}) | ex::sync_wait;
	CHECK(result == 84);
}


TEST_CASE("conditional test"){
	
	auto result = ex::value(5) 
		| ex::value_else_error([](auto i){
			return i > 9;
		  }) 
	    | ex::bind_error([](auto i){
			return ex::value(8);
		  })
		| ex::sync_wait;
		
	CHECK(result == 8);
}


