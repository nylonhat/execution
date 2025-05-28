#ifndef LOOPBACK_CHILD_HPP
#define LOOPBACK_CHILD_HPP

namespace ex {

    template<class ParentOp>
    struct LoopbackChildOp {
		
		using OpStateOptIn = ex::OpStateOptIn;

        template<class... Cont>
        auto start(Cont&... cont){
			auto* parent = static_cast<ParentOp*>(this);
            [[gnu::musttail]] return parent->loopback(cont...);
        }
		
		auto& get(){
			return *this;
		}
        
    };   
    
}

#endif//LOOPBACK_CHILD_HPP
