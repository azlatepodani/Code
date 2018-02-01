#include <chrono>
#include <ratio>
#include <stdio.h>


template <typename T, typename Op>
T apply(T l, T r, Op op) {
	return op(l, r);
}

template <typename T, typename Op>
T apply(T l, Op op) {
	return op(l);
}

template <typename Op>
void time_op(Op op, int times) {
	auto start = std::chrono::steady_clock::now();
	
	op();
	
	auto end = std::chrono::steady_clock::now();
	auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start);
	
	printf("%7.2f", (float)(diff.count()) / times);
}


template <typename T>
void benchmark_char(T t[1024]) {
	T l[1024];
	T r[1024];

	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; i+=1) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return a + b;	});
		}
	}, 1024*20480);
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return a - b;	});
		}
	}, 1024*20480);
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return a * b;	});
		}
	}, 1024*20480);
	
	// remove 0s from the divisor
	for (int i=0; i<1024; ++i) { r[i] = r[i] ? r[i] : 1; }
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return a / b;	});
		}
	}, 1024*20480);
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return a % b;	});
		}
	}, 1024*20480);
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return a ^ b;	});
		}
	}, 1024*20480);
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return a | b;	});
		}
	}, 1024*20480);
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return a & b;	});
		}
	}, 1024*20480);
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return T(a || b); });
		}
	}, 1024*20480);
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return T(a && b); });
		}
	}, 1024*20480);
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], [](auto a) { return T(!a); });
		}
	}, 1024*20480);
	
	for (int i=0; i<1024; ++i) {
		r[i] = (unsigned)r[i] & 31;
	}
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return T(a << b); });
		}
	}, 1024*20480);

	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return T(a >> b); });
		}
	}, 1024*20480);
}

template <typename T>
void benchmark_float(T t[1024]) {
	T l[1024];
	T r[1024];
	
	for (int i=0; i<1024; ++i) {
		if (!isfinite(r[i])) r[i] = 1.2;
	}

	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; i+=1) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return a + b;	});
		}
	}, 1024*20480);
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return a - b;	});
		}
	}, 1024*20480);
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return a * b;	});
		}
	}, 1024*20480);
	
	// remove 0s from the divisor
	for (int i=0; i<1024; ++i) { r[i] = r[i] ? r[i] : 1; }
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return a / b;	});
		}
	}, 1024*20480);
	
	time_op([&](){
		// for (int j=0; j<20480; ++j)
		// for (int i=0; i<1024; ++i) {
			// t[i] = apply(l[i], r[i], [](auto a, auto b) { return 0;	});
		// }
	}, 1024*20480);
	
	time_op([&](){
		// for (int j=0; j<20480; ++j)
		// for (int i=0; i<1024; ++i) {
			// t[i] = apply(l[i], r[i], [](auto a, auto b) { return 0;	});
		// }
	}, 1024*20480);
	
	time_op([&](){
		// for (int j=0; j<20480; ++j)
		// for (int i=0; i<1024; ++i) {
			// t[i] = apply(l[i], r[i], [](auto a, auto b) { return 0;	});
		// }
	}, 1024*20480);
	
	time_op([&](){
		// for (int j=0; j<20480; ++j)
		// for (int i=0; i<1024; ++i) {
			// t[i] = apply(l[i], r[i], [](auto a, auto b) { return 0;	});
		// }
	}, 1024*20480);
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return T(a || b); });
		}
	}, 1024*20480);
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], r[i], [](auto a, auto b) { return T(a && b); });
		}
	}, 1024*20480);
	
	time_op([&](){
		for (int j=0; j<20480; ++j)
		for (int i=0; i<1024; ++i) {
			t[i] = apply(l[i], [](auto a) { return T(!a); });
		}
	}, 1024*20480);
	
	time_op([&](){
		// for (int j=0; j<20480; ++j)
		// for (int i=0; i<1024; ++i) {
			// t[i] = apply(l[i], r[i], [](auto a, auto b) { return T(a << b); });
		// }
	}, 1024*20480);

	time_op([&](){
		// for (int j=0; j<20480; ++j)
		// for (int i=0; i<1024; ++i) {
			// t[i] = apply(l[i], r[i], [](auto a, auto b) { return T(a >> b); });
		// }
	}, 1024*20480);

}

	// [] + - * / % ^ | & || && ! 

// char 
// short
// int 
// long long
// float
// double


int main() {
	printf("     +      -      *      /      %%       ^      |      &     ||    &&     !      <<      >>\n");
	{
	char t[1024];
	benchmark_char(t);
	printf("   char\n");
	}
		{
	short t[1024];
	benchmark_char(t);
	printf("   short\n");
	}
	{
	int t[1024];
	benchmark_char(t);
	printf("   int\n");
	}
	{
	long long t[1024];
	benchmark_char(t);
	printf("   int64\n");
	}
	{
	float t[1024];
	benchmark_float(t);
	printf("   float\n");
	}
	{
	double t[1024];
	benchmark_float(t);
	printf("   double\n");
	}
	{
	unsigned char t[1024];
	benchmark_char(t);
	printf("   uhar\n");
	}
		{
	unsigned short t[1024];
	benchmark_char(t);
	printf("   ushort\n");
	}
	{
	unsigned int t[1024];
	benchmark_char(t);
	printf("   uint\n");
	}
	{
	unsigned long long t[1024];
	benchmark_char(t);
	printf("   uint64\n");
	}
	
}