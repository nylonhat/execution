#ifndef QUEUE_H
#define QUEUE_H

#include <limits>
#include <bit>
#include <array>
#include <atomic>

//Lock free MPMC Bounded Queue
//Modified from Dmitry Vyukov's implimentation
//https://www.1024cores.net/home/lock-free-algorithms/Queues/bounded-mpmc-Queue

template<typename T, size_t buffer_size>
requires
	(buffer_size > 1) &&
	(buffer_size < std::numeric_limits<size_t>::max()/2) && //circular dif
	(std::has_single_bit(buffer_size)) //power of 2

class Queue{
public:
	Queue()	{
		for(size_t i = 0; i != buffer_size; i += 1){
			buffer[i].sequence.store(i, m::relaxed);
		}
	}
	
	~Queue() = default;

	bool try_enqueue(T const& data){
		Cell* cell;
		size_t pos = enqueue_pos.load(m::relaxed);

		for(;;){
			cell = &buffer[pos & buffer_mask];
			size_t seq = cell->sequence.load(m::acquire);
			
			//Circular difference
			intptr_t dif = (intptr_t)seq - (intptr_t)pos;
			
			if(dif < 0){
				return false;
			}
			
			if(dif > 0){
				pos = enqueue_pos.load(m::relaxed);
				continue;
			}
			
			if(enqueue_pos.compare_exchange_weak(pos, pos + 1, m::relaxed)){
				break;
			}
		}
		
		cell->data = data;
		cell->sequence.store(pos + 1, m::release);
		return true;
	}

	bool try_dequeue(T& data){
		Cell* cell;
		size_t pos = dequeue_pos.load(m::relaxed);

		for(;;){
			cell = &buffer[pos & buffer_mask];
			size_t seq = cell->sequence.load(m::acquire);
			intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
			
			if(dif < 0){
				return false;
			}
			
			if(dif > 0){
				pos = dequeue_pos.load(m::relaxed);
				continue;
			}
			
			if(dequeue_pos.compare_exchange_weak(pos, pos + 1, m::relaxed)){
				break;
			}
		}
		
		data = cell->data;
		cell->sequence.store(pos + buffer_mask + 1, m::release);
		return true;
	}

private:
	struct alignas(64) Cell{
		std::atomic<size_t> sequence;
		T data;
	};
	
	std::array<Cell, buffer_size> buffer;
	size_t const buffer_mask = buffer_size - 1;
	alignas(64) std::atomic<size_t>  enqueue_pos = 0;
	alignas(64) std::atomic<size_t> dequeue_pos = 0;

	Queue(Queue const&) = delete;
	void operator= (Queue const&) = delete;
	
	using m = std::memory_order;
}; 

#endif
