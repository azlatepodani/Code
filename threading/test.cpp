#include <stdio.h>
#include <thread>
#include "callbacks.h"
#include <windows.h>

#pragma comment(lib, "Kernel32.lib")

struct Foo {
	void operator()(const char * str) {
		printf(str);
	}
};


struct Bar {
	void operator()(const char * str) {
		printf(str);
	}
};


void scenario1() {
	CallbackManager<Foo, Bar> cm((Foo()), (Bar()));
	
	auto t1 = std::thread([&cm]() {
		for (int i=0; i<200; i++) {
			cm.call_f();
			cm.call_f();
			cm.call_f();
			cm.call_f();
			cm.call_f();
			cm.call_g();
			cm.call_g();
			cm.call_g();
			cm.call_g();
			cm.call_f();
			Sleep(1);
		}
	});
	
	auto t2 = std::thread([&cm]() {
		for (int i=0; i<200; i++) {
			cm.call_g();
			cm.call_g();
			cm.call_g();
			cm.call_g();
			cm.call_f();
			cm.call_f();
			cm.call_f();
			cm.call_f();
			cm.call_f();
			Sleep(1);
		}
	});
	
	
	CallbackManager2<Foo, Bar> cm2((Foo()), (Bar()));
	auto t3 = std::thread([&cm2]() {
		for (int i=0; i<200; i++) {
			cm2.call_f();
			cm2.call_g();
			cm2.call_f();
			Sleep(1);
			cm2.call_g();
			cm2.call_f();
			cm2.call_g();
		}
	});
	
	auto t4 = std::thread([&cm2]() {
		for (int i=0; i<200; i++) {
			cm2.call_g();
			cm2.call_f();
			cm2.call_g();
			Sleep(1);
			cm2.call_f();
			cm2.call_g();
			cm2.call_f();
		}
	});

	Sleep(150);
	printf("invalidating\n");
	cm.invalidate();
	cm2.invalidate();
	
	printf("joining\n");
	
	t1.join();
	t2.join();
	t3.join();
	t4.join();
}


long aaa;
long space[10];
long bbb;

struct Fiz {
	void operator()(const char * str) {
		*(volatile long*)&aaa = *str;
	}
};


struct Baz {
	void operator()(const char * str) {
		*(volatile long*)&bbb = *str;
	}
};


void scenario2() {
	//CallbackManager<Fiz, Baz> cm((Fiz()), (Baz()));
	//CallbackManager3<Fiz, Baz> cm((Fiz()), (Baz()));
	CallbackManager3<Fiz, Baz> cm((Fiz()), (Baz()));
	LARGE_INTEGER f;
	QueryPerformanceFrequency(&f);
	printf("frequency=%I64d\n", f.QuadPart);
	
	auto t1 = std::thread([&cm]() {
		LARGE_INTEGER a,b;
		QueryPerformanceCounter(&a);
		
		for (int i=0; i<1000000; i++) {
			/*cm.call_f(0);cm.call_f(0);cm.call_f(0);cm.call_f(0);
			cm.call_f(0);cm.call_f(0);cm.call_f(0);cm.call_f(0);*/
			cm.call_f();cm.call_f();cm.call_f();cm.call_f();
			cm.call_f();cm.call_f();cm.call_f();cm.call_f();
		}
		
		QueryPerformanceCounter(&b);
		printf("1 tid %d  time=%I64d\n", GetCurrentThreadId(), b.QuadPart-a.QuadPart);
	});
	
	auto t2 = std::thread([&cm]() {
		LARGE_INTEGER a,b;
		QueryPerformanceCounter(&a);
		
		for (int i=0; i<1000000; i++) {
			/*cm.call_g(1);cm.call_g(1);cm.call_g(1);cm.call_g(1);
			cm.call_g(1);cm.call_g(1);cm.call_g(1);cm.call_g(1);*/
			cm.call_g();cm.call_g();cm.call_g();cm.call_g();
			cm.call_g();cm.call_g();cm.call_g();cm.call_g();
		}
		
		QueryPerformanceCounter(&b);
		printf("1 tid %d  time=%I64d\n", GetCurrentThreadId(), b.QuadPart-a.QuadPart);
	});
	
	t1.join();
	t2.join();
	
	CallbackManager2<Fiz, Baz> cm2((Fiz()), (Baz()));
	//CallbackManager3<Fiz, Baz> cm2((Fiz()), (Baz()));
	auto t3 = std::thread([&cm2]() {
		LARGE_INTEGER a,b;
		QueryPerformanceCounter(&a);
		
		for (int i=0; i<1000000; i++) {
			cm2.call_f();cm2.call_f();cm2.call_f();cm2.call_f();
			cm2.call_f();cm2.call_f();cm2.call_f();cm2.call_f();
		}
		
		QueryPerformanceCounter(&b);
		printf("2 tid %d  time=%I64d\n", GetCurrentThreadId(), b.QuadPart-a.QuadPart);
	});
	
	auto t4 = std::thread([&cm2]() {
		LARGE_INTEGER a,b;
		QueryPerformanceCounter(&a);
		
		for (int i=0; i<1000000; i++) {
			cm2.call_g();cm2.call_g();cm2.call_g();cm2.call_g();
			cm2.call_g();cm2.call_g();cm2.call_g();cm2.call_g();
		}
		
		QueryPerformanceCounter(&b);
		printf("2 tid %d  time=%I64d\n", GetCurrentThreadId(), b.QuadPart-a.QuadPart);
	});
	

	t3.join();
	t4.join();

}

void main() {
	//scenario1();
	scenario2();
}

