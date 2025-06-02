#ifndef SPLIT_NODE_HPP
#define SPLIT_NODE_HPP

#include "concepts.hpp"
#include "inlined_receiver.hpp"
#include "loopback_child.hpp"

namespace ex::algorithms::split {
    
	struct NodeOp {
		using OpStateOptIn = ex::OpStateOptIn;
		
		void(*resume_ptr)(NodeOp*) = [](NodeOp*){};
		NodeOp* next_node_ptr = nullptr;
		
		template<class O>
		NodeOp(O& op)
			: resume_ptr{[](NodeOp* type_ptr){
				O& op = *static_cast<O*>(type_ptr);
				[[gnu::musttail]] return op.resume();
			}} 
		{}
		
		auto& get(){
			return *this;
		}
		
		void start(){
			[[gnu::musttail]] return resume_ptr(this);
		}
		
	};
	
    template<class SuffixReceiver, class SplitOp>
    struct SplitNodeOp 
		: InlinedReceiver<SplitNodeOp<SuffixReceiver, SplitOp>, SuffixReceiver>
		, LoopbackChildOp<SplitNodeOp<SuffixReceiver, SplitOp>>
		, NodeOp
	{
		using OpStateOptIn = ex::OpStateOptIn;
		using Receiver = InlinedReceiver<SplitNodeOp, SuffixReceiver>;
		using Loopback = LoopbackChildOp<SplitNodeOp<SuffixReceiver, SplitOp>>;
		
		SplitOp* split_op_ptr = nullptr;
		
		SplitNodeOp(SuffixReceiver suffix_receiver, SplitOp* split_op_ptr)
			: Receiver{suffix_receiver}
			, NodeOp{*this}
			, split_op_ptr{split_op_ptr}
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
				
				this->next_node_ptr = static_cast<NodeOp*>(old_head);
				
			} while (!head.compare_exchange_weak(old_head, std::addressof(NodeOp::get())));
			
			//successfully added to waiting list
			[[gnu::musttail]] return ex::start(cont...);
		}
		
		void resume(){
			if(this->next_node_ptr == nullptr){
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
