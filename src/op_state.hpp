#ifndef OP_STATE_HPP
#define OP_STATE_HPP

#include <concepts>

namespace ex {
    struct OpStateOptIn {};
    
    template<class T>
    concept IsOpState = requires {
        typename T::OpStateOptIn;
    };
    
}//namespace ex

namespace ex::concepts::start_cpo {

    template<class T, class... Cont>
    concept HasMember = requires(T t, Cont&... cont){
        {t.start(cont...)} -> std::same_as<void>;  
    };

    template<class T, class... Cont>
    concept HasFree = requires(T t, Cont&... cont){
        {start(t, cont...)} -> std::same_as<void>;  
    };

    template<class T, class... Cont>
    concept HasAll = HasMember<T, Cont...> && HasFree<T, Cont...>;
 
    
    struct Function {
        constexpr static auto operator()(){}

        template<IsOpState Op, IsOpState... Cont>
        requires HasMember<Op, Cont...>
        // [[clang::always_inline]]
        constexpr static auto operator()(Op& op, Cont&...cont){
            return op.start(cont...);
        }

        template<IsOpState Op, IsOpState... Cont>
        requires HasFree<Op, Cont...>
        // [[clang::always_inline]]
        constexpr static auto operator()(Op& op, Cont&...cont){
            return start(op, cont...);
        }

        template<IsOpState Op, IsOpState... Cont>
        requires HasAll<Op, Cont...>
        // [[clang::always_inline]]
        constexpr static auto operator()(Op& op, Cont&...cont){
            return op.start(cont...);
        }

    };
    
}//namespace ex::concept::start::cpo


namespace ex {

    inline constexpr auto start = concepts::start_cpo::Function{};    
    
}//namespace ex

#endif//OP_STATE_HPP
