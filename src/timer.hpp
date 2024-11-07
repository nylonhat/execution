#ifndef TIMER_H
#define TIMER_H

#include <chrono>

struct Timer {
	std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
	std::chrono::time_point<std::chrono::steady_clock> stop_time = std::chrono::steady_clock::now();

	void start(){
		start_time = std::chrono::steady_clock::now();
	}

	void stop(){
		stop_time = std::chrono::steady_clock::now();
	}

	auto count(){
		return (stop_time - start_time).count();
	}

};

#endif
