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

static auto add = [](auto... v) {
	return (... + v);
};

template<size_t N>
constexpr auto purez_t(){
	return []<size_t... I>(std::index_sequence<I...>){
		return [](auto v){
			return (ex::value(v) >=...>= ([](auto){return ex::value;}(I)) ); 
		};
	}(std::make_index_sequence<N>{});
	//make sure to have link time optimisation on
	//pure(v) >= pure >= ... >= pure;
};

template<size_t N>
inline constexpr auto purez = purez_t<N>();

int main(){
	
	Threadpool pool{1};
	InlineScheduler ils{};

	auto map_test = ex::value(7, 3) > add | ex::sync_wait;
	auto repeat_test = ex::value(42) | ex::repeat_n(10) | ex::sync_wait;
	auto bind_stress = ex::value(5, 7) > add >= purez<20> > ex::identity | ex::sync_wait;
	auto monadic = ex::value(42) > ex::value >= ex::identity | ex::sync_wait;

	auto branch_bench = ex::value(42)
		| ex::branch(ils, ex::value(69)) 
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
	
	auto multibranch = ex::value(3)
		| ex::branch(ils, ex::value(4))
		| ex::branch(ils, ex::value(5)) 
		| ex::branch(ils, ex::value(6)) 
		| ex::map_value(add) 
		| ex::bind_value(ex::value)
		| ex::sync_wait;
		
	std::println("Final result: {}", branch_bench);

	return 0;
}
