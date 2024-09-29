#include "bind.h"
#include "map.h"
#include "branch.h"
#include "pure.h"
#include "sync.h"
#include "threadpool.h"
#include <print>

#include <type_traits>
Threadpool pool{1};


auto add = [](int a, int b) -> int{
	return a + b;
};

auto add5 = [](int a) -> int{
	return a + 5;
};

auto purez = [](auto v){
	return pure(v) >> pure >> pure >> pure >> pure >> pure >> pure;

};

int main(){
	int a = purez(3) | sync_wait;
	
	Sender auto b = branch(pool, pure(12), pure(5));
	Sender auto c = b >> pure2;
	Sender auto d = c > add;
	Sender auto e = d >> pure;
	Sender auto f = e > add5;
	Sender auto g = f >> pure;
	int h = g | sync_wait;

	return h;

}
