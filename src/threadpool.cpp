#include "backoff.h"
#include "threadpool.h"
#include <random>
#include <iostream>

thread_local size_t Threadpool::worker_id = 0;
thread_local Threadpool* Threadpool::my_pool = nullptr;

Threadpool::Threadpool(int num_threads) {

	//start each worker
	for (int i=0; i<num_threads; i++){
		threads.emplace_back(&Threadpool::work, this);
	}
}

void Threadpool::work(){
	Backoff backoff;

	//assign each worker a unique id representing queue index
	my_pool = this;
	worker_id = worker_id_ticket.fetch_add(1);
	
	std::minstd_rand random_generator{std::random_device{}()};

	while(running.load()){

		OpHandle task;
		//dequeue from threads own queue first 
		if(queues.at(worker_id).try_local_pop(task)){
			task.start({});
			continue;
		}
		
		if(master_queue.try_dequeue(task)){
			task.start({});
			continue;
		}
	
		
		//try to steal from other random queues
		std::uniform_int_distribution<int> distribution(0,worker_id_ticket.load() -1);
		for(size_t i=0; i<worker_id_ticket.load()-1; i++){
			int random_index = distribution(random_generator);
		
			if(queues.at(random_index).try_steal(task)){
				task.start({});
				backoff.reset();
				break;
			}
		}

		
		if(worker_id == 0){
			continue;
		}
		
		backoff.backoff();
		
	}
}

Threadpool::~Threadpool(){
	running.store(false);
}

OpHandle Threadpool::schedule(OpHandle task){
	//return {task};
	//thread does not belong to this pool
	if(my_pool != this){
		if(master_queue.try_enqueue(task)){
			return {};
		}
		return {task};
	}

	//enqueue task into threads own queue
	if(queues.at(worker_id).try_local_push(task)){
		return {};
	}
	return {task};
}




