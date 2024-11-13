#ifndef NESTED_OP_H
#define NESTED_OP_H

#include "concepts.hpp"

namespace ex::algorithms::branch {

    template<size_t Index, class T>
    constexpr auto& get_result(T& nested_op){
        if constexpr (T::index == Index){
            return nested_op.result;
        }else{
            return get_result<Index>(nested_op.next);
        }
    }

    template<size_t Index, class T>
    consteval auto get_offset(){
        if constexpr (T::index == Index){
            return 0;
        }else{
            return offsetof(T, next) + get_offset<Index, typename T::Next>();
        }
    }

    template<class BaseOp, class ChildOp>
    auto& get_base_op(ChildOp* child_ptr){
        auto total_offset = offsetof(BaseOp, tuple) + get_offset<ChildOp::index, typename BaseOp::Tuple>();
        auto* offset_ptr = reinterpret_cast<std::byte*>(child_ptr) - total_offset;
    	auto& base_op = *reinterpret_cast<BaseOp*>(offset_ptr);
    	return base_op;
    }


    template<size_t Index, class BaseOp>
    struct NestedOp {
        using Sender = std::tuple_element_t<Index, typename BaseOp::SenderTypeList>;
        struct Empty{};
        using Next = std::conditional_t<Index == BaseOp::size-1, Empty, NestedOp<Index+1, BaseOp>>;
        using InfixReceiver =  BaseOp::template IndexRecvr<Index>;
        using Op = connect_t<Sender, InfixReceiver>;
        using Result = single_value_t<Sender>;

        constexpr static size_t index = Index;

        union {
            Sender sender;
            Op op;
            Result result;
        };

        [[no_unique_address]] Next next;

        NestedOp(Sender sender, auto... senders)
            : sender{sender}
            , next{senders...}
        {}
    
        auto start(auto&... cont){
    		auto& base_op = get_base_op<BaseOp>(this);

            new (&op) Op (ex::connect(sender, InfixReceiver{}));

            if constexpr(Index == BaseOp::size-1){
                return ex::start(op, cont...);

            } else {
                if(base_op.scheduler.try_schedule(next)){
                    return ex::start(op, cont...);
                }

                return ex::start(op, next, cont...); 
            }
        }
    
    };

    template<class BOP>
    using OpTuple = NestedOp<0, BOP>;

    template<class F, class Tuple, std::size_t... I>
    constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>){
        return std::invoke(std::forward<F>(f), get_result<I>(std::forward<Tuple>(t))...);    
    }

    template<class F, class BaseOp>
    constexpr decltype(auto) apply(F&& f, OpTuple<BaseOp>& t){
        return apply_impl(std::forward<F>(f), std::forward<decltype(t)>(t), std::make_index_sequence<BaseOp::size>{});
    }

    template<class... Pack>
    concept empty_pack = sizeof...(Pack) == 0;

    template<class... Pack>
    using first_in_pack = std::remove_reference_t<std::tuple_element_t<0, std::tuple<Pack...>>>;

    template<class T, class...Pack>
    concept first_same_as = !empty_pack<Pack...> && std::same_as<first_in_pack<Pack...>, T>;

}//namespace ex::algorithms::branch

#endif//NESTED_OP_H
