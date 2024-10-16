#ifndef DEQUE_H
#define DEQUE_H

#include <limits>
#include <bit>
#include <array>
#include <atomic>

//Lock free Bounded Workstealing Deque

template<typename T, uint32_t buffer_size>
requires
	(buffer_size > 1) &&
	(std::has_single_bit(buffer_size)) //power of 2

class Deque{
public:
	Deque(){
		for(uint64_t i = 0; i < buffer_size; i++){
			buffer[i].sequence.store(i, m::relaxed);
		}
	}
	
	~Deque() = default;

	bool try_local_push(T const& data){
		Cell& cell = buffer[stack_id & buffer_mask];
		auto raw_seq = cell.sequence.load(m::acquire);
		auto [epoch, seq] = unpack(raw_seq);

		if(seq != stack_id){
			//Deque is full
			return false;
		}

		cell.data = data;
		cell.sequence.store(pack(epoch, seq+1), m::release);
		stack_id++;
		return true;
	}
	
	bool try_local_pop(T& data){
		Cell& cell = buffer[(stack_id - 1) & buffer_mask];
		auto raw_seq = cell.sequence.load(m::acquire);
		auto [epoch, seq] = unpack(raw_seq);
		
		uint64_t expected = pack(epoch, stack_id);
		uint64_t desired = pack(epoch + 1, seq - 1);
		if(!cell.sequence.compare_exchange_strong(expected, desired, m::acq_rel)){
			//Empty
			return false;
		}

		data = cell.data;
		stack_id--;
		return true;
	}
	
	

	bool try_steal(T& data){
		Cell* cell;
		auto old_steal = steal_id.load(m::relaxed);

		for(;;){
			cell = &buffer[old_steal & buffer_mask];
			auto raw_seq = cell->sequence.load(m::acquire);
			auto [epoch, seq] = unpack(raw_seq);
			
			//Circular difference
			int32_t dif = std::bit_cast<int32_t>(seq) - std::bit_cast<int32_t>(old_steal + 1);

			if(dif < 0){
				//Deque is empty
				return false;
			}
			
			if(dif > 0){
				//We are lagging behind other stealers; try again
				old_steal = steal_id.load(m::relaxed);
				continue;
			}
			
			//Preemptive read
			data = cell->data;
		
			uint64_t expected = raw_seq;
			uint64_t desired = pack(epoch, seq + buffer_mask);
			//Race to steal item
			if(cell->sequence.compare_exchange_weak(expected, desired, m::acq_rel)){
				//won race
				break;
			}
			//Lost race; try again
		}
		
		steal_id.fetch_add(1, m::relaxed);
		return true;
	}

private:
	struct alignas(64) Cell{
		std::atomic<uint64_t> sequence;
		T data;
	};
	
	std::array<Cell, buffer_size> buffer;
	static constexpr uint32_t buffer_mask = buffer_size - 1;

	alignas(64) uint32_t stack_id = 0;
	alignas(64) std::atomic<uint32_t> steal_id = 0;

	Deque(Deque const&) = delete;
	void operator= (Deque const&) = delete;

	using m = std::memory_order;

	struct epoch_seq {
		uint32_t epoch;
		uint32_t seq;
	};

	static epoch_seq unpack(uint64_t sequence){
		uint32_t epoch = sequence >> 32;
		uint32_t seq = sequence;

		return {epoch, seq};
	}

	static uint64_t pack(uint32_t epoch, uint32_t seq){
		uint64_t sequence = epoch;
		sequence = sequence << 32;
		sequence = sequence + seq;
		return sequence;
	}

};

#endif
