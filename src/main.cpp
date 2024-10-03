#include "bind.h"
#include "repeat.h"
#include "map.h"
#include "branch.h"
#include "pure.h"
#include "sync.h"
#include "threadpool.h"
#include "timer.h"

#include <print>
#include <iostream>

Threadpool pool{1};

auto add = [](int a, int b) -> int{
	return a + b;
};

auto add5 = [](int a){
	return a + 5;
};

auto purez = [](auto v){
	Sender auto s = pure(v) >= pure >= pure >= pure >= pure  >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure >= pure>= pure >= pure >= pure >= pure >= pure>= pure >= pure >= pure >= pure >= pure>= pure >= pure >= pure >= pure >= pure;
	return s >= pure >= pure >= pure >= pure >= pure >= pure>= pure >= pure >= pure >= pure >= pure>= pure >= pure >= pure >= pure >= pure>= pure >= pure >= pure >= pure >= pure >=pure >= pure;
};

auto print = [](auto v){
	//std::println("{}", v);
	std::cout << v;
	return v;
};

int main(){
	
	//Sender auto b = branch(pool, purez(19), pure(5));
	//auto r = b >= pure2 > add >= pure > add5 >= purez | sync_wait;

	auto r = branch(pool, pure(42), pure(69)) > add >= purez | repeat | sync_wait;
	//auto r = pure2(11, 23) > add | repeat | sync_wait;

	std::println("{}", r);
	return 0;
}
