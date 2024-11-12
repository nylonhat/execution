
#ifndef RECEIVER_HPP
#define RECEIVER_HPP


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
        inline constexpr auto operator()(this const Function&, HasMember auto& recvr, auto&...cont, auto&&... args) {
            return recvr.template set_value<decltype(cont)...>(cont..., std::forward<decltype(args)>(args)...);
        }

        //auto operator()(this auto&&, HasFree auto& recvr, auto&...cont, auto&&...args){
        //    return set_value<decltype(recvr), decltype(cont)...>(recvr, cont..., args...);
        //}

        auto operator()(this const Function&, HasAll auto& recvr, auto&...cont, auto&&... args){
            return recvr.template set_value<decltype(cont)...>(cont..., args...);
        }
    
    };
    
}//namespace ex::concept::set_value_cpo


namespace ex::concepts::set_error_cpo {

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
        inline constexpr auto operator()(this const Function&, HasMember auto& recvr, auto&...cont, auto&&... args) {
            return recvr.template set_error<decltype(cont)...>(cont..., std::forward<decltype(args)>(args)...);
        }

        //auto operator()(this auto&&, HasFree auto& recvr, auto&...cont, auto&&...args){
        //    return set_value<decltype(recvr), decltype(cont)...>(recvr, cont..., args...);
        //}

        auto operator()(this const Function&, HasAll auto& recvr, auto&...cont, auto&&... args){
            return recvr.template set_error<decltype(cont)...>(cont..., args...);
        }
    
    };
    
}//namespace ex::concept::set_error_cpo


namespace ex {

    inline constexpr auto set_value = concepts::set_value_cpo::Function{};
    inline constexpr auto set_error = concepts::set_error_cpo::Function{};

    template<class T>
    concept IsReceiver = requires(T t){
        //{ex::set_value(t)} -> std::same_as<void>;  
        t;
    };
    
}//namespace ex


#endif// RECEIVER_HPP
