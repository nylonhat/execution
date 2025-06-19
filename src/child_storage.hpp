#ifndef CHILD_STORAGE_HPP
#define CHILD_STORAGE_HPP

#include <algorithm>
#include "concepts.hpp"
#include "signature.hpp"

namespace ex {
inline namespace details {
    
    template<class ParentOp, auto Tag, IsSingleValueSender ChildSender>
    struct ChildStorage {

        struct Receiver {
            using ReceiverOptIn = ex::ReceiverOptIn;
			
			template<class ChildOp>
            static Receiver make_for_child(ChildOp* child_op){
                auto* storage = reinterpret_cast<std::byte*>(child_op);
                auto* self = reinterpret_cast<ChildStorage*>(storage);
                auto* parent_op = static_cast<ParentOp*>(self);
                return Receiver{parent_op};           
            }

            ParentOp* parent_op;

            Receiver(ParentOp* parent_op)
                : parent_op{parent_op}
            {}
         

            template<class... Cont, class... Arg>
            auto set_value(this auto self, Cont&... cont, Arg... arg){
                [[gnu::musttail]] return self.parent_op->template set_value<Tag, Cont...>(cont..., arg...);
            }

            template<class... Cont, class... Arg>
            auto set_error(this auto self, Cont&... cont, Arg... arg){
                [[gnu::musttail]] return self.parent_op->template set_error<Tag, Cont...>(cont..., arg...);
            }
          
        };


        using Sender = ChildSender;
        using ChildOp = ex::connect_t<ChildSender, Receiver>;
        using Result = single_return_t<ChildSender>;
    
        alignas(Sender) alignas(ChildOp) alignas(Result) std::array<std::byte, std::max({sizeof(Sender), sizeof(ChildOp), sizeof(Result)})> storage;
        
        ChildStorage() = default;
        ChildStorage(ChildSender child_sender){
            ::new (&storage) Sender (child_sender);
        }
    
        ~ChildStorage() = default;
    
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

}}//namespace ex::details

#endif//CHILD_STORAGE_HPP
