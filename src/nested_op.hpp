#ifndef NESTED_OP_H
#define NESTED_OP_H

#include "concepts.hpp"
#include "pure.hpp"

namespace ex::algorithms::branch_all {
    
    template<class ParentOp, std::size_t ChildIndex, class ChildSender>
    struct ManualChildResultOp {

        template <std::size_t VariantIndex>
        struct Receiver {

            ParentOp* parent_op;

            Receiver(ParentOp* parent_op)
                : parent_op{parent_op}
            {}
         
            template<class ChildOp>
            static Receiver make_for_child(ChildOp* child_op){
                auto* storage = reinterpret_cast<std::byte*>(child_op);
                auto* self = reinterpret_cast<ManualChildResultOp*>(storage);
                auto* parent_op = static_cast<ParentOp*>(self);
                return Receiver{parent_op};           
            }


            template<class... Cont, class... Arg>
            auto set_value(Cont&... cont, Arg... arg){
                return parent_op->template set_value<ChildIndex, VariantIndex, Cont...>(cont..., arg...);
            }

            template<class... Cont, class... Arg>
            auto set_error(Cont&... cont, Arg... arg){
                return parent_op->template set_error<ChildIndex, VariantIndex, Cont...>(cont..., arg...);
            }
          
        };


        using SenderOp = ex::connect_t<decltype(ex::value(std::declval<ChildSender>())), Receiver<0>>;
        using ChildOp = ex::connect_t<ChildSender, Receiver<1>>;
        using Result = ex::single_value_t<ChildSender>;
    
        alignas(SenderOp) alignas(ChildOp) alignas(Result) std::array<std::byte, std::max({sizeof(SenderOp), sizeof(ChildOp), sizeof(Result)})> storage;
        
        ManualChildResultOp() = default;
        ManualChildResultOp(ChildSender child_sender){
            auto* parent_op = static_cast<ParentOp*>(this);
            ::new (&storage) SenderOp (ex::connect(ex::value(child_sender), Receiver<0>{parent_op}));
        }
    
        ~ManualChildResultOp() = default;
    
        auto& construct_from(ChildSender child_sender){
            auto* parent_op = static_cast<ParentOp*>(this);
            return *::new (&storage) ChildOp (ex::connect(child_sender, Receiver<1>{parent_op}));
        }

        
        auto& construct_result(Result result){
            return *::new (&storage) Result (result);
        }

        auto& get_sender_op(){
            return *std::launder(reinterpret_cast<SenderOp*>(&storage));
        }

        auto& get_child_op(){
            return *std::launder(reinterpret_cast<ChildOp*>(&storage));
        }

        auto& get_result(){
            return *std::launder(reinterpret_cast<Result*>(&storage));
        }

        template<class... Cont>
        auto start_sender_op(Cont&... cont){
            return ex::start(get_sender_op(), cont...);  
        }

        template<class... Cont>
        auto start_child_op(Cont&... cont){
            return ex::start(get_child_op(), cont...);  
        }
            
    };

}//namespace ex::algorithms::branch

#endif//NESTED_OP_H
