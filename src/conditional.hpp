#ifndef CONDITIONAL_HPP
#define CONDITIONAL_HPP

#include "concepts.hpp"
#include "signature.hpp"
#include "inlined_receiver.hpp"
#include "child_variant.hpp"

namespace ex {
inline namespace channel_else_algorithm {

	template<Channel channel, IsReceiver NextRx, class Predicate, IsSender ChildSender>
	struct Op 
		: InlinedReceiver<Op<channel, NextRx, Predicate, ChildSender>, NextRx>
		, ChildVariant<Op<channel, NextRx, Predicate, ChildSender>, 0, ChildSender>
	{
		
		using OpStateOptIn = ex::OpStateOptIn;
		using Receiver = InlinedReceiver<Op, NextRx>;
		using ChildOp =  ChildVariant<Op, 0, ChildSender>;

		Predicate predicate;

		Op(NextRx suffix_receiver, Predicate predicate, ChildSender child_sender)
			: Receiver{suffix_receiver}
			, ChildOp{child_sender}
			, predicate{predicate}
		{}

		template<class... Cont>
		auto start(Cont&... cont){
			return ChildOp::template start<0>(cont...);
		}

		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
        auto set_value(Cont&... cont, Arg... arg){
        	if constexpr(channel == Channel::value){
	        	if(!std::invoke(predicate, arg...)){
	        		return ex::set_error<Cont...>(this->get_receiver(), cont..., arg...);
	        	}
        	}
    		return ex::set_value<Cont...>(this->get_receiver(), cont..., arg...);
        }

        
		template<std::size_t ChildIndex, std::size_t VariantIndex, class... Cont, class... Arg>
        auto set_error(Cont&... cont, Arg... arg){
        	if constexpr(channel == Channel::error){
	        	if(!std::invoke(predicate, arg...)){
	        		return ex::set_value<Cont...>(this->get_receiver(), cont..., arg...);
	        	}
        	}
    		return ex::set_error<Cont...>(this->get_receiver(), cont..., arg...);
        }
		
	};     


	template<Channel channel, class Predicate, IsSender ChildSender>
	struct Sender {
		using SenderOptIn = ex::SenderOptIn;
		using value_t = ChildSender::value_t;
		using error_t = ChildSender::value_t;
		
	    [[no_unique_address]] Predicate predicate;
	    [[no_unique_address]] ChildSender child_sender;

		template<IsReceiver NextRx>
	    auto connect(NextRx suffix_receiver){
	    	return Op<channel, NextRx, Predicate, ChildSender>{suffix_receiver, predicate, child_sender};
	    }

	};

	template<Channel channel>
	struct FnObj{

		template<class Predicate, IsSender ChildSender>
		static auto operator()(ChildSender child_sender, Predicate predicate){
			return Sender<channel, Predicate, ChildSender>{predicate, child_sender};
		}

		template<class Predicate>
		static auto operator()(Predicate predicate){
			return [=]<IsSender ChildSender>(ChildSender child_sender){
				return Sender<channel, Predicate, ChildSender>{predicate, child_sender};
			};
		}
	};

}}//namespace ex::channel_else_algorithm

namespace ex {

	inline constexpr auto value_else_error = channel_else_algorithm::FnObj<Channel::value>{};
	inline constexpr auto error_else_value = channel_else_algorithm::FnObj<Channel::error>{};

}//namespace ex


#endif//CONDITIONAL_HPP
