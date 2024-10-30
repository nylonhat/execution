#include "bind.h"
#include "repeat.h"
#include "map.h"
#include "branch.h"
#include "pure.h"
#include "sync.h"
#include "threadpool.h"
#include "timer.h"

//#include <print>
//#include <iostream>

Threadpool pool{1};

auto add = [](int a, int b) -> int{
	return a + b;
};

auto add5 = [](int a){
	return a + 5;
};

auto purez = [](auto v){
	return pure(v) >= pure >= pure >= pure >= pure  >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure  >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure  >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure  >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure  >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure;
};

auto print = [](auto v){
	//std::println("{}", v);
	//std::cout << v << "\n";
	return v;
};

int main(){
	
	//Sender auto b = branch(pool, purez(19), pure(5));
	//auto r = b >= pure2 > add >= pure > add5 >= purez | sync_wait;
	//auto r = pure2(3, 6) > add | repeat | sync_wait;
	//auto r = (branch(pool, pure(42), pure(69)) > add >= purez  | repeat) | repeat | sync_wait;
	auto r = branch(pool, pure(42), pure(69)) > add | repeat | sync_wait;
	//auto r = pure2(23, 5) > add >= purez | sync_wait;
	//auto r = (pure(5) >= pure > print | repeat) > print > id | repeat | sync_wait;
	//auto r = pure(42) | repeat | sync_wait;
	//auto r = pure(42) > pure >= id | sync_wait;

	//auto r = pure(4) | sync_wait;
	//auto r = pure(5) >= purez | sync_wait;
	std::println("Final result: {}", r);
	return 0;
}
