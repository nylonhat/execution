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

int main(){
	
	Threadpool<1> pool{};
	InlineScheduler ils{};
	
	auto map_test = ex::value(42)  >= ex::value > add | ex::repeat_n(10000000) | ex::sync_wait;
	auto branch_all_test = ex::branch_all(ils, ex::value(1, 2) > add) | ex::sync_wait;
	auto repeat_test = ex::value(42) | ex::repeat_n(10) | ex::sync_wait;
	auto bind_stress = ex::value(5, 7) > add >= pure_stress<10> > ex::identity | ex::sync_wait;
	auto monadic = ex::value(42) > ex::value >= ex::identity | ex::sync_wait;

	auto branch_bench = ex::value(42)
		| ex::branch(pool, ex::value(69)) 
	 	//| ex::branch(pool, ex::value(69)) 
		//| ex::branch(pool, ex::value(69)) 
		| ex::map_value(add)
		| ex::bind_value(ex::value)
		| ex::repeat_n(100'000'000) 
		| ex::benchmark 
		| ex::sync_wait;
	
	auto conditional = ex::value(5) 
		| ex::stay_if([](auto i){
			return i > 9;
		}) 
	    | ex::bind_error([](auto i){
			return ex::value(8);
		})
		| ex::sync_wait;
	
	std::println("Final result: {}", branch_bench);
	
	return 0;
}
