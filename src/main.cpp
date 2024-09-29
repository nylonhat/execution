#include "bind.h"
#include "map.h"
#include "branch.h"
#include "pure.h"
#include "sync.h"
#include "threadpool.h"

Threadpool pool{1};

auto add = [](int a, int b) -> int{
	return a + b;
};

auto add5 = [](int a) -> int{
	return a + 5;
};

auto purez = [](auto v){
	Sender auto s = pure(v) >= pure >= pure >= pure >= pure;  
	return s >= pure >= pure >= pure >= pure >= pure >= pure;
};

int main(){
	
	Sender auto b = branch(pool, purez(12), pure(5));
	auto c = b >= pure2 > add >= pure > add5 >= purez | sync_wait;
	
	return 0;
}
