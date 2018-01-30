
#include <utility>
#include <cstdint>
#include <array>
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "azp_json_api.h"


using namespace azp;


std::pair<JsonValue, std::string> parseJson(const std::string& doc) {
	std::pair<JsonValue, std::string> root;
	try { root = json_reader(doc); }
	catch (std::exception& e) {
		std::cout << "parse failure  " << e.what() << '\n';
	}
	return root;
}


std::string loadFile(const wchar_t * path) {
	std::string str;
	auto h = CreateFileW(path, GENERIC_READ, 0,0, OPEN_EXISTING, 0,0);
	if (h == INVALID_HANDLE_VALUE) {
		std::cout << "cannot open file  " << GetLastError() << '\n';
		return std::string();
	}
	auto size = GetFileSize(h, 0);
	str.resize(size);
	if (!ReadFile(h, &str[0], (ULONG)str.size(), 0,0)) {
		std::cout << "cannot read file  " << GetLastError() << '\n';
	}
	CloseHandle(h);
	return str;
}


std::string writeJson(const JsonValue& root) {
	std::string stm;
	json_writer(stm, root);
	return stm;
}


template <typename Fn>
void benchmark(int, const char * desc, Fn alg)
{
	long long time = 0;
	LARGE_INTEGER li, li2, freq;
	
	QueryPerformanceFrequency(&freq);
	 
	int i=0;
	int maxi = 1500;
	
	QueryPerformanceCounter(&li);
	
	for (; i<maxi; ++i) {
		alg();
	}
	
	QueryPerformanceCounter(&li2);
	time += li2.QuadPart - li.QuadPart;
	
	time = time * 1000000 / (freq.QuadPart * i);
	printf("%s time=%dus\n", desc, (int)time);
}

template <typename Fn>
void benchmark(const char * desc, Fn alg)
{
	benchmark(0, desc, alg);
}


void wmain(int, PWSTR argv[]) {
	 
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	if (GetThreadPriority(GetCurrentThread()) != THREAD_PRIORITY_HIGHEST) printf("Priority set failed\n");
	 
	if (!SetThreadAffinityMask(GetCurrentThread(), 1)) printf("Affinity set failed\n");
	 
	auto str = loadFile(argv[1]);
	
	benchmark("Json API load",  [&str](){parseJson(str);});
	auto root = parseJson(str);
	benchmark("Json API write", [&root,&str](){/*if (str != */writeJson(root.first)/*) __debugbreak()*/;});
	
	printf("\n");
}


