#ifndef SIGNATURE_HPP
#define SIGNATURE_HPP

#include "op_state.hpp"
#include "sender.hpp"
#include "receiver.hpp"

#include <tuple>
#include <functional>

namespace ex {
inline namespace signature {
	
	enum class Channel {value, error};  

    template<Channel channel, class S>
    struct channel_sig;

    template<class S>
    struct channel_sig<Channel::value, S>{
        using type = S::value_t;
    };

    template<class S>
    struct channel_sig<Channel::error, S>{
        using type = S::error_t;
    };

    template<Channel channel, class S>
    using channel_sig_t = channel_sig<channel, S>::type;
	
	

	template <class T>
	constexpr static bool is_tuple = false;
	template<class... T>
	constexpr static bool is_tuple<std::tuple<T...>> = true;

	template<class Sig>
	concept is_no_signature = std::same_as<Sig, std::tuple<>>;

	template<class Sig>
	concept is_single_signature = is_tuple<Sig> 
		&& (std::tuple_size_v<Sig> == 1)   
		&& is_tuple<std::tuple_element_t<0, Sig>>;

	template<class Sig>
	concept is_single_return_signature = is_single_signature<Sig> 
		&& (std::tuple_size_v<std::tuple_element_t<0, Sig>> == 1);

	template<class Sig>
	concept is_simple_signature = is_no_signature<Sig> || is_single_signature<Sig>;

	template<class A, class B>
	concept are_both_simple_signatures = is_simple_signature<A> && is_simple_signature<B>;

	template<class A, class B>
	concept has_common_simple_signature = are_both_simple_signatures<A, B>
		&& (std::same_as<A, B> || is_no_signature<A> || is_no_signature<B>);

	template<class A, class B>
		requires has_common_simple_signature<A, B>
	struct common_simple_signature;

	template<class A, class B>
		requires has_common_simple_signature<A, B>
		&& std::same_as<A, B>
	struct common_simple_signature<A, B> {
		using type = A;
	};

	template<class A, class B>
		requires has_common_simple_signature<A, B>
		&& is_no_signature<A>
		&& is_single_signature<B>
	struct common_simple_signature<A, B> {
		using type = B;
	};

	template<class A, class B>
		requires has_common_simple_signature<A, B>
		&& is_no_signature<B>
		&& is_single_signature<A>
	struct common_simple_signature<A, B> {
		using type = A;
	};

	template<class A, class B>
	using common_simple_signature_t = common_simple_signature<A, B>::type;

	template<class F, class Sig>
		requires is_simple_signature<Sig>
	struct signature_of_apply;

	template<class F, class Sig>
		requires is_simple_signature<Sig>
		&& is_single_signature<Sig>
	struct signature_of_apply<F, Sig> {
		using type_t = std::tuple<std::tuple<decltype(std::apply(std::declval<F>(), std::declval<std::tuple_element_t<0, Sig>>()))>>;
		using type = std::conditional_t<std::same_as<type_t, std::tuple<std::tuple<void>>>, std::tuple<std::tuple<>>, type_t>;
	};

	template<class F, class Sig>
		requires is_simple_signature<Sig>
		&& is_no_signature<Sig>
	struct signature_of_apply<F, Sig> {
		using type = std::tuple<>;
	};

	template<class F, class Sig>
	using signature_of_apply_t = signature_of_apply<F, Sig>::type;
	
	struct NullOp {
		using OpStateOptIn = ex::OpStateOptIn;
		
		template<class... Cont>
		auto start(Cont&... cont){
			return;
		}
	};
	
	struct NullSender {
		using SenderOptIn = ex::SenderOptIn;
		
		using value_t = std::tuple<>;
		using error_t = std::tuple<>;
		
		template<IsReceiver NextRx>
		auto connect(NextRx next_receiver){
			return NullOp{};
		}
	};
	
	
	
	
	template<class F, class Sig>
		requires is_simple_signature<Sig>
	struct sender_of_apply_signature;

	template<class F, class Sig>
		requires is_simple_signature<Sig>
		&& is_single_signature<Sig>
	struct sender_of_apply_signature<F, Sig> {
		using type = decltype(std::apply(std::declval<F>(), std::declval<std::tuple_element_t<0, Sig>>()));
	};

	template<class F, class Sig>
		requires is_simple_signature<Sig>
		&& is_no_signature<Sig>
	struct sender_of_apply_signature<F, Sig> {
		using type = NullSender;
	};

	template<class F, class Sig>
	using sender_of_apply_signature_t = sender_of_apply_signature<F, Sig>::type;
	
	
	
	template<IsSender... Senders>
    using sig_values_join_t = std::tuple<decltype(std::tuple_cat(std::declval<std::tuple_element_t<0, typename Senders::value_t>>()...))>;
	
	template<IsSender Sender>
		requires is_single_return_signature<typename Sender::value_t>
    using single_return_t = std::tuple_element_t<0, std::tuple_element_t<0, typename Sender::value_t>>;
	
	template<class Sender>
    concept IsSingleValueSender = is_single_return_signature<typename Sender::value_t>;
	
	
}}//namespace ex::details



#endif//SIGNATURE_HPP
