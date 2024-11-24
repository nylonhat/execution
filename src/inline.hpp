#ifndef INLINE_HPP
#define INLINE_HPP

namespace ex {

    template<class Receiver, class ChildOp>
    concept InlinableReceiver = requires(ChildOp* op){
        {Receiver::make_for_child(op)} -> std::same_as<Receiver>;
    };

    template<class Derived, class Receiver>
        requires InlinableReceiver<Receiver, Derived>
    struct InlinedReceiver {
        InlinedReceiver(Receiver receiver){}

        Receiver get_receiver(){
            return Receiver::make_for_child(static_cast<Derived*>(this));
        }
    };


    template<template<std::size_t> class Receiver, class... Senders, std::size_t... I>
    consteval auto max_op_size_impl(std::index_sequence<I...>){
        return std::max(sizeof(ex::connect_t<Senders, Receiver<I>>)...);
    }
    
    template<template<std::size_t> class Receiver, class... Senders>
    consteval auto max_op_size(){
        return max_op_size_impl<Receiver, Senders...>(std::index_sequence_for<Senders...>{});
    }

    
    template<class ParentOp, std::size_t ChildIndex, std::size_t StageIndex, class... ChildSenders>
    struct ManualChildOp {

        template <std::size_t VariantIndex>
        class Receiver {

            ParentOp* parent_op;

            Receiver(ParentOp* parent_op)
                : parent_op{parent_op}
            {}
             
            template<class ChildOp>
            static Receiver make_for_child(ChildOp* child_op){
                auto* storage = reinterpret_cast<std::byte*>(child_op);
                auto* self = reinterpret_cast<ManualChildOp*>(storage);
                auto* parent_op = static_cast<ParentOp*>(self);
                return Receiver{parent_op};           
            }


            template<class... Cont, class... Arg>
            auto set_value(Cont&... cont, Arg... arg){
                return parent_op->template set_value<ChildIndex, VariantIndex, StageIndex, Cont..., Arg...>(cont..., arg...);
            }

            template<class... Cont, class... Arg>
            auto set_error(Cont&... cont, Arg... arg){
                return parent_op->template set_error<ChildIndex, VariantIndex, StageIndex, Cont..., Arg...>(cont..., arg...);
            }
              
        };
        
        std::array<std::byte, max_op_size<Receiver, ChildSenders...>()> storage;
    
        ManualChildOp() = default;
        ManualChildOp(ChildSenders...[0] sender){
            construct_from<0>(sender);
        }
        
        ~ManualChildOp() = default;
        
        template<std::size_t Index>
        auto& construct_from(ChildSenders...[Index] sender){
            using ChildOp = ex::connect_t<ChildSenders...[Index], Receiver>;
            auto* parent_op = static_cast<ParentOp*>(this);
            return *::new (&storage) ChildOp (ex::connect(sender, Receiver{parent_op}));
        }

        template<std::size_t Index>
        auto& get(){
            using ChildOp = ex::connect_t<ChildSenders...[Index], Receiver>;
            return *std::launder(reinterpret_cast<ChildOp*>(&storage));
        }

        template<std::size_t Index>
        void destruct(){
            using ChildOp = ex::connect_t<ChildSenders...[Index], Receiver>;
            get().~ChildOp();
        }
        

        
        
    };

    

    
}


#endif//INLINE_HPP
