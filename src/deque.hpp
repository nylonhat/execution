#ifndef DEQUE_H
#define DEQUE_H

#include <array>
#include <atomic>
#include <bit>

/** @brief Lock free Bounded Workstealing Deque
 *
 * One end of the deque behaves as a LIFO stack for a single
 * producer / consumer. The other end behaves like a FIFO queue
 * for multiple consumers.
 */
template<typename T, size_t buffer_size>
requires (buffer_size > 1)
	&& (std::has_single_bit(buffer_size)) //power of 2

class Deque {
private:
	struct alignas(64) Cell {
		std::atomic<std::size_t> steal_tag;
		std::atomic<std::size_t> stack_tag;
		T data;
	};
	
	static constexpr size_t buffer_mask = buffer_size - 1;
	
	std::array<Cell, buffer_size> buffer;
	
	alignas(64) std::size_t stack_index = 0;
	alignas(64) std::atomic<std::size_t> steal_index = 0;

	using m = std::memory_order;

public:
	Deque(){
		for(size_t i = 0; i < buffer_size; ++i){
			buffer[i].steal_tag.store(i, m::relaxed);
			buffer[i].stack_tag.store(i, m::relaxed);
		}
	}
	
	Deque(const Deque&)            = delete;
	Deque(Deque&&)                 = delete;
	Deque& operator=(const Deque&) = delete;
	Deque& operator=(Deque&&)      = delete;
	
	~Deque() = default;
	
	bool try_local_push(const T& data){
		auto& cell = buffer[stack_index & buffer_mask];
		auto stack_tag = cell.stack_tag.load(m::acquire);
		
		if(stack_tag != stack_index){
			//Deque is full
			return false;
		}
		
		//Push item
		cell.data = data;
		cell.stack_tag.store(stack_index + 1, m::relaxed);
		cell.steal_tag.store(stack_index + 1, m::release);
		stack_index++;
		return true;
	}
	
	bool try_local_pop(T& data){
		auto& cell = buffer[(stack_index - 1) & buffer_mask];
		
		auto test_tag = stack_index;
		cell.steal_tag.compare_exchange_strong(
			test_tag, stack_index - 1, m::acq_rel
		);
		
		if(test_tag != stack_index){
			//Deque is full
			return false;
		}
		
		data = cell.data;
		cell.stack_tag.store(stack_index - 1, m::relaxed);
		stack_index--;
		return true;
	}
	
	bool try_steal(T& data){
		for(;;){
			const auto steal_now = steal_index.load(m::acquire);
			const auto steal_key = steal_now + 1;
			const auto next_tag = steal_key + buffer_mask;
			auto& cell = buffer[steal_now & buffer_mask];
			
			//Race to steal item
			auto test_tag = steal_key;
			const auto won_race = cell.steal_tag.compare_exchange_strong(
				test_tag, next_tag, m::acq_rel
			);

			if(test_tag == steal_now){
				//Deque is empty
				return false;
			}

			//A stealer won. Help each other update steal_index
			test_tag = steal_now;
			steal_index.compare_exchange_strong(
				test_tag, steal_key, m::acq_rel
			);
		
			if(!won_race){
				//We lost but there maybe more to steal. 
				continue;
			}

			//We won the race!
			data = cell.data;
			cell.stack_tag.store(next_tag, m::release);
			return true;
		}
	}

};

#endif
