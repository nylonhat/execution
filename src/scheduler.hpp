#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <functional>
#include <memory>

struct OpHandle {
	void* type_ptr = nullptr;
	void (*start_ptr)(void*) = [](void*){};

	OpHandle() = default;

	template<class O>
	OpHandle(O& op)
		: type_ptr{std::addressof(op)}
		, start_ptr{[](void* type_ptr){
			O& op = *static_cast<O*>(type_ptr);
			return op.start();
		}} 
	{}

	OpHandle(OpHandle& rhs) = default;

	void start(){
		return start_ptr(type_ptr);
	}
};

template<typename S>
concept IsScheduler = requires(S scheduler, OpHandle op_handle){
    {scheduler.try_schedule(op_handle)} -> std::same_as<bool>;
};

template<IsScheduler S>
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
