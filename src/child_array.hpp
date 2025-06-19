#ifndef CHILD_ARRAY_HPP
#define CHILD_ARRAY_HPP

#include "concepts.hpp"
#include "signature.hpp"


namespace ex {
inline namespace details {
    
    template<std::size_t Size, class ParentOp, auto Tag, IsSingleValueSender ChildSender>
    struct ChildArray {

        struct Receiver {
            using ReceiverOptIn = ex::ReceiverOptIn;

            ParentOp* parent_op;
			std::size_t index;

            Receiver(ParentOp* parent_op, std::size_t index)
                : parent_op{parent_op}
				, index{index}
            {}
         

            template<class... Cont, class... Arg>
            auto set_value(this auto self, Cont&... cont, Arg... arg){
                [[gnu::musttail]] return self.parent_op->template set_value<Tag, Cont...>(self.index, cont..., arg...);
            }

            template<class... Cont, class... Arg>
            auto set_error(this auto self, Cont&... cont, Arg... arg){
                [[gnu::musttail]] return self.parent_op->template set_error<Tag, Cont...>(self.index, cont..., arg...);
            }
          
        };

        using ChildOp = ex::connect_t<ChildSender, Receiver>;
        using Result = single_return_t<ChildSender>;
		
		union alignas(64) Cell {
			std::monostate monostate = {};
			ChildOp child_op;
			Result result;
		};
		
		std::array<Cell, Size> storage = {};
    
        auto& construct_from_sender_at(ChildSender child_sender, std::size_t index){
            auto* parent_op = static_cast<ParentOp*>(this);
            return *::new (&storage.at(index).child_op) ChildOp (ex::connect(child_sender, Receiver{parent_op, index}));
        }
        
        auto& construct_result_at(Result result, std::size_t index){
            return *::new (&storage.at(index).result) Result (result);
        }

        auto& get_result_at(std::size_t index){
            return storage.at(index).result;
        }
            
    };

}}//namespace ex::details

#endif//CHILD_ARRAY_HPP