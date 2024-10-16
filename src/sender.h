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


struct OpHandle;

template<typename O>
concept Op = requires(O op, OpHandle op_handle){
	{op.start(op_handle)} -> std::same_as<void>;
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

	OpHandle(OpHandle& rhs) = default;

	void start(OpHandle op_handle){
		return start_ptr(type_ptr, op_handle);
	}
};

auto set_value = [](auto&& recvr, OpHandle op_handle, auto&&... args){
	recvr.set_value(op_handle, args...);
};

auto start = [](Op auto&& op, OpHandle op_handle) {
	op.start(op_handle);
};


template<class R>
struct NoopOp {
	[[no_unique_address]] R recvr;

	auto start(OpHandle op_handle){
		recvr.set_value(op_handle, 0);
	}
};

struct NoopSender {
	using value_t = std::tuple<int>;

	auto connect(auto recvr){
		return NoopOp{recvr};
	}
};

auto noop = [](){
	return NoopSender{};
};

#endif//SENDER_H
