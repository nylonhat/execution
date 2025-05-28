#include "doctest.h"

#include "../src/bind.hpp"
#include "../src/map.hpp"
#include "../src/pure.hpp"
#include "../src/sync.hpp"

namespace{

    constexpr inline auto pure_stress_generator = []<size_t N>(){
    	//turn on link time optimisation -flto
    	//value(v) >= value >= ... >= value;
    	return []<size_t... I>(std::index_sequence<I...>){
    		return [](auto v){
    			return (ex::value(v) >=...>= ([](auto){return ex::value;}(I)) ); 
    		};
    	}(std::make_index_sequence<N>{});
    };

    template<size_t N>
    inline constexpr auto pure_stress = pure_stress_generator.operator()<N>();

    
    TEST_CASE("bind stress"){
    	auto result = (ex::value(5, 7) > ex::add >= pure_stress<20> > ex::identity) | ex::sync_wait;
    	CHECK(result == 12);
    }

    
    TEST_CASE("monadic"){
    	auto result = (ex::value(42) > ex::value >= ex::identity) | ex::sync_wait;
    	CHECK(result == 42);
    }


}//namespace
