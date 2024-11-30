#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "timer.hpp"
#include "concepts.hpp"
#include "inline.hpp"

namespace ex::algorithms::benchmark {
	
	template<IsReceiver SuffixReceiver, IsSender ChildSender>
	struct OpState
		: InlinedReceiver<OpState<SuffixReceiver, ChildSender>, SuffixReceiver>
		, ManualChildOp<OpState<SuffixReceiver, ChildSender>, 0, ChildSender>
	{

		using Receiver = InlinedReceiver<OpState, SuffixReceiver>;
		using ChildOp =  ManualChildOp<OpState, 0, ChildSender>;
		Timer timer = {};
	
		OpState(SuffixReceiver suffix_receiver, ChildSender child_sender)
			: Receiver{suffix_receiver}
			, ChildOp{child_sender}
		{}
	
		auto start(IsOpState auto&... cont){
			timer.start();
			return ChildOp::template start<0>(cont...);
		}

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont>
        auto set_value(Cont&... cont, auto count){
			timer.stop();
			return ex::set_value.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., timer.count()/count);
        }

	};

	template<IsSender ChildSender>
	struct Sender {
		using value_t = std::tuple<std::size_t>;
		ChildSender child_sender;

		auto connect(IsReceiver auto suffix_receiver){
			return OpState{suffix_receiver, child_sender};
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
