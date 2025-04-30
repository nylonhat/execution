#ifndef FOLD_CHILD_HPP
#define FOLD_CHILD_HPP

#include "concepts.hpp"

namespace ex::algorithms::fold {
    
    template<std::size_t Size, class ParentOp, std::size_t ChildIndex, IsSingleValueSender ChildSender>
    struct FoldChildOp {

        struct Receiver {
            using ReceiverOptIn = ex::ReceiverOptIn;

            ParentOp* parent_op;
			std::size_t ticket;

            Receiver(ParentOp* parent_op, std::size_t ticket)
                : parent_op{parent_op}
				, ticket{ticket}
            {}
         

            template<class... Cont, class... Arg>
            auto set_value(Cont&... cont, Arg... arg){
                return parent_op->template set_value<ChildIndex, Cont...>(ticket, cont..., arg...);
            }

            template<class... Cont, class... Arg>
            auto set_error(Cont&... cont, Arg... arg){
                return parent_op->template set_error<ChildIndex, Cont...>(ticket, cont..., arg...);
            }
          
        };


        using ChildOp = ex::connect_t<ChildSender, Receiver>;
        using Result = ex::single_value_t<ChildSender>;
		
		union Cell {
			bool init = false;
			ChildOp child_op;
			Result result;
		};
		
		std::array<Cell, Size> storage = {};
        
        FoldChildOp() = default;
    
        ~FoldChildOp() = default;
    
        auto& construct_from_at(ChildSender child_sender, std::size_t ticket){
            auto* parent_op = static_cast<ParentOp*>(this);
            return *::new (&storage.at(ticket).child_op) ChildOp (ex::connect(child_sender, Receiver{parent_op, ticket}));
        }
        
        auto& construct_result_at(Result result, std::size_t ticket){
            return *::new (&storage.at(ticket).result) Result (result);
        }


        auto& get_child_op_at(std::size_t ticket){
            return storage.at(ticket).child_op;
        }

        auto& get_result_at(std::size_t ticket){
            return storage.at(ticket).result;
        }

        template<class... Cont>
        auto start_child_op_at(std::size_t ticket, Cont&... cont){
            return ex::start(get_child_op_at(ticket), cont...);  
        }
            
    };

}//namespace ex::algorithms::fold

#endif//FOLD_CHILD_HPP