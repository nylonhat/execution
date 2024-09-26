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
	(buffer_size < std::numeric_limits<size_t>::max()/2) && //circular dif
	(std::has_single_bit(buffer_size)) //power of 2

class Deque{
public:
	Deque(){
		for(size_t i = 0; i < buffer_size; i++){
			buffer[i].sequence.store(i, m::relaxed);
		}
	}
	
	~Deque() = default;

	bool try_local_push(T const& data){
		Cell& cell = buffer[stack_id & buffer_mask];
		size_t seq = cell.sequence.load(m::acquire);
		
		if(seq != stack_id){
			//Deque is full
			return false;
		}

		cell.data = data;
		cell.sequence.store(stack_id + 1, m::release);
		stack_id++;
		return true;
	}

	bool try_local_pop(T& data){
		Cell& cell = buffer[(stack_id - 1) & buffer_mask];
		
		//Stop any further stealers, while also checking if deque is emtpy
		size_t old_seq = stack_id;
		if(!cell.sequence.compare_exchange_strong(old_seq, stack_id - 1, m::acq_rel)){
			//Deque is empty
			return false;
		}

		//Try to race for last item
		size_t old_steal = stack_id - 1;
		if(steal_id.compare_exchange_strong(old_steal, stack_id, m::relaxed)){
			//Can take last item as if a stealer
			data = cell.data;
			cell.sequence.store(old_steal + buffer_size, m::release);
			return true;
		}
		
		if(old_steal == stack_id){
			//Lost race for last item; deque now empty
			//Note: stealer will correct the cell sequence
			return false;
		}
	
		//Item was not the last; free to take
		data = cell.data;
		stack_id--;
		return true;
	}

	bool try_steal(T& data){
		Cell* cell;
		size_t old_steal = steal_id.load(m::relaxed);

		for(;;){
			cell = &buffer[old_steal & buffer_mask];
			size_t seq = cell->sequence.load(m::acquire);
			
			//Circular difference
			intptr_t dif = (intptr_t)seq - (intptr_t)(old_steal + 1);

			if(dif < 0){
				//Deque is empty
				return false;
			}
			
			if(dif > 0){
				//We are lagging behind other stealers; try again
				old_steal = steal_id.load(m::relaxed);
				continue;
			}
			
			//Race to steal item
			if(steal_id.compare_exchange_weak(old_steal, old_steal + 1, m::relaxed)){
				break;
			}
			//Lost race; try again
		}

		data = cell->data;
		cell->sequence.store(old_steal + buffer_size, m::release);
		return true;
	}

private:
	struct alignas(64) Cell{
		std::atomic<size_t> sequence;
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
