#ifndef INLINE_SCHEDULER_H
#define INLINE_SCHEDULER_H

#include "concepts.hpp"
#include "inlined_receiver.hpp"
#include "loopback_child.hpp"

namespace ex {

	struct InlineScheduler {
	
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

			
			void start(){
				[[gnu::musttail]] return ex::set_value<>(this->get_receiver());
			}

			template<class ContFirst, class... Cont>
			void start(ContFirst& cont_first, Cont&... cont){
				[[gnu::musttail]] return ex::start(cont_first, Loopback::get(), cont...);
			}
			
			template<class... Cont>
			void loopback(Cont&... cont){
				[[gnu::musttail]] return ex::set_value<Cont...>(this->get_receiver(), cont...);
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

} //namespace ex

#endif//INLINE_SCHEDULER_H

