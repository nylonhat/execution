#include <print>

#include "bind.h"


int main(){
	auto b = pure(5) >> pure | sync_start;
	//std::println("{}", b);

	return b;

}
