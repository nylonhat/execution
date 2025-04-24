#ifndef MANUAL_BRANCH_CHILD_H
#define MANUAL_BRANCH_CHILD_H

#include "concepts.hpp"

namespace ex::algorithms::branch_all {
    
    template<class ParentOp, std::size_t ChildIndex, IsSingleValueSender ChildSender>
    struct ManualBranchChildOp {

        template <std::size_t VariantIndex>
        struct Receiver {
            using ReceiverOptIn = ex::ReceiverOptIn;

            ParentOp* parent_op;

            Receiver(ParentOp* parent_op)
                : parent_op{parent_op}
            {}
         
            template<class ChildOp>
            static Receiver make_for_child(ChildOp* child_op){
                auto* storage = reinterpret_cast<std::byte*>(child_op);
                auto* self = reinterpret_cast<ManualBranchChildOp*>(storage);
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


        using Sender = ChildSender;
        using ChildOp = ex::connect_t<ChildSender, Receiver<0>>;
        using Result = ex::single_value_t<ChildSender>;
    
        alignas(Sender) alignas(ChildOp) alignas(Result) std::array<std::byte, std::max({sizeof(Sender), sizeof(ChildOp), sizeof(Result)})> storage;
        
        ManualBranchChildOp() = default;
        ManualBranchChildOp(ChildSender child_sender){
            ::new (&storage) Sender (child_sender);
        }
    
        ~ManualBranchChildOp() = default;
    
        auto& construct_from(ChildSender child_sender){
            auto* parent_op = static_cast<ParentOp*>(this);
            return *::new (&storage) ChildOp (ex::connect(child_sender, Receiver<0>{parent_op}));
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
            return ex::start(get_child_op(), cont...);  
        }
            
    };

}//namespace ex::algorithms::branch

#endif//MANUAL_BRANCH_CHILD_HPP