#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "../src/bind.hpp"
#include "../src/map.hpp"
#include "../src/pure.hpp"
#include "../src/sync.hpp"


TEST_CASE("monadic"){
	auto result = ex::value(42) > ex::value >= ex::identity | ex::sync_wait;
	CHECK(result == 42);
}






