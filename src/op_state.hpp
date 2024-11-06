#ifndef OP_STATE_HPP
#define OP_STATE_HPP

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
        auto operator()(this auto&&){}
        
        auto operator()(this auto&&, HasMember auto& op, auto&...cont){
            return op.start(cont...);
        }

        auto operator()(this auto&&, HasFree auto& op, auto&...cont){
            return start(op, cont...);
        }

    
        auto operator()(this auto&&, HasAll auto& op, auto&...cont){
            return op.start(cont...);
        }

    };
    
}//namespace ex::concept::start::cpo


namespace ex {

    inline constexpr auto start = concepts::start_cpo::Function{};

    template<class T>
    concept OpState = requires(T t){
        {ex::start(t)} -> std::same_as<void>;  
    };
    
}//namespace ex

#endif//OP_STATE_HPP
