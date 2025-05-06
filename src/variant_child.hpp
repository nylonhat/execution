#ifndef VARIANT_CHILD_HPP
#define VARIANT_CHILD_HPP

#include "concepts.hpp"
#include <algorithm>

namespace ex {

    template<template<std::size_t> class Receiver, class... Senders, std::size_t... I>
    consteval auto max_op_size_impl(std::index_sequence<I...>){
        return std::max({sizeof(ex::connect_t<Senders, Receiver<I>>)...});
    }
    
    template<template<std::size_t> class Receiver, class... Senders>
    consteval auto max_op_size(){
        return max_op_size_impl<Receiver, Senders...>(std::index_sequence_for<Senders...>{});
    }

    template<template<std::size_t> class Receiver, class... Senders, std::size_t... I>
    consteval auto max_op_align_impl(std::index_sequence<I...>){
        return std::max({alignof(ex::connect_t<Senders, Receiver<I>>)...});
    }
    
    template<template<std::size_t> class Receiver, class... Senders>
    consteval auto max_op_align(){
        return max_op_align_impl<Receiver, Senders...>(std::index_sequence_for<Senders...>{});
    }

    template<class ParentOp, std::size_t ChildIndex, class... ChildSenders>
    struct VariantChildOp {

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
                auto* self = reinterpret_cast<VariantChildOp*>(storage);
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
			
			template<class... Cont>
            auto set_stop(Cont&... cont){
				return parent_op->template set_stop<ChildIndex, VariantIndex, Cont...>(cont...);
			}
              
        };

        template<std::size_t Index>
        using VariantOp = ex::connect_t<ChildSenders...[Index], Receiver<Index>>;
        
        alignas(max_op_align<Receiver, ChildSenders...>()) std::array<std::byte, max_op_size<Receiver, ChildSenders...>()> storage;
    
        VariantChildOp() = default;
        VariantChildOp(ChildSenders...[0] sender){
            construct_from<0>(sender);
        }
        
        ~VariantChildOp() = default;
        
        template<std::size_t Index>
        auto& construct_from(ChildSenders...[Index] sender){
            auto* parent_op = static_cast<ParentOp*>(this);
            return *::new (&storage) VariantOp<Index> (ex::connect(sender, Receiver<Index>{parent_op}));
        }

        template<std::size_t Index>
        auto& get(){
            return *std::launder(reinterpret_cast<VariantOp<Index>*>(&storage));
        }

        template<std::size_t Index>
        void destruct(){
            get<Index>().~VariantOp<Index>();
        }

        template<std::size_t Index, class... Cont>
        auto start(Cont&... cont){
            return ex::start(get<Index>(), cont...);  
        }
        
    };   
    
}


#endif//VARIANT_CHILD_HPP
