#ifndef BACKOFF_H
#define BACKOFF_H

#include <immintrin.h>
#include <algorithm>
#include <random>
#include <thread>

using namespace std::literals::chrono_literals;

struct alignas(64) Backoff {
	std::minstd_rand random_generator{std::random_device{}()};
	const int min_backoff_count = 0;
	const int max_backoff_count = 9;
	
	int backoff_count = 0;


	void backoff() noexcept{
		if (isMaxBackoff()){
			std::this_thread::sleep_for(1ms);
			return;
		}

		std::uniform_int_distribution<int> distribution(0, (1<<backoff_count)-1);
		int random_iterations = distribution(random_generator);
		
		for (int i = 0; i < random_iterations; i++){
			_mm_pause();
		}

		backoff_count = std::min(max_backoff_count, backoff_count + 1);
	}

	void reset() noexcept{
		backoff_count = min_backoff_count;
	}

	void easein() noexcept{
		if (isMinBackoff()){
			return;
		}

		std::uniform_int_distribution<int> distribution(0, (1<<backoff_count)-1);
		int random_iterations = distribution(random_generator);
		
		for (int i = 0; i < random_iterations; i++){
			_mm_pause();
		}

		backoff_count = std::max(min_backoff_count, backoff_count - 1);
	}

	bool isMaxBackoff() noexcept{
		return backoff_count == max_backoff_count;
	}
	
	bool isMinBackoff() noexcept{
		return backoff_count == min_backoff_count;
	}


};

#endif
