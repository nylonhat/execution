#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "timer.h"
#include "recvr.hpp"
#include "sender.hpp"
#include "op_state.hpp"

namespace ex::algorithms::benchmark {

	template<class BaseOp>
	struct Recvr {

		template<class... Cont>
		void set_value(Cont&... cont, auto count){
			auto* byte_p = reinterpret_cast<std::byte*>(this) - offsetof(BaseOp, child_op);
			auto& base_op = *reinterpret_cast<BaseOp*>(byte_p);

			base_op.timer.stop();

			return ex::set_value.operator()
			    <typename BaseOp::EndRecvrT, Cont...>(
				base_op.end_recvr, 
				cont..., 
				base_op.timer.count()/count
			);
		}
	};

	template<ex::Recvr EndRecvr, ex::Sender ChildSender>
	struct OpState {
		using Self = OpState<EndRecvr, ChildSender>;
		using BridgeRecvr = Recvr<Self>;
		using ChildOp = connect_t<ChildSender, BridgeRecvr>;
		using EndRecvrT = EndRecvr;
	
		[[no_unique_address]] EndRecvr end_recvr;

		union{
			ChildOp child_op;
		};

		Timer timer = {};
	
		OpState(ChildSender child_sender, EndRecvr end_recvr)
			: end_recvr{end_recvr}
			, child_op{ex::connect(child_sender, BridgeRecvr{})}
		{}
	
		auto start(auto&... cont){
			timer.start();
			return ex::start(child_op, cont...);
		}

	};

	template<Sender S>
	struct Sender {
		using value_t = std::tuple<std::size_t>;
		S sender;

		auto connect(auto end_recvr){
			return OpState{sender, end_recvr};
		}
	};

}//namespace ex::algorithms::benchmark

namespace ex {

	inline constexpr auto benchmark = []<Sender S>(S sender){
		//explicit template to prevent copy constructor ambiguity
		return algorithms::benchmark::Sender<S>{sender};
	};

}//namespace ex

#endif//REPEAT_H
