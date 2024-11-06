#include "bind.h"
#include "repeat.h"
#include "map.h"
#include "branch.h"
#include "pure.h"
#include "sync.h"
#include "benchmark.h"
#include "threadpool.h"
#include "inline_scheduler.h"

#include "op_state.hpp"
#include "sender.hpp"
#include "recvr.hpp"

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
			return (pure(v) >=...>= ([](auto){return pure;}(I)) ); 
		};
	}(std::make_index_sequence<N>{});

	//pure(v) >= pure >= ... >= pure;
};

int main(){
	
	//auto r = pure(4) | sync_wait;
	//auto r = pure(42) | repeat | sync_wait;
	//auto r = pure2(5, 7) > add >= purez<50>() > id | sync_wait;
	//auto r = pure(42) > pure >= id | sync_wait;
	
	auto r = branch(ils, pure(42), pure(69)) 
			> add
			| repeat_n(100'000'000) 
			| benchmark 
			| ex::sync_wait;

	std::println("Final result: {}", r);

	return 0;
}
