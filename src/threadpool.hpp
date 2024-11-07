#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <atomic>
#include <thread>
#include <utility>
#include <vector>
#include <array>

#include "queue.hpp"
#include "deque.hpp"
#include "scheduler.hpp"

struct Threadpool {
private:
	std::atomic<bool> running{true};
	std::vector<std::jthread> threads;
	thread_local static size_t worker_id;
	thread_local static Threadpool* my_pool;

	std::atomic<size_t> worker_id_ticket = 0;
	std::array<Deque<OpHandle,4>, 16> queues{};
	Queue<OpHandle, 16> master_queue{};

public:
	//Constructor
	Threadpool(int num_threads);

	//Destructor
	~Threadpool();
	
private:	
	void work();

public:
	OpHandle schedule(OpHandle task);
	bool try_schedule(OpHandle task);
	

};

#endif
