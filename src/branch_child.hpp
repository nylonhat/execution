#ifndef BRANCH_CHILD_H
#define BRANCH_CHILD_H

#include <algorithm>
#include "concepts.hpp"

namespace ex::algorithms::branch_all {
    
    template<class ParentOp, auto Tag, IsSingleValueSender ChildSender>
    struct BranchChildOp {

        struct Receiver {
            using ReceiverOptIn = ex::ReceiverOptIn;
			
			
			template<class Derived>
            struct InlinedReceiver {
				using ReceiverOptIn = ex::ReceiverOptIn;
				
				template<class... Cont, class... Arg>
				auto set_value(Cont&... cont, Arg... arg){
					auto* derived = static_cast<Derived*>(this);
					auto* storage = reinterpret_cast<std::byte*>(derived);
					auto* self = reinterpret_cast<BranchChildOp*>(storage);
					auto* parent_op = static_cast<ParentOp*>(self);
					[[gnu::musttail]] return parent_op->template set_value<Tag, Cont...>(cont..., arg...);
				}
				
				template<class... Cont, class... Arg>
				auto set_error(Cont&... cont, Arg... arg){
					auto* derived = static_cast<Derived*>(this);
					auto* storage = reinterpret_cast<std::byte*>(derived);
					auto* self = reinterpret_cast<BranchChildOp*>(storage);
					auto* parent_op = static_cast<ParentOp*>(self);
					[[gnu::musttail]] return parent_op->template set_error<Tag, Cont...>(cont..., arg...);
				}
				
				template<class... Cont>
				auto set_stop(Cont&... cont){
					auto* derived = static_cast<Derived*>(this);
					auto* storage = reinterpret_cast<std::byte*>(derived);
					auto* self = reinterpret_cast<BranchChildOp*>(storage);
					auto* parent_op = static_cast<ParentOp*>(self);
					[[gnu::musttail]] return parent_op->template set_stop<Tag, Cont...>(cont...);
				}
				
				auto& get_receiver(){
					return *this;
				}

			};
			
			

            ParentOp* parent_op;

            Receiver(ParentOp* parent_op)
                : parent_op{parent_op}
            {}
         

            template<class... Cont, class... Arg>
            auto set_value(Cont&... cont, Arg... arg){
                [[gnu::musttail]] return parent_op->template set_value<Tag, Cont...>(cont..., arg...);
            }

            template<class... Cont, class... Arg>
            auto set_error(Cont&... cont, Arg... arg){
                [[gnu::musttail]] return parent_op->template set_error<Tag, Cont...>(cont..., arg...);
            }
          
        };


        using Sender = ChildSender;
        using ChildOp = ex::connect_t<ChildSender, Receiver>;
        using Result = ex::single_value_t<ChildSender>;
    
        alignas(Sender) alignas(ChildOp) alignas(Result) std::array<std::byte, std::max({sizeof(Sender), sizeof(ChildOp), sizeof(Result)})> storage;
        
        BranchChildOp() = default;
        BranchChildOp(ChildSender child_sender){
            ::new (&storage) Sender (child_sender);
        }
    
        ~BranchChildOp() = default;
    
        auto& construct_from(ChildSender child_sender){
            auto* parent_op = static_cast<ParentOp*>(this);
            return *::new (&storage) ChildOp (ex::connect(child_sender, Receiver{parent_op}));
        }
        
        auto& construct_result(Result result){
            return *::new (&storage) Result (result);
        }

        auto& get_sender(){
            return *std::launder(reinterpret_cast<Sender*>(&storage));
        }

        auto& get_child_op(){
            return *std::launder(reinterpret_cast<ChildOp*>(&storage));
        }

        auto& get_result(){
            return *std::launder(reinterpret_cast<Result*>(&storage));
        }

        template<class... Cont>
        auto start_child_op(Cont&... cont){
            [[gnu::musttail]] return ex::start(get_child_op(), cont...);  
        }
            
    };

}//namespace ex::algorithms::branch

#endif//BRANCH_CHILD_HPP
