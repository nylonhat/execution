#include "bind.hpp"
#include "repeat.hpp"
#include "map.hpp"
#include "stay_if.hpp"
#include "branch.hpp"
#include "pure.hpp"
#include "sync.hpp"
#include "benchmark.hpp"
#include "threadpool.hpp"
#include "inline_scheduler.hpp"
#include "inline.hpp"
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




int main(){
	
	Threadpool<0> pool{};

	auto map_test = ex::value(42) >= ex::value > add >= pure_stress<1> | ex::repeat_n(10'000'000) | ex::sync_wait;
	auto branch_all_test = ex::branch_all(pool, ex::value(1, 2) > add) | ex::sync_wait;
	auto branch_all_test2 = ex::branch_all(pool, ex::value(1), ex::value(2)) > add | ex::repeat_n(100'000'000) | ex::sync_wait;
	auto repeat_test = ex::value(42) | ex::repeat_n(10) | ex::sync_wait;
	auto bind_stress = ex::value(5, 7) > add >= pure_stress<40> > ex::identity | ex::sync_wait;
	auto monadic = ex::value(42) > ex::value >= ex::identity | ex::sync_wait;
	
	///*
	auto branch_bench = ex::value(4)
		| ex::branch(pool, ex::value(42)) 
		| ex::map_value(add)
		//| ex::bind_value(ex::value)
		| ex::repeat_n(100'000'000) 
		| ex::benchmark 
		| ex::sync_wait;
	//*/
	///*
	auto conditional = ex::value(5) 
		| ex::stay_if([](auto i){
			return i > 9;
		}) 
	    | ex::bind_error([](auto i){
			return ex::value(8);
		})
		| ex::sync_wait;
	//*/
	//auto fib_test = fib<10>(pool) | ex::sync_wait;
	
	std::println("Final result: {}", branch_all_test);

	return 0;
}
