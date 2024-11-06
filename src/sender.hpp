#ifndef SENDER_HPP
#define SENDER_HPP

#include <utility>
#include <tuple>
#include <functional>
#include "op_state.hpp"

namespace ex::concepts::connect_cpo {

    struct DummyRecvr{
        auto set_value(auto&..., auto&&...){}
    };
    
    template<class T>
    concept HasMember = requires(T t){
        {t.connect(DummyRecvr{})} -> ex::OpState<>;  
    };

    template<class T>
    concept HasFree = requires(T t){
        {connects(t, DummyRecvr{})} -> ex::OpState<>;  
    };

    template<class T>
    concept HasAll = HasMember<T> && HasFree<T>;
 
    
    struct Function {
        auto operator()(this auto&&, HasMember auto&& sender, auto&& recvr){
            return sender.connect(recvr);
        }


        auto operator()(this auto&&, HasFree auto&& sender, auto&& recvr){
            return connect(sender, recvr);
        }

        
        auto operator()(this auto&&, HasAll auto&& sender, auto&& recvr){
            return sender.connect(recvr);
        }
        
    };
    
}//namespace ex::concepts::sender::connect

namespace ex {
    
    inline constexpr auto connect = concepts::connect_cpo::Function{};

    template<class T>
    concept Sender = requires(T t){
        {ex::connect(t, concepts::connect_cpo::DummyRecvr{})} -> ex::OpState<>;  
    };
    
}//namespace ex

namespace ex {

    static constexpr  auto id = [](auto i){return i;};

    template<typename F, Sender S>
    using apply_values_t = std::invoke_result_t<decltype(&std::apply<F, typename S::value_t>), F, typename S::value_t>;        

    template<Sender... Ss>
    using values_join_t =  std::invoke_result_t<decltype(&std::tuple_cat<typename Ss::value_t...>), typename Ss::value_t...>;        

    template<Sender S>
    using single_value_t = apply_values_t<decltype(id), S>;

    template<Sender S, typename R>
    using connect_t = std::invoke_result_t<decltype(ex::connect), S, R>;

    
    
}//namespace ex

auto operator | (ex::Sender auto value, auto func){
	return func(value);
}

struct OpHandle {
	void* type_ptr = nullptr;
	void (*start_ptr)(void*) = [](void*){};

	OpHandle() = default;

	template<class O>
	OpHandle(O& op)
		: type_ptr{std::addressof(op)}
		, start_ptr{[](void* type_ptr){
			O& op = *static_cast<O*>(type_ptr);
			return op.start();
		}} 
	{}

	OpHandle(OpHandle& rhs) = default;

	void start(){
		return start_ptr(type_ptr);
	}
};

#endif//SENDER_HPP
