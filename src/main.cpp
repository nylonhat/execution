#include "bind.h"
#include "repeat.h"
#include "map.h"
#include "branch.h"
#include "pure.h"
#include "sync.h"
#include "benchmark.h"
#include "threadpool.h"
#include "inline_scheduler.h"
#include <print>

Threadpool pool{1};
InlineScheduler ils{};

static auto add = [](int a, int b) -> int{
	return a + b;
};

template<size_t N>
auto purez(){
	return []<size_t... I>(std::index_sequence<I...>){
		return [](auto v){
			return (ex::pure(v) >=...>= ([](auto){return ex::pure;}(I)) ); 
		};
	}(std::make_index_sequence<N>{});

	//pure(v) >= pure >= ... >= pure;
};

int main(){
	
	//auto r = pure(4) | sync_wait;
	//auto r = pure(42) | repeat | sync_wait;
	//auto r = pure2(5, 7) > add >= purez<50>() > id | sync_wait;
	//auto r = pure(42) > pure >= id | sync_wait;
	
	auto r = ex::branch(ils, ex::pure(42), ex::pure(69)) 
			> add
			>= ex::pure
			| ex::repeat_n(100'000'000) 
			| ex::benchmark 
			| ex::sync_wait;

	std::println("Final result: {}", r);

	return 0;
}
