#ifndef REPEAT_H
#define REPEAT_H

#include "concepts.hpp"
#include "conditional.hpp"
#include "signature.hpp"
#include "inlined_receiver.hpp"
#include "child_variant.hpp"

namespace ex {
inline namespace repeat_while_algorithm {

	template<Channel channel, IsReceiver NextRx, IsSender ChildSender1, class Predicate, class SndrFn>
	struct Op 
		: InlinedReceiver<Op<channel, NextRx, ChildSender1, Predicate, SndrFn>, NextRx>
		, ChildVariant<Op<channel, NextRx, ChildSender1, Predicate, SndrFn>, 0, ChildSender1, sender_of_apply_signature_t<SndrFn, channel_sig_t<channel, ChildSender1>> >
	{

		using OpStateOptIn = ex::OpStateOptIn;
		using Receiver = InlinedReceiver<Op, NextRx>;
		using ChildSender2 = sender_of_apply_signature_t<SndrFn, channel_sig_t<channel, ChildSender1>>;
		using ChildOps = ChildVariant<Op, 0, ChildSender1, ChildSender2>; 

		[[no_unique_address]] Predicate predicate;
		[[no_unique_address]] SndrFn sender_fn;
		
		Op(NextRx next_receiver, ChildSender1 child_sender1, Predicate predicate, SndrFn sender_fn)
			: Receiver{next_receiver}
			, ChildOps{child_sender1} 
			, predicate{predicate}
			, sender_fn{sender_fn}
		{}
	
		template<class... Cont>
		void start(Cont&... cont){
			[[gnu::musttail]] return ex::start(ChildOps::template get<0>(), cont...);
		}

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
        void set_value(Cont&... cont, Arg... args){
			//static_assert(sizeof...(cont) == 0);
        	
        	if constexpr(channel == Channel::value){
	        	if(predicate(args...)){
	        		auto child_sender2 = sender_fn(args...);
	        		auto& child_op2 = ChildOps::template construct_from<1>(child_sender2);
	        		[[gnu::musttail]] return ex::start(child_op2, cont...);
	        	}
        	}
			[[gnu::musttail]] return ex::set_value<Cont...>(Receiver::get_receiver(), cont..., args...);
        }

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
        auto set_error(Cont&... cont, Arg... args){
			if constexpr(channel == Channel::error){
	        	if(predicate(args...)){
	        		auto child_sender2 = sender_fn(args...);
	        		auto& child_op2 = ChildOps::template construct_from<1>(child_sender2);
	        		[[gnu::musttail]] return ex::start(child_op2, cont...);
	        	}
        	}
        	
			[[gnu::musttail]] return ex::set_error<Cont...>(this->get_receiver(), cont..., args...);
        }
	};

	template<Channel channel, IsSender ChildSender1, class Predicate, class SndrFn>
	struct Sender {
		using SenderOptIn = ex::SenderOptIn;
		using value_t = ChildSender1::value_t;
		using error_t = ChildSender1::error_t;

		ChildSender1 child_sender1;
		Predicate predicate;
		SndrFn sender_fn;

		template<IsReceiver NextRx>
		auto connect(NextRx next_receiver){
			return Op<channel, NextRx, ChildSender1, Predicate, SndrFn>{next_receiver, child_sender1, predicate, sender_fn};
		}
	};

	template<Channel channel>
	struct FnObj {
		template<IsSender Sender1, class Predicate, class SndrFn>
		static auto operator()(Sender1 sender1, Predicate predicate, SndrFn sender_fn){
			return Sender<channel, Sender1, Predicate, SndrFn>{sender1, predicate, sender_fn};
		}
		
		template<class Predicate, class SndrFn>
		static auto operator()(Predicate predicate, SndrFn sender_fn){
			return [=]<IsSender Sender1>(Sender1 sender1){
				return FnObj::operator()(sender1, predicate, sender_fn);
			};
		}
		
	};


}}//namespace ex::repeat_while_algorithm


namespace ex {

	template<Channel channel>
	inline constexpr auto repeat_while = repeat_while_algorithm::FnObj<channel>{};

	inline constexpr auto repeat_while_value = repeat_while_algorithm::FnObj<Channel::value>{};
	inline constexpr auto repeat_while_error = repeat_while_algorithm::FnObj<Channel::error>{};

	
}//namespace ex

#include "pure.hpp"

namespace ex {
inline namespace repeat_n_algorithm {
	
	template<Channel channel>
	struct FnObj {
				
		template<IsSender Sender1>
		static auto operator()(Sender1 sender1, const std::size_t iterations){
			auto loop = [counter = iterations](auto...) mutable {
				return counter-- > 0;
			};

			auto sender_fn = [sender1](auto...){
				return sender1;
			};

			auto initial_sender = std::apply(pure<channel>, std::get<0>(typename Sender1::value_t{}));
			return ex::repeat_while<channel>(initial_sender, loop, sender_fn);
		}

		static auto operator()(const std::size_t iterations){
			return [=]<IsSender Sender1>(Sender1 sender1){
				return FnObj::operator()(sender1, iterations);
			};
		}
	};
	
}}//namespace ex::repeat_n_algorithm


namespace ex {

	template<Channel channel>
	inline constexpr auto repeat_n = repeat_n_algorithm::FnObj<channel>{};
	
	inline constexpr auto repeat_n_value = repeat_n_algorithm::FnObj<Channel::value>{};
	inline constexpr auto repeat_n_error = repeat_n_algorithm::FnObj<Channel::error>{};

	
}//namespace ex

#endif//REPEAT_H
