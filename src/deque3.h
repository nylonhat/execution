#ifndef DEQUE_H
#define DEQUE_H

#include <limits>
#include <bit>
#include <array>
#include <atomic>

//Lock free Bounded Workstealing Deque

template<typename T, size_t buffer_size>
requires
	(buffer_size > 1) &&
	(std::has_single_bit(buffer_size)) //power of 2

class Deque{
public:
	Deque(){
		for(size_t i = 0; i < buffer_size; i++){
			buffer[i].steal_sequence.store(i, m::relaxed);
			buffer[i].push_sequence.store(i, m::relaxed);
		}
	}
	
	~Deque() = default;

	bool try_local_push(T const& data){
		Cell& cell = buffer[stack_id & buffer_mask];
		size_t seq = cell.push_sequence.load(m::acquire);
		
		if(seq != stack_id){
			//Deque is full
			return false;
		}

		cell.data = data;
		cell.push_sequence.store(stack_id + 1, m::relaxed);
		cell.steal_sequence.store(stack_id + 1, m::release);
		stack_id++;
		return true;
	}
	
	bool try_local_pop(T& data){
		Cell& cell = buffer[(stack_id - 1) & buffer_mask];
		
		auto test_seq = stack_id;
		if(!cell.steal_sequence.compare_exchange_strong(test_seq, stack_id - 1, m::acq_rel)){
			//Deque is empty
			return false;
		}
		
		data = cell.data;
		cell.push_sequence.store(stack_id - 1, m::relaxed);
		stack_id--;
		return true;
	}
	
	bool try_steal(T& data){
		Cell* cell;	
		auto old_steal = steal_id.load(m::acquire);
		
		for(;;){
			cell = &buffer[old_steal & buffer_mask];
			auto test_steal = old_steal + 1;
			
			//Race to steal item
			cell->steal_sequence.compare_exchange_strong(test_steal, old_steal + 1 + buffer_mask, m::acq_rel);

			//Circular Difference
			auto diff = (intptr_t)test_steal - (intptr_t)(old_steal+1);
			
			if(diff < 0){
				//Deque empty
				return false;
			}

			test_steal = old_steal;
			//Stealers won against popper. Update the steal id.
			steal_id.compare_exchange_strong(test_steal, old_steal + 1, m::acq_rel);
			
			if(diff > 0){
				//Lagging behind; retry
				old_steal = steal_id.load(m::acquire);
				continue;
			}

			//Won the race;
			break;
		}

		data = cell->data;
		cell->push_sequence.store(old_steal + 1 + buffer_mask, m::release);
		//Pusher signalled
		return true;
	}

private:
	struct alignas(64) Cell{
		std::atomic<size_t> steal_sequence;
		std::atomic<size_t> push_sequence;
		T data;
	};
	
	std::array<Cell, buffer_size> buffer;
	size_t const buffer_mask = buffer_size - 1;
	alignas(64) size_t stack_id = 0;
	alignas(64) std::atomic<size_t> steal_id = 0;

	Deque(Deque const&) = delete;
	void operator= (Deque const&) = delete;

	using m = std::memory_order;
};

#endif
