#ifndef REPEAT_H
#define REPEAT_H

#include "concepts.hpp"
#include "manual_lifetime.hpp"

namespace ex::algorithms::repeat {

	template<typename BaseOp>
	struct Receiver {
		void set_value(auto&... cont, auto... args){
			auto* byte_p = reinterpret_cast<std::byte*>(this) - offsetof(BaseOp, child_op);
			auto& base_op = *reinterpret_cast<BaseOp*>(byte_p);

			return ex::start(base_op, cont...);
		}
	};

	template<IsReceiver SuffixReceiver, IsSender ChildSender>
	struct OpState 
		: InlinedReceiver<OpState<SuffixReceiver, ChildSender>, SuffixReceiver>
		, ManualChildOp<OpState<SuffixReceiver, ChildSender>, 0, 0, ChildSender>
	{
		using ChildOp = ManualChildOp<OpState<SuffixReceiver, ChildSender>, 0, 0, ChildSender>; 
		
		[[no_unique_address]] ChildSender child_sender;

		std::size_t count = 0;
		const std::size_t max = 0;

		OpState(SuffixReceiver suffix_receiver, ChildSender child_sender, size_t iterations)
			: InlinedReceiver<OpState<SuffixReceiver, ChildSender>, SuffixReceiver>{suffix_receiver}
			, ManualChildOp<OpState<SuffixReceiver, ChildSender>, 0, 0, ChildSender>{} 
			, child_sender{child_sender}
			, max{iterations}
		{}
	
		template<class... Cont>
		void start(Cont&... cont){
			if(count < max){
				count++;
				ChildOp::template construct_from<0>(child_sender);
				return ChildOp::template start<0>(cont...);
			}

			return ex::set_value.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., count);
		}

		template<std::size_t ChildIndex, std::size_t VariantIndex, std::size_t StageIndex, class... Cont, class... Arg>
        auto set_value(Cont&... cont, Arg... args){
        	return this->start(cont...);
        }

		template<std::size_t ChildIndex, std::size_t VariantIndex, std::size_t StageIndex, class... Cont, class... Arg>
        auto set_error(Cont&... cont, Arg... args){
			return ex::set_error.operator()<SuffixReceiver, Cont...>(this->get_receiver(), cont..., count);
        }
	};

	template<IsSender ChildSender>
	struct Sender {
		using value_t = std::tuple<std::size_t>;
		ChildSender child_sender;
		size_t iterations;

		auto connect(IsReceiver auto suffix_receiver){
			return OpState{suffix_receiver, child_sender, iterations};
		}
	};


}//namespace ex::algorithms::repeat


namespace ex {

	inline constexpr auto repeat = []<IsSender S>(S sender, size_t iterations){
		//explicit template to prevent copy constructor ambiguity
		return algorithms::repeat::Sender<S>{sender, iterations};
	};

	inline constexpr auto repeat_n = [](size_t iterations){
		return [=]<IsSender S>(S sender){
			return algorithms::repeat::Sender<S>{sender, iterations};
		};
	};

}//namespace ex


#endif//REPEAT_H
