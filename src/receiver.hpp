#ifndef RECEIVER_HPP
#define RECEIVER_HPP

namespace ex {
    struct ReceiverOptIn {};
    
    template<class Rcvr>
    concept IsReceiver = requires(){
        typename std::remove_cvref_t<Rcvr>::ReceiverOptIn;
    };
    
}//namespace ex

namespace ex::concepts::set_value_cpo {
 
    template<class... Cont>
    struct FunctionObject {
        template<IsReceiver Recvr, class... Args>
			requires requires(Recvr recvr, Cont&... cont, Args... args){{recvr.template set_value<Cont...>(cont..., args...)} -> std::same_as<void>;}
			&& (!requires(Recvr recvr, Cont&... cont, Args... args){{set_value<Cont...>(recvr, cont..., args...)} -> std::same_as<void>;})
        constexpr static auto operator()(Recvr recvr, Cont&...cont, Args... args) {
            [[gnu::musttail]] return recvr.template set_value<Cont...>(cont..., args...);
        }

        template<IsReceiver Recvr, class... Args>
			requires requires(Recvr recvr, Cont&... cont, Args... args){{set_value<Cont...>(recvr, cont..., args...)} -> std::same_as<void>;}
			&& (!requires(Recvr recvr, Cont&... cont, Args... args){{recvr.template set_value<Cont...>(cont..., args...)} -> std::same_as<void>;})
        constexpr static auto operator()(Recvr recvr, Cont&...cont, Args... args) {
            [[gnu::musttail]] return set_value<Recvr, Cont...>(recvr, cont..., args...);
        }

        template<IsReceiver Recvr, class... Args>
			requires requires(Recvr recvr, Cont&... cont, Args... args){{recvr.template set_value<Cont...>(cont..., args...)} -> std::same_as<void>;}
			&& requires(Recvr recvr, Cont&... cont, Args... args){{set_value<Cont...>(recvr, cont..., args...)} -> std::same_as<void>;}
        constexpr static auto operator()(Recvr recvr, Cont&...cont, Args... args) {
			[[gnu::musttail]] return recvr.template set_value<Cont...>(cont..., args...);
        }
		
            
    };
    
}//namespace ex::concept::set_value_cpo


namespace ex::concepts::set_error_cpo {
 
    template<class... Cont>
    struct FunctionObject {

        template<IsReceiver Recvr, class... Args>
		requires requires(Recvr recvr, Cont&... cont, Args... args){{recvr.template set_error<Cont...>(cont..., args...)} -> std::same_as<void>;}
			&& (!requires(Recvr recvr, Cont&... cont, Args... args){{set_error<Cont...>(recvr, cont..., args...)} -> std::same_as<void>;})
        constexpr static auto operator()(Recvr recvr, Cont&...cont, Args... args) {
            [[gnu::musttail]] return recvr.template set_error<Cont...>(cont..., args...);
        }

        template<IsReceiver Recvr, class... Args>
		requires requires(Recvr recvr, Cont&... cont, Args... args){{set_error<Cont...>(recvr, cont..., args...)} -> std::same_as<void>;}
			&& (!requires(Recvr recvr, Cont&... cont, Args... args){{recvr.template set_error<Cont...>(cont..., args...)} -> std::same_as<void>;})
        constexpr static auto operator()(Recvr recvr, Cont&...cont, Args... args) {
            [[gnu::musttail]] return set_error<Recvr, Cont...>(recvr, cont..., args...);
        }

        template<IsReceiver Recvr, class... Args>
		requires requires(Recvr recvr, Cont&... cont, Args... args){{recvr.template set_error<Cont...>(cont..., args...)} -> std::same_as<void>;}
			&& requires(Recvr recvr, Cont&... cont, Args... args){{set_error<Cont...>(recvr, cont..., args...)} -> std::same_as<void>;}
        constexpr static auto operator()(Recvr recvr, Cont&...cont, Args... args) {
            [[gnu::musttail]] return recvr.template set_error<Cont...>(cont..., args...);
        }
            
    };
    
}//namespace ex::concept::set_error_cpo

namespace ex::concepts::set_stop_cpo {

    template<class Recvr, class... Cont>
    concept HasMember = requires(Recvr recvr, Cont&... cont){
        {recvr.template set_stop<Cont...>(cont...)} -> std::same_as<void>;  
    };

    template<class Recvr, class... Cont>
    concept HasFree = requires(Recvr recvr, Cont&... cont){
        {set_stop<Cont...>(recvr, cont...)} -> std::same_as<void>;  
    };

    template<class Recvr, class... Cont>
    concept HasAll = HasMember<Recvr, Cont...> && HasFree<Recvr, Cont...>;
 
    template<class... Cont>
    struct FunctionObject {

        template<IsReceiver Recvr>
			requires HasMember<Recvr, Cont...>
        constexpr static auto operator()(Recvr recvr, Cont&...cont) {
            return recvr.template set_stop<Cont...>(cont...);
        }

        template<IsReceiver Recvr>
			requires HasFree<Recvr, Cont...>
        constexpr static auto operator()(Recvr recvr, Cont&...cont) {
            return set_stop<Recvr, Cont...>(recvr, cont...);
        }
 
        template<IsReceiver Recvr>
			requires HasAll<Recvr, Cont...>
        constexpr static auto operator()(Recvr recvr, Cont&...cont) {
            return recvr.template set_stop<Cont...>(cont...);
        }
            
    };
    
}//namespace ex::concept::set_stop_cpo

namespace ex {
	template<class... Cont>
    inline constexpr auto set_value = concepts::set_value_cpo::FunctionObject<Cont...>{};
    
	template<class... Cont>
	inline constexpr auto set_error = concepts::set_error_cpo::FunctionObject<Cont...>{};
	
	template<class... Cont>
	inline constexpr auto set_stop = concepts::set_stop_cpo::FunctionObject<Cont...>{};
    
}//namespace ex


#endif// RECEIVER_HPP
