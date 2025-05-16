#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "scheduler.hpp"
#include "concepts.hpp"
#include "inlined_receiver.hpp"
#include "loopback_child.hpp"
#include "inline_scheduler.hpp"
#include "bounded_threadpool.hpp"

namespace ex {
	
	
	template<std::size_t thread_count>
	struct Threadpool {
		
		struct OpHandle {
			void* type_ptr = nullptr;
			void (*resume_ptr)(void*) = [](void*){};

			OpHandle() = default;

			template<class O>
			OpHandle(O& op)
				: type_ptr{std::addressof(op)}
				, resume_ptr{[](void* type_ptr){
					O& op = *static_cast<O*>(type_ptr);
					return op.loopback();
				}} 
			{}

			OpHandle(OpHandle& rhs) = default;

			void operator()(){
				return resume_ptr(type_ptr);
			}
		};
		
		using ExecutionResource = BoundedThreadpool<thread_count, OpHandle>;
		
		ExecutionResource execution_resource = {};
		
		template<class Receiver>
		struct OpState 
			: ex::InlinedReceiver<OpState<Receiver>, Receiver>
			, ex::LoopbackChildOp<OpState<Receiver>>
		{
			using OpStateOptIn = ex::OpStateOptIn;
			using InlinedReceiver = ex::InlinedReceiver<OpState, Receiver>;
			using Loopback = ex::LoopbackChildOp<OpState>;

			ExecutionResource* execution_resource = nullptr;

			OpState(Receiver receiver, ExecutionResource* execution_resource)
				: InlinedReceiver{receiver}
				, execution_resource{execution_resource}
			{}

			template<class... Cont>
			void start(Cont&... cont){
				if(execution_resource->try_schedule(*this)){
					return ex::start(cont...); 
				}

				return ex::start(cont..., Loopback::get());
			}
			
			template<class... Cont>
			void loopback(Cont&... cont){
				return ex::set_value<Cont...>(this->get_receiver(), cont...);
			}
			
		};

		struct Sender {
			using SenderOptIn = ex::SenderOptIn;
			using value_t = std::tuple<>;
			using error_t = std::tuple<>;
			

			ExecutionResource* execution_resource = nullptr;
			
			template<class NextReceiver>
			auto connect(NextReceiver next_receiver){
				return OpState<NextReceiver>{next_receiver, execution_resource};
			}
		};
		
		struct Scheduler {
			using sender_t = Sender;
			
			ExecutionResource* execution_resource = nullptr;
			
			auto sender(){
				return Sender{execution_resource};
			}
		};
		
		auto scheduler(){
			return Scheduler{&execution_resource};
		}
		
	};
	
	template<>
	struct Threadpool<0> {
		
		auto scheduler(){
			return InlineScheduler{};
		}

	};
	
}




#endif//THREADPOOL_H