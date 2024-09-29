#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <concepts>
#include <functional>
#include <coroutine>
#include <memory>

template<typename O>
concept Op = requires(O op){
	{op.start()} -> std::same_as<void>;
};


template<Op O>
auto type_erased_start(void* type_ptr){
	O& op = *static_cast<O*>(type_ptr);
	return op.start();
}

struct OpHandle {
	void* type_ptr = nullptr;
	void (*start_ptr)(void*) = nullptr;

	OpHandle() = default;
	
	template<Op O>
	OpHandle(O& op)
		: type_ptr{std::addressof(op)}
		, start_ptr{type_erased_start<O>} 
	{}

	OpHandle(OpHandle& rhs) = default;

	void start(){
		return start_ptr(type_ptr);
	}
};




template<typename S>
concept Scheduler = requires(S scheduler, OpHandle op_handle){
	{scheduler.schedule(op_handle)} -> std::same_as<bool>;
};

template<Scheduler S>
auto type_erased_schedule(void* type_ptr, OpHandle op_handle){
	S& scheduler = *static_cast<S*>(type_ptr);
	return scheduler.schedule(op_handle);
}

struct SchedulerHandle {
	void* type_ptr = nullptr;
	bool (*schedule_ptr)(void*, OpHandle) = nullptr;

	SchedulerHandle() = default;

	template<Scheduler S>
	SchedulerHandle(S& scheduler)
		: type_ptr{std::addressof(scheduler)}
		, schedule_ptr{type_erased_schedule<S>} 
	{}

	SchedulerHandle(SchedulerHandle& rhs) = default;

	bool schedule(OpHandle op_handle){
		return schedule_ptr(type_ptr, op_handle);
	}
};

#endif
