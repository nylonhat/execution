#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <functional>
#include <memory>

#include "sender.hpp"

template<typename S>
concept Scheduler = requires(S scheduler, OpHandle op_handle){
    {scheduler.try_schedule(op_handle)} -> std::same_as<bool>;
};

template<Scheduler S>
struct SchedulerHandle {
	S* ptr = nullptr;
	
	SchedulerHandle() = default;

	SchedulerHandle(S& scheduler)
		: ptr{std::addressof(scheduler)}
	{}

	SchedulerHandle(SchedulerHandle& rhs) = default;

	auto try_schedule(OpHandle op_handle){
		return ptr->try_schedule(op_handle);
	}
};


#endif
