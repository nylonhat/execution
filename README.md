Experimental Async Library based on Senders and Recievers (p2300; std::execution).
  - Testing Continuation Passing Style based implementation
    - More concise assembly generation
- Tail call optimisation by compiler without explicit trampolining
  - Testing operator overloading syntax for: fmap, monadic bind, function application
  - Continuation Stealing implementation of scheduling algorithms

```
int main(){
	auto monadic = pure(42) > pure >= id | sync_wait;
	std::println("Final result: {}", monadic);

	Threadpool pool = {};
	auto parallel = branch(pool, pure(42), pure(69)) 
			> add
			| repeat_n(100'000'000) 
			| benchmark 
			| sync_wait;

	std::println("Final result: {}", parallel);

	return 0;
}
```

