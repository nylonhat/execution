Experimental Async Library based on Senders and Recievers (p2300; std::execution).
  - Testing Continuation Passing Style based implementation for better asm generation
  - Testing operator overloaded syntax

```
int main(){
	auto r = pure(42) > pure >= id | sync_wait;
	std::println("Final result: {}", r);
	return 0;
}
```

