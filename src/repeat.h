#ifndef REPEAT_H
#define REPEAT_H

#include "timer.h"
#include "sender.hpp"
#include "op_state.hpp"
#include "recvr.hpp"

namespace ex::algorithms::repeat {

	template<typename BaseOp>
	struct Recvr {
		void set_value(auto&... cont, auto... args){
			auto* byte_p = reinterpret_cast<std::byte*>(this) - offsetof(BaseOp, child_op);
			auto& base_op = *reinterpret_cast<BaseOp*>(byte_p);

			return ex::start(base_op, cont...);
		}
	};

	template<typename EndRecvr, ex::Sender ChildSender>
	struct OpState {
		using Self = OpState<EndRecvr, ChildSender>;
		using BridgeRecvr = Recvr<Self>;
		using ChildOp = connect_t<ChildSender, BridgeRecvr>;
	
		[[no_unique_address]] EndRecvr end_recvr;

		union{
			ChildOp child_op;
		};
	
		[[no_unique_address]] ChildSender child_sender;

		std::size_t count = 0;
		const size_t max = 100'000'000;

		OpState(ChildSender child_sender, EndRecvr end_recvr, size_t iterations)
			: end_recvr{end_recvr}
			, child_sender{child_sender}
			, max{iterations}
		{}
	
		template<class... Cont>
		auto start(Cont&... cont){
			if(count < max){
				count++;
				new (&child_op) ChildOp (ex::connect(child_sender, BridgeRecvr{}));
				return ex::start(child_op, cont...);
			}

			return ex::set_value.operator()<EndRecvr, Cont...>(end_recvr, cont..., count);
		}

	};

	template<ex::Sender ChildSender>
	struct Sender {
		using value_t = std::tuple<std::size_t>;
		ChildSender child_sender;
		size_t iterations;

		auto connect(ex::Recvr auto end_recvr){
			return OpState{child_sender, end_recvr, iterations};
		}
	};


}//namespace ex::algorithms::repeat


namespace ex {

	inline constexpr auto repeat = []<Sender S>(S sender, size_t iterations){
		//explicit template to prevent copy constructor ambiguity
		return algorithms::repeat::Sender<S>{sender, iterations};
	};

	inline constexpr auto repeat_n = [](size_t iterations){
		return [=]<Sender S>(S sender){
			return algorithms::repeat::Sender<S>{sender, iterations};
		};
	};

}//namespace ex


#endif//REPEAT_H
