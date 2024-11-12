#ifndef REPEAT_H
#define REPEAT_H

#include "sender.hpp"
#include "op_state.hpp"
#include "receiver.hpp"

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
	struct OpState {
		using Self = OpState<SuffixReceiver, ChildSender>;
		using InfixReceiver = Receiver<Self>;
		using ChildOp = connect_t<ChildSender, InfixReceiver>;
		using NextReceiver = SuffixReceiver;
	
		[[no_unique_address]] NextReceiver next_receiver;

		union{
			ChildOp child_op;
		};
	
		[[no_unique_address]] ChildSender child_sender;

		std::size_t count = 0;
		const std::size_t max = 0;

		OpState(ChildSender child_sender, SuffixReceiver suffix_receiver, size_t iterations)
			: next_receiver{suffix_receiver}
			, child_sender{child_sender}
			, max{iterations}
		{}
	
		template<IsOpState... Cont>
		auto start(Cont&... cont){
			if(count < max){
				count++;
				new (&child_op) ChildOp (ex::connect(child_sender, InfixReceiver{}));
				return ex::start(child_op, cont...);
			}

			return ex::set_value.operator()<NextReceiver, Cont...>(
			    next_receiver, 
			    cont..., 
			    count
			);
		}

	};

	template<IsSender ChildSender>
	struct Sender {
		using value_t = std::tuple<std::size_t>;
		ChildSender child_sender;
		size_t iterations;

		auto connect(IsReceiver auto suffix_receiver){
			return OpState{child_sender, suffix_receiver, iterations};
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
