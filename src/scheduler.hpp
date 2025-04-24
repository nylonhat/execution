#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <functional>
#include <memory>


template<typename S>
concept IsScheduler = requires(S scheduler){
    //{scheduler.try_schedule(op_handle)} -> std::same_as<bool>;
	scheduler;
};

template<IsScheduler S>
struct SchedulerHandle {
	S* ptr = nullptr;
	
	SchedulerHandle() = default;
	
	using sender_t = S::sender_t;

	SchedulerHandle(S& scheduler)
		: ptr{std::addressof(scheduler)}
	{}

	SchedulerHandle(const SchedulerHandle& rhs) = default;

	auto sender(){
		return ptr->sender();
	}
};


#endif
