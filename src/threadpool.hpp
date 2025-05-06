#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "scheduler.hpp"
#include "concepts.hpp"
#include "inlined_receiver.hpp"
#include "pure.hpp"

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
		{
			using OpStateOptIn = ex::OpStateOptIn;
			using Receiver = ex::InlinedReceiver<OpState, NextReceiver>;

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

				return ex::set_value<Cont...>(this->get_receiver(), cont...);
			}

			void resume(){
				return ex::set_value<>(this->get_receiver());
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
	
		using sender_t = decltype(ex::value());

		auto sender(){
			return ex::value();
		}
	};
	
}




#endif//THREADPOOL_H