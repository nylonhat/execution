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
#include <print>

static auto add = [](int a, int b) -> int{
	return a + b;
};

template<size_t N>
constexpr auto purez_t(){
	return []<size_t... I>(std::index_sequence<I...>){
		return [](auto v){
			return (ex::pure(v) >=...>= ([](auto){return ex::pure;}(I)) ); 
		};
	}(std::make_index_sequence<N>{});
	//make sure to have link time optimisation on
	//pure(v) >= pure >= ... >= pure;
};

template<size_t N>
inline constexpr auto purez = purez_t<N>();

int main(){
	
	Threadpool pool{1};
	//InlineScheduler ils{};

	//auto r = ex::pure(7) | ex::sync_wait;
	//auto r = pure(42) | ex::repeat | ex::sync_wait;
	auto r = ex::pure(5, 7) > add >= purez<20> > ex::identity | ex::sync_wait;
	//auto r = ex::pure(42) > ex::pure >= ex::identity | ex::sync_wait;

	/*
	auto r = ex::branch(ils, ex::pure(42), ex::pure(69)) 
			> add
			>= ex::pure
			| ex::repeat_n(100'000'000) 
			//| ex::benchmark 
			| ex::sync_wait;
	*/
	/*
	auto r = ex::pure(5) 
			| ex::stay_if([](auto i){
			    return i > 9;
			  }) 
	        | ex::bind_error([](auto i){
	        	return ex::pure(8);
	          })
			| ex::sync_wait;
	*/
	std::println("Final result: {}", r);


	return 0;
}
