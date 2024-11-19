#ifndef OP_STATE_HPP
#define OP_STATE_HPP

#include <concepts>

namespace ex::concepts::start_cpo {

    template<class T>
    concept HasMember = requires(T t){
        {t.start()} -> std::same_as<void>;  
    };

    template<class T>
    concept HasFree = requires(T t){
        {start(t)} -> std::same_as<void>;  
    };

    template<class T>
    concept HasAll = HasMember<T> && HasFree<T>;
 
    
    struct Function {
        constexpr auto operator()(this auto&&){}
        
        constexpr auto operator()(this auto&&, HasMember auto& op, auto&...cont){
            return op.start(cont...);
        }

        constexpr auto operator()(this auto&&, HasFree auto& op, auto&...cont){
            return start(op, cont...);
        }

    
        constexpr auto operator()(this auto&&, HasAll auto& op, auto&...cont){
            return op.start(cont...);
        }

    };
    
}//namespace ex::concept::start::cpo


namespace ex {

    inline constexpr auto start = concepts::start_cpo::Function{};
    
    template<class T>
    concept IsOpState = requires(T t){
        {ex::start(t)} -> std::same_as<void>;  
    };
    
}//namespace ex

#endif//OP_STATE_HPP
