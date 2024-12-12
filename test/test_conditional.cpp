#include "doctest.h"

#include "../src/conditional.hpp"
#include "../src/sync.hpp"
#include "../src/pure.hpp"
#include "../src/bind.hpp"

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

