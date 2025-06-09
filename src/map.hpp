#ifndef MAP_H
#define MAP_H

#include "concepts.hpp"
#include "inlined_receiver.hpp"
#include "variant_child.hpp"

namespace ex {
inline namespace algorithms_map {
	
	template<Channel channel, class SuffixReceiver, IsSender ChildSender, class Function>
	struct OpState
		: InlinedReceiver<OpState<channel, SuffixReceiver, ChildSender, Function>, SuffixReceiver>
		, VariantChildOp<OpState<channel, SuffixReceiver, ChildSender, Function>, 0, ChildSender> 
	{ 
		using OpStateOptIn = ex::OpStateOptIn;
		using Receiver = InlinedReceiver<OpState, SuffixReceiver>;
		using ChildOp = VariantChildOp<OpState, 0, ChildSender>;
		
		[[no_unique_address]] Function function;
		
		OpState(SuffixReceiver suffix_receiver, ChildSender child_sender, Function function)
			: Receiver{suffix_receiver}
			, ChildOp{child_sender}
			, function{function}
		{}

		template<class... Cont>
		auto start(Cont&... cont){
			[[gnu::musttail]] return ex::start(ChildOp::template get<0>(), cont...);
		}


		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
        auto set_value(Cont&... cont, Arg... args){
			if constexpr(channel == Channel::value){
				[[gnu::musttail]] return ex::set_value<Cont...>(this->get_receiver(), cont..., function(args...));
			} else if (channel == Channel::error){
				[[gnu::musttail]] return ex::set_value<Cont...>(this->get_receiver(), cont..., args...);
			}
        }

        template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
        auto set_error(Cont&... cont, Arg... args){
			if constexpr(channel == Channel::value){
				[[gnu::musttail]] return ex::set_error<Cont...>(this->get_receiver(), cont..., args...);
			} else if (channel == Channel::error){
				[[gnu::musttail]] return ex::set_error<Cont...>(this->get_receiver(), cont..., function(args...));
			}
        }		
	};       

	template<Channel channel1, Channel channel2, IsSender ChildSender, class Function>
	struct map_channel;

	template<Channel channel1, Channel channel2, IsSender ChildSender, class Function>
	requires (channel1 == channel2)
	struct map_channel<channel1, channel2, ChildSender, Function> {
		using type = std::tuple<apply_channel_t<channel1, Function, ChildSender>>;
	};

	template<Channel channel1, Channel channel2, IsSender ChildSender, class Function>
	requires (channel1 != channel2)
	struct map_channel<channel1, channel2, ChildSender, Function> {
		using type = channel_t<channel1, ChildSender>;
	};

	template<Channel channel, IsSender ChildSender, class Function>
	struct Sender {
		using SenderOptIn = ex::SenderOptIn;
		using value_t = map_channel<Channel::value, channel, ChildSender, Function>::type;
		using error_t = map_channel<Channel::error, channel, ChildSender, Function>::type;
	    
	    [[no_unique_address]] ChildSender child_sender;
	    [[no_unique_address]] Function function;

		template<IsReceiver SuffixReceiver>
	    constexpr auto connect(SuffixReceiver suffix_receiver){
	        return OpState<channel, SuffixReceiver, ChildSender, Function>{suffix_receiver, child_sender, function};
	    }

	};

	template<Channel channel>
	struct FunctionObject {
		template<IsSender ChildSender, class Function>
		constexpr static auto operator()(ChildSender child_sender, Function function){
			return Sender<channel, ChildSender, Function>{child_sender, function};
		}

		template<class Function>
		constexpr static auto operator()(Function function){
			return [=]<IsSender ChildSender>(ChildSender child_sender){
				return Sender<channel, ChildSender, Function>{child_sender, function};
			};
		}
	};

}}//namespace ex::algorithms_map

namespace ex {

	inline constexpr auto map_value = algorithms_map::FunctionObject<Channel::value>{};
	inline constexpr auto map_error = algorithms_map::FunctionObject<Channel::error>{};

}//namespace ex

namespace ex {

	constexpr auto operator > (ex::IsSender auto sender, auto function){
		return ex::map_value(sender, function);
	}

}//namespace ex

#endif//MAP_H
