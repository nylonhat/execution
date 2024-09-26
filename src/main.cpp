#include "bind.h"
#include "branch.h"
#include "threadpool.h"
#include <type_traits>
#include <print>

Threadpool pool{1};

auto add = [](int a, int b) -> int{
	return a + b;
};

auto add5 = [](int a) -> int{
	return a + 5;
};


int main(){
	//int a = pure(12) >> pure >> pure >> pure >> pure >> pure | sync_start;

	//Sender auto b =branch(pool, pure(12), pure(5));
	Sender auto b = pure2(9, 11);
	Sender auto c = b >> pure2;
	Sender auto d = c > add;
	Sender auto e = d >> pure;
	Sender auto f = e > add5;
	Sender auto g = f >> pure;
	int h = g | sync_start;

	//int g = c | sync_start;
	//int e = d | sync_start;


	//static_assert(std::is_same_v<typename decltype(c)::value_t, int>);

	//d.connect(noop_recvr{});

	//auto f = ((pure(3) > add5) >> pure) | sync_start;
	
	//Sender auto b = branch(pool, pure(12), pure(5)) > add5 ;

	//auto t = branch(pool, pure(12), pure(5)) > add;
	//auto u = branch(pool, t, pure(7));
	//auto o = connect(u, noop_recvr{});
	
	//std::println("{}", sizeof(o));

	return 0 ;

}
