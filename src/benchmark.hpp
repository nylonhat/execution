#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "timer.hpp"
#include "concepts.hpp"
#include "inlined_receiver.hpp"
#include "child_variant.hpp"

namespace ex::algorithms::benchmark {
	
	template<IsReceiver SuffixReceiver, IsSender ChildSender>
	struct OpState
		: InlinedReceiver<OpState<SuffixReceiver, ChildSender>, SuffixReceiver>
		, ChildVariant<OpState<SuffixReceiver, ChildSender>, 0, ChildSender>
	{
		
		using OpStateOptIn = ex::OpStateOptIn;
		using Receiver = InlinedReceiver<OpState, SuffixReceiver>;
		using ChildOp =  ChildVariant<OpState, 0, ChildSender>;
		Timer timer = {};
	
		OpState(SuffixReceiver suffix_receiver, ChildSender child_sender)
			: Receiver{suffix_receiver}
			, ChildOp{child_sender}
		{}
	
		auto start(IsOpState auto&... cont){
			timer.start();
			[[gnu::musttail]] return ChildOp::template start<0>(cont...);
		}

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont>
        auto set_value(Cont&... cont, auto...){
			timer.stop();
			[[gnu::musttail]] return ex::set_value<Cont...>(this->get_receiver(), cont..., timer.count());
        }

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont>
        auto set_error(Cont&... cont, auto...){
			timer.stop();
			[[gnu::musttail]] return ex::set_error<Cont...>(this->get_receiver(), cont..., timer.count());
        }

	};

	template<IsSender ChildSender>
	struct Sender {
		using SenderOptIn = ex::SenderOptIn;
		using value_t = std::tuple<std::tuple<std::size_t>>;
		using error_t = std::tuple<std::tuple<std::size_t>>;
		
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
