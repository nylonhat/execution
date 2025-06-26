#ifndef CONCEPTS_HPP
#define CONCEPTS_HPP

#include "op_state.hpp"
#include "sender.hpp"
#include "receiver.hpp"

namespace ex {

    constexpr inline  auto identity = [](auto i){return i;};
    
    constexpr inline auto add = [](auto... v) {
    	return (... + v);
    };

    template<IsSender Sender, IsReceiver Receiver>
    using connect_t = std::invoke_result_t<decltype(ex::connect), Sender, Receiver>;

    template<size_t A, size_t B>
	concept same_index = A == B;

	template<size_t A, size_t B>
	concept not_same_index = A != B;

	template<class... Pack>
	concept not_empty_pack = sizeof...(Pack) != 0;

	template<class T, class... Pack>
	concept first_same_as = std::same_as<Pack...[0], T>;

	template<class T, auto I>
	using repeat_t = T;
	
	
	constexpr auto operator | (ex::IsSender auto value, auto func){
		return func(value);
	}
    
}//namespace ex



#endif//CONCEPTS_HPP
