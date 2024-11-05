#include "bind.h"
#include "repeat.h"
#include "map.h"
#include "branch.h"
#include "pure.h"
#include "sync.h"
#include "threadpool.h"
#include "inline_scheduler.h"
#include "timer.h"

#include <print>
//#include <iostream>

Threadpool pool{1};
InlineScheduler ils{};

auto add = [](int a, int b) -> int{
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
	
	//Sender auto b = branch(pool, purez(19), pure(5));
	//auto r = b >= pure2 > add >= pure > add5 >= purez | sync_wait;
	//auto r = pure2(3, 6) > add | repeat | sync_wait;
	//auto r = (branch(pool, pure(42), pure(69)) > add >= purez  | repeat) | repeat | sync_wait;
	//auto r = branch(pool, pure(42), pure(69)) > add | repeat | sync_wait;
	//auto r = pure2(23, 5) > add >= purez | sync_wait;
	//auto r = (pure(5) >= pure > print | repeat) > print > id | repeat | sync_wait;
	//auto r = pure(42) | repeat | sync_wait;
	//auto r = pure(42) > pure >= id | sync_wait;

	//auto r = pure(4) | sync_wait;
	auto r = pure2(5, 7) > add >= purez<50>() > id | sync_wait;
	std::println("Final result: {}", r);
	return 0;
}
