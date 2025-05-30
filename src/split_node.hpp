#ifndef SPLIT_NODE_HPP
#define SPLIT_NODE_HPP

#include "concepts.hpp"
#include "inlined_receiver.hpp"
#include "loopback_child.hpp"

namespace ex::algorithms::split {
    
	struct NodeOp {
		using OpStateOptIn = ex::OpStateOptIn;
		
		void* type_ptr = nullptr;
		void(*resume_ptr)(void*) = [](void*){};
		NodeOp* next_node_ptr = nullptr;
		
		template<class O>
		NodeOp(O& op)
			: type_ptr{std::addressof(op)}
			, resume_ptr{[](void* type_ptr){
				O& op = *static_cast<O*>(type_ptr);
				return op.resume();
			}} 
		{}
		
		void start(){
			return resume_ptr(type_ptr);
		}
		
	};
	
    template<class SuffixReceiver, class SplitOp>
    struct SplitNodeOp 
		: InlinedReceiver<SplitNodeOp<SuffixReceiver, SplitOp>, SuffixReceiver>
		, LoopbackChildOp<SplitNodeOp<SuffixReceiver, SplitOp>>
	{
		using Receiver = InlinedReceiver<SplitNodeOp, SuffixReceiver>;
		using Loopback = LoopbackChildOp<SplitNodeOp<SuffixReceiver, SplitOp>>;
		
		SplitOp* split_op_ptr = nullptr;
		NodeOp node_op;
		
		SplitNodeOp(SuffixReceiver suffix_receiver, SplitOp* split_op_ptr)
			: Receiver{suffix_receiver}
			, split_op_ptr{split_op_ptr}
			, node_op{*this}
		{}
		
		template<class... Cont>
		void start(Cont&... cont){
			
			auto& head = split_op_ptr->head;
			void* done_state = split_op_ptr;
			
			void* old_head = head.load();
			
			do {
				if(old_head == done_state){
					[[gnu::musttail]] return ex::set_value<Cont...>(this->get_receiver(), cont..., split_op_ptr->get_source_result());
				}
				
				node_op.next_node_ptr = static_cast<NodeOp*>(old_head);
				
			} while (!head.compare_exchange_weak(old_head, &node_op));
			
			//successfully added to waiting list
			[[gnu::musttail]] return ex::start(cont...);
		}
		
		void resume(){
			if(node_op.next_node_ptr == nullptr){
				//No more waiters
				[[gnu::musttail]] return ex::start(Loopback::get());
			}
			
			//schedule next loop and start self
			[[gnu::musttail]] return ex::start(split_op_ptr->get_scheduler_op(), Loopback::get());
		}
		
		template<class... Cont>
		void loopback(Cont&... cont){
			[[gnu::musttail]] return ex::set_value<Cont...>(this->get_receiver(), cont..., split_op_ptr->get_source_result());
		}
        
    };
	
	
	template<class SplitOp, class SourceSender>
	struct SplitNodeSender {
		using SenderOptIn = ex::SenderOptIn;
		using value_t = SourceSender::value_t;
		using error_t = SourceSender::error_t;

		SplitOp* split_op_ptr = nullptr;
		
		SplitNodeSender(SplitOp* split_op_ptr)
			: split_op_ptr{split_op_ptr}
		{}

		template<IsReceiver SuffixReceiver>
		auto connect(SuffixReceiver suffix_receiver){
			return SplitNodeOp{suffix_receiver, split_op_ptr};
		}
	};

}//namespace ex::algorithms::spit

#endif//SPLIT_NODE_HPP
