
#ifndef RECVR_HPP
#define RECVR_HPP


namespace ex::concepts::set_value_cpo {

    template<class T>
    concept HasMember = requires(T t){
        //{t.set_value()} -> std::same_as<void>;  
        t;
    };

    template<class T>
    concept HasFree = requires(T t){
        //{set_value(t)} -> std::same_as<void>;  
        t;
    };

    template<class T>
    concept HasAll = HasMember<T> && HasFree<T>;
 
    
    struct Function {
        auto operator()(this const Function&, HasMember auto& recvr, auto&...cont, auto&&... args) {
            return recvr.template set_value<decltype(cont)...>(cont..., std::forward<decltype(args)>(args)...);
        }

        //auto operator()(this auto&&, HasFree auto& recvr, auto&...cont, auto&&...args){
        //    return set_value<decltype(recvr), decltype(cont)...>(recvr, cont..., args...);
        //}

        auto operator()(this const Function&, HasAll auto& recvr, auto&...cont, auto&&... args){
            return recvr.template set_value<decltype(cont)...>(cont..., args...);
        }
    
    };
    
}//namespace ex::concept::start::cpo


namespace ex {

    inline constexpr auto set_value = concepts::set_value_cpo::Function{};

    template<class T>
    concept Recvr = requires(T t){
        //{ex::set_value(t)} -> std::same_as<void>;  
        t;
    };
    
}//namespace ex


#endif// RECVR_HPP
