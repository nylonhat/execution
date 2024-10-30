#ifndef SENDER_H
#define SENDER_H

#include <functional>
#include <concepts>

struct NoopRecvr {        
	auto set_value(auto... args){}
};

auto id = [](auto i){return i;};

template<typename S>
concept Sender = requires(S sender, NoopRecvr recvr){
	sender.connect(recvr);
};

template<typename F, Sender S>
using apply_values_t = std::invoke_result_t<decltype(&std::apply<F, typename S::value_t>), F, typename S::value_t>;        

template<Sender... Ss>
using values_join_t =  std::invoke_result_t<decltype(&std::tuple_cat<typename Ss::value_t...>), typename Ss::value_t...>;        

template<Sender S>
using single_value_t = apply_values_t<decltype(id), S>;


auto connect = [](Sender auto&& sender, auto&& recvr){
	return sender.connect(recvr);
};

template<Sender S, typename R>
using connect_t = std::invoke_result_t<decltype(::connect), S, R>;

auto operator | (Sender auto value, auto func){
	return func(value);
}


struct Noop {
	auto start(auto&& cont){}
};

template<typename O>
concept Op = requires(O op){
	{op.start(Noop{})} -> std::same_as<void>;
};


struct OpHandle {
	void* type_ptr = nullptr;
	void (*start_ptr)(void*, OpHandle) = [](void*, OpHandle){};

	OpHandle() = default;
	
	template<class O>
	OpHandle(O& op)
		: type_ptr{std::addressof(op)}
		, start_ptr{[](void* type_ptr, OpHandle op_handle){
			O& op = *static_cast<O*>(type_ptr);
			return op.start(op_handle);
		}} 
	{}

	OpHandle(Noop noop){}

	OpHandle(OpHandle& rhs) = default;

	void start(OpHandle op_handle){
		return start_ptr(type_ptr, op_handle);
	}
};

auto set_value = [](auto&& recvr, auto&& cont, auto&&... args){
	recvr.set_value(std::forward<decltype(cont)>(cont), args...);
};

auto start = [](Op auto&& op, auto&& cont) {
	op.start(std::forward<decltype(cont)>(cont));
};

#endif//SENDER_H
