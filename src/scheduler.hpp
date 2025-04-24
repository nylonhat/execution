#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <functional>
#include <memory>

#include "sender.hpp"

namespace ex {

template<typename S>
concept IsScheduler = requires(S scheduler){
    {scheduler.sender()} -> IsSender<>;
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

}

#endif
