Experimental Async Library based on Senders and Recievers (p2300; std::execution).
- Testing Continuation Passing Style based implementation
	- Tail call optimisation by compiler without explicit trampolining
	- gcc : [[gnu::musttail]]
- Testing operator overloading syntax for: fmap, monadic bind, function application
- Continuation Stealing implementation of scheduling algorithms

```
constexpr inline auto add = [](auto... v) {
	return (... + v);
};

constexpr inline auto identity = [](auto i) {
	return i;
};

int main(){
	///////////////
	//Monadic style
	///////////////
	auto meaning_of_life = ex::value(42) > ex::value >= identity | ex::sync_wait;

	///////////////////////////////////////////////
	//Continuation stealing branching on threadpool
	///////////////////////////////////////////////
	ex::Threadpool<16> threadpool{};
	auto scheduler = threadpool.scheduler();
	
    	auto branching = ex::value(4)
    		| ex::branch(scheduler, ex::value(1))
		| ex::branch(scheduler, ex::value(42)) 
		| ex::branch(scheduler, ex::value(69)) 
		| ex::branch(scheduler, ex::value(111)) 
    		| ex::map_value(add)
    		| ex::sync_wait;
	
	//////////////////////////////////////////////////////
	//Async chunk accumulate using generic fold algorithm
	/////////////////////////////////////////////////////
	std::size_t min = 0;
	std::size_t max = 1000000;
	
	//Range of Senders of chunks views
	auto sender_range = std::views::iota(min, max)
		| std::views::chunk(max/16)
		| std::views::transform(ex::value);

	//Synchronous folding
	auto fold_chunk = [=](auto chunk_view){
		return std::ranges::fold_left(chunk_view, min, add);
	};

	//Async folding on each chunk
	auto async_fold_chunk = [=](auto chunk_view){
		return ex::value(chunk_view) 
		     | ex::map_value(fold_chunk);
	};

	//Async fold with max concurrency of 16
	auto sum = ex::fold_on<16>(scheduler, sender_range, async_fold_chunk, min, add)
		| ex::sync_wait;

	return 0;
}
```

