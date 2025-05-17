#ifndef FOLD_CHILD_HPP
#define FOLD_CHILD_HPP

#include "concepts.hpp"

namespace ex::algorithms::fold {
    
    template<std::size_t Size, class ParentOp, auto Tag, IsSingleValueSender ChildSender>
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
                return parent_op->template set_value<Tag, Cont...>(ticket, cont..., arg...);
            }

            template<class... Cont, class... Arg>
            auto set_error(Cont&... cont, Arg... arg){
                return parent_op->template set_error<Tag, Cont...>(ticket, cont..., arg...);
            }
          
        };

        using ChildOp = ex::connect_t<ChildSender, Receiver>;
        using Result = ex::single_value_t<ChildSender>;
		
		union alignas(64) Cell {
			bool init = false;
			ChildOp child_op;
			Result result;
		};
		
		std::array<Cell, Size> storage = {};
    
        auto& construct_from_sender_at(ChildSender child_sender, std::size_t ticket){
            auto* parent_op = static_cast<ParentOp*>(this);
            return *::new (&storage.at(ticket).child_op) ChildOp (ex::connect(child_sender, Receiver{parent_op, ticket}));
        }
        
        auto& construct_result_at(Result result, std::size_t ticket){
            return *::new (&storage.at(ticket).result) Result (result);
        }

        auto& get_result_at(std::size_t ticket){
            return storage.at(ticket).result;
        }
            
    };

}//namespace ex::algorithms::fold

#endif//FOLD_CHILD_HPP