#include "bind.h"

int main(){
	auto b = pure(12) >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure ;
	auto c = b >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure;
	auto d = c >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure;
	auto e = d >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure;
	auto f = e >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure;
	auto g = f >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure;
	auto h = g >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure;
	auto i = h >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure;
	auto j = i >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure;
	auto k = j >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure;
	auto l = k >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure;
	auto m = l >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure;
	auto n = m >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure;
	auto o = n >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure;
	auto p = o >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure;
	auto q = p >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure >> pure;
	auto r = q | sync_start;

	return r;

}
