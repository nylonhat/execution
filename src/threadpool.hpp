#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <atomic>
#include <thread>
#include <utility>
#include <array>
#include <random>
#include <bit>

#include "backoff.hpp"
#include "queue.hpp"
#include "deque.hpp"
#include "scheduler.hpp"


template<std::size_t thread_count>
struct Threadpool {
private:
	std::atomic<bool> running{true};
	std::array<std::jthread, thread_count> threads;
	std::array<Deque<OpHandle,4>, thread_count> local_queues{};
	Queue<OpHandle, std::bit_ceil(thread_count*2)> common_queue{};

	std::atomic<size_t> worker_count = 0;
	inline thread_local static size_t worker_id = 0;
	inline thread_local static Threadpool* parent_threadpool = nullptr;
	
public:
	//Constructor
	Threadpool(){
		for (auto& thread : threads){
			thread = std::jthread(&Threadpool::work, this);
		}
	}

	//Destructor
	~Threadpool(){
		running.store(false);
	}
	
private:	
	void work(){
		
		//Backoff backoff;

		//assign each worker a unique id representing queue index
		parent_threadpool = this;
		worker_id = worker_count.fetch_add(1);

		//create random distribution for stealing
		std::minstd_rand random_generator{std::random_device{}()};
		std::uniform_int_distribution<int> distribution(0, thread_count - 1);

		while(running.load()){

			OpHandle task;
			//dequeue from threads own queue first 
			if(local_queues.at(worker_id).try_local_pop(task)){
				task.start();
				continue;
			}
		
			if(common_queue.try_dequeue(task)){
				task.start();
				continue;
			}
	
			//try to steal from other random queues
			for(size_t i = 0; i < thread_count; i++){
				int random_index = distribution(random_generator);
		
				if(local_queues.at(random_index).try_steal(task)){
					task.start();
					//backoff.reset();
					break;
				}
			}

			//if(worker_id == 0){
			//	continue;
			//}
		
			//backoff.backoff();
		
		}		
	}

public:
	bool try_schedule(OpHandle task){
		//thread does not belong to this pool
		if(parent_threadpool != this){
			return common_queue.try_enqueue(task);
		}

		//enqueue task into threads own queue
		return local_queues.at(worker_id).try_local_push(task);
	}
	
};


template<>
struct Threadpool<0> {
	
	bool try_schedule(auto&&...){
		return false;
	}	
};

#endif
