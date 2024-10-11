#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <functional>
#include <memory>

#include "sender.h"

template<typename S>
concept Scheduler = requires(S scheduler, OpHandle op_handle){
	{scheduler.schedule(op_handle)} -> std::same_as<OpHandle>;
};

template<Scheduler S>
auto type_erased_schedule(void* type_ptr, OpHandle op_handle){
	S& scheduler = *static_cast<S*>(type_ptr);
	return scheduler.schedule(op_handle);
}

struct SchedulerHandle {
	void* type_ptr = nullptr;
	OpHandle (*schedule_ptr)(void*, OpHandle) = nullptr;

	SchedulerHandle() = default;

	template<Scheduler S>
	SchedulerHandle(S& scheduler)
		: type_ptr{std::addressof(scheduler)}
		, schedule_ptr{type_erased_schedule<S>} 
	{}

	SchedulerHandle(SchedulerHandle& rhs) = default;

	OpHandle schedule(OpHandle op_handle){
		return schedule_ptr(type_ptr, op_handle);
	}
};

#endif
