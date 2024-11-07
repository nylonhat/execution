#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "timer.hpp"
#include "concepts.hpp"

namespace ex::algorithms::benchmark {

	template<class BaseOp>
	struct Receiver {

		template<class... Cont>
		void set_value(Cont&... cont, auto count){
			auto* byte_p = reinterpret_cast<std::byte*>(this) - offsetof(BaseOp, child_op);
			auto& base_op = *reinterpret_cast<BaseOp*>(byte_p);

			base_op.timer.stop();

			return ex::set_value.operator()
			    <typename BaseOp::NextReceiver, Cont...>(
				base_op.next_receiver, 
				cont..., 
				base_op.timer.count()/count
			);
		}
	};

	template<IsReceiver SuffixReceiver, IsSender ChildSender>
	struct OpState {
		using Self = OpState<SuffixReceiver, ChildSender>;
		using InfixReceiver = Receiver<Self>;
		using ChildOp = connect_t<ChildSender, InfixReceiver>;
		using NextReceiver = SuffixReceiver;
	
		NextReceiver next_receiver;

		union{
			ChildOp child_op;
		};

		Timer timer = {};
	
		OpState(ChildSender child_sender, SuffixReceiver suffix_receiver)
			: next_receiver{suffix_receiver}
			, child_op{ex::connect(child_sender, InfixReceiver{})}
		{}
	
		auto start(IsOpState auto&... cont){
			timer.start();
			return ex::start(child_op, cont...);
		}

	};

	template<IsSender ChildSender>
	struct Sender {
		using value_t = std::tuple<std::size_t>;
		ChildSender child_sender;

		auto connect(IsReceiver auto suffix_receiver){
			return OpState{child_sender, suffix_receiver};
		}
	};

}//namespace ex::algorithms::benchmark

namespace ex {

	inline constexpr auto benchmark = []<IsSender ChildSender>(ChildSender child_sender){
		//explicit template to prevent copy constructor ambiguity
		return algorithms::benchmark::Sender<ChildSender>{child_sender};
	};

}//namespace ex

#endif//REPEAT_H
