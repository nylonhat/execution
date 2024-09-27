#include "bind.h"
#include "map.h"
#include "branch.h"
#include "pure.h"
#include "sync.h"
#include "threadpool.h"
#include <print>

Threadpool pool{1};

auto add = [](int a, int b) -> int{
	return a + b;
};

auto add5 = [](int a) -> int{
	return a + 5;
};


int main(){
	//int a = pure(12) >> pure >> pure >> pure >> pure >> pure | sync;

	//Sender auto b = branch(pool, pure(12), pure(5));
	Sender auto b = pure2(9, 11);
	Sender auto c = b >> pure2;
	Sender auto d = c > add;
	Sender auto e = d >> pure;
	Sender auto f = e > add5;
	Sender auto g = f >> pure;
	int h = g | sync_wait;

	//int g = c | sync;
	//int e = d | sync;


	return 0 ;

}
