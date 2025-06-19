#ifndef CHILD_VARIANT_HPP
#define CHILD_VARIANT_HPP

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
    struct ChildVariant {

        template <std::size_t VariantIndex>
        struct Receiver {
            using ReceiverOptIn = ex::ReceiverOptIn;
	
			template<class ChildOp>
			static Receiver make_for_child(ChildOp* child_op){
				auto* storage = reinterpret_cast<std::byte*>(child_op);
				auto* self = reinterpret_cast<ChildVariant*>(storage);
				auto* parent_op = static_cast<ParentOp*>(self);
				return Receiver{parent_op};           
			}

			ParentOp* parent_op;

            Receiver(ParentOp* parent_op)
                : parent_op{parent_op}
            {}
            

            template<class... Cont, class... Arg>
            auto set_value(this auto self, Cont&... cont, Arg... arg){
                [[gnu::musttail]] return self.parent_op->template set_value<ChildIndex, VariantIndex, Cont...>(cont..., arg...);
            }

            template<class... Cont, class... Arg>
            auto set_error(this auto self, Cont&... cont, Arg... arg){
                [[gnu::musttail]] return self.parent_op->template set_error<ChildIndex, VariantIndex, Cont...>(cont..., arg...);
            }
			
			template<class... Cont>
            auto set_stop(this auto self, Cont&... cont){
				[[gnu::musttail]] return self.parent_op->template set_stop<ChildIndex, VariantIndex, Cont...>(cont...);
			}
              
        };

        template<std::size_t Index>
        using VariantOp = ex::connect_t<ChildSenders...[Index], Receiver<Index>>;
        
        alignas(max_op_align<Receiver, ChildSenders...>()) std::array<std::byte, max_op_size<Receiver, ChildSenders...>()> storage;
    
        ChildVariant() = default;
        ChildVariant(ChildSenders...[0] sender){
            construct_from<0>(sender);
        }
        
        ~ChildVariant() = default;

        // gcc can't mangle packs yet
        // template<std::size_t Index>
        // auto& construct_from(ChildSenders...[Index] sender){
        template<std::size_t Index, class S>
        auto& construct_from(S sender){
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


#endif//CHILD_VARIANT_HPP
