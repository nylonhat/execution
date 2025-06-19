#ifndef BIND_H
#define BIND_H

#include "concepts.hpp"
#include "inlined_receiver.hpp"
#include "child_variant.hpp"

namespace ex {
inline namespace bind_algorithm {
		
	template<Channel channel, IsReceiver NextRx, IsSender Sender1, class SndrFn>
	struct Op 
		: InlinedReceiver<Op<channel, NextRx, Sender1, SndrFn>, NextRx>
		, ChildVariant<Op<channel, NextRx, Sender1, SndrFn>, 0, Sender1, apply_channel_t<channel, SndrFn, Sender1>>
	{

		using OpStateOptIn = ex::OpStateOptIn;
		using Sender2 = apply_channel_t<channel, SndrFn, Sender1>;
		using Receiver = InlinedReceiver<Op, NextRx>;
		using ChildOps = ChildVariant<Op, 0, Sender1, Sender2>;

		[[no_unique_address]] SndrFn sender_fn;
		
		Op(NextRx next_receiver, Sender1 sender1, SndrFn sender_fn)
			: Receiver{next_receiver}
			, ChildOps{sender1}
			, sender_fn{sender_fn}
		{}
		
		template<class... Cont>
		auto start(Cont&... cont){
			return ex::start(ChildOps::template get<0>(), cont...);
		}

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
			requires (VariantIndex == 0)
        auto set_value(Cont&... cont, Arg... args){
			if constexpr(channel == Channel::value){
				auto& child_op2 = ChildOps::template construct_from<1>(sender_fn(args...));
				return ex::start(child_op2, cont...);
			} else if (channel == Channel::error){
				return ex::set_value<Cont...>(this->get_receiver(), cont..., args...);			
			}
        }

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
			requires (VariantIndex == 0)
        auto set_error(Cont&... cont, Arg... args){
			if constexpr(channel == Channel::value){
				return ex::set_error<Cont...>(this->get_receiver(), cont..., args...);			
			} else if (channel == Channel::error){
				auto& child_op2 = ChildOps::template construct_from<1>(sender_fn(args...));
				return ex::start(child_op2, cont...);
			}
        }

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
			requires (VariantIndex == 1)
        auto set_value(Cont&... cont, Arg... args){
			return ex::set_value<Cont...>(this->get_receiver(), cont..., args...);			
        }

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
			requires (VariantIndex == 1)
        auto set_error(Cont&... cont, Arg... args){
			return ex::set_error<Cont...>(this->get_receiver(), cont..., args...);			
        }
		
	};

	template<Channel channel1, Channel channel2, IsSender Sender1, class SndrFn>
	struct bind_channel;

	template<Channel channel1, Channel channel2, IsSender Sender1, class SndrFn>
	requires (channel1 == channel2)
	struct bind_channel<channel1, channel2, Sender1, SndrFn>{
		using type = channel_t<channel1, apply_channel_t<channel1, SndrFn, Sender1>>;
	};

	template<Channel channel1, Channel channel2, IsSender Sender1, class SndrFn>
	requires (channel1 != channel2)
	struct bind_channel<channel1, channel2, Sender1, SndrFn>{
		using type = channel_t<channel1, Sender1>;
	};

	template<Channel channel, IsSender Sender1, class SndrFn>
	struct Sender {
		using SenderOptIn = ex::SenderOptIn;
		using value_t = bind_channel<Channel::value, channel, Sender1, SndrFn>::type;
		using error_t = bind_channel<Channel::error, channel, Sender1, SndrFn>::type;

		[[no_unique_address]] Sender1 sender1;
		[[no_unique_address]] SndrFn sender_fn;

		Sender(Sender1 sender1, SndrFn sender_fn)
			: sender1{sender1}
			, sender_fn{sender_fn}
		{}
		
		template<IsReceiver NextRx>
		auto connect(NextRx next_receiver){
			return Op<channel, NextRx, Sender1, SndrFn>{next_receiver, sender1, sender_fn};
		}
	};

	template<Channel channel>
	struct FnObj {
		template<IsSender Sender1, class SndrFn>
		static auto operator()(Sender1 sender1, SndrFn sender_fn){
			return Sender<channel, Sender1, SndrFn>{sender1, sender_fn};
		}

		template<class SndrFn>
		static auto operator()(SndrFn sender_fn){
			return [=]<IsSender Sender1>(Sender1 sender1){
				return Sender<channel, Sender1, SndrFn>{sender1, sender_fn};
			};
		}
	};

}}//namespace ex::bind_algorithm

namespace ex {

	inline constexpr auto bind_value = bind_algorithm::FnObj<Channel::value>{};
	inline constexpr auto bind_error = bind_algorithm::FnObj<Channel::error>{};

}//namespace ex


auto operator >= (ex::IsSender auto sender, auto sender_fn){
	return ex::bind_value(sender, sender_fn);
}

#endif//BIND_H
