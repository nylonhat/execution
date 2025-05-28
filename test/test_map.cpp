#include "doctest.h"

#include "../src/map.hpp"
#include "../src/pure.hpp"
#include "../src/sync.hpp"

namespace {

    auto times_two = [](auto i){
        return i * 2;
    };

    TEST_CASE("map times 2"){
    	auto result = (ex::value(42) > times_two) | ex::sync_wait;
    	CHECK(result == 84);
    }

}//namespace
