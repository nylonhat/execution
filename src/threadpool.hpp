#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "scheduler.hpp"
#include "concepts.hpp"
#include "inlined_receiver.hpp"
#include "pure.hpp"
#include "loopback_child.hpp"

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
					return op.resume();
				}} 
			{}

			OpHandle(OpHandle& rhs) = default;

			void operator()(){
				return resume_ptr(type_ptr);
			}
		};
		
		using Pool = BoundedThreadpool<thread_count, OpHandle>;
		
		template<class NextReceiver>
		struct OpState 
			: ex::InlinedReceiver<OpState<NextReceiver>, NextReceiver>
			, ex::LoopbackChildOp<OpState<NextReceiver>>
		{
			using OpStateOptIn = ex::OpStateOptIn;
			using Receiver = ex::InlinedReceiver<OpState, NextReceiver>;
			using Loopback = ex::LoopbackChildOp<OpState>;

			Pool* pool = nullptr;

			OpState(NextReceiver next_receiver, Pool* pool)
				: Receiver{next_receiver}
				, pool{pool}
			{}

			template<class... Cont>
			void start(Cont&... cont){
				if(pool->try_schedule(*this)){
					return ex::start(cont...); 
				}

				//return ex::set_value<Cont...>(this->get_receiver(), cont...);
				return ex::start(cont..., Loopback::get());
			}

			
			void resume(){
				return ex::set_value<>(this->get_receiver());
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
			

			Pool* pool = nullptr;
			
			template<class NextReceiver>
			auto connect(NextReceiver next_receiver){
				return OpState<NextReceiver>{next_receiver, pool};
			}
		};
		
		Pool pool = {};
		
		using sender_t = Sender;
		
		auto sender(){
			return Sender{&pool};
		}
		
		
	};
	
	template<>
	struct Threadpool<0> {
	
		template<class NextReceiver>
		struct OpState 
			: ex::InlinedReceiver<OpState<NextReceiver>, NextReceiver>
			, ex::LoopbackChildOp<OpState<NextReceiver>>
		{
			using OpStateOptIn = ex::OpStateOptIn;
			using Receiver = ex::InlinedReceiver<OpState, NextReceiver>;
			using Loopback = ex::LoopbackChildOp<OpState>;


			OpState(NextReceiver next_receiver)
				: Receiver{next_receiver}
			{}

			template<class... Cont>
			void start(Cont&... cont){
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
			
			template<class NextReceiver>
			auto connect(NextReceiver next_receiver){
				return OpState<NextReceiver>{next_receiver};
			}
		};
		
		using sender_t = Sender;
		
		auto sender(){
			return Sender{};
		}
	};
	
}




#endif//THREADPOOL_H