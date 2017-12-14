
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
extern "C" {
#include "swhealthex/JSON_parser.h"
}
#include "swhealthex/json_api.h"


asu::JsonValue parseJson(const std::vector<char>& doc) {
	asu::JsonValue root;
	try { asu::json_reader(doc, root); }
	catch (std::exception& e) {
		std::cout << "parse failure  " << e.what() << '\n';
	}
	return root;
}


std::vector<char> loadFile(const wchar_t * path) {
	std::vector<char> str;
	auto h = CreateFileW(path, GENERIC_READ, 0,0, OPEN_EXISTING, 0,0);
	if (h == INVALID_HANDLE_VALUE) {
		std::cout << "cannot open file  " << GetLastError() << '\n';
		return std::vector<char>();
	}
	auto size = GetFileSize(h, 0);
	str.resize(size);
	if (!ReadFile(h, &str[0], str.size(), 0,0)) {
		std::cout << "cannot read file  " << GetLastError() << '\n';
	}
	CloseHandle(h);
	return str;
}


std::string writeJson(const asu::JsonValue& root) {
	std::stringstream stm;
	asu::json_writer(stm, root);
	return stm.str();
}


template <typename Fn>
void benchmark(int size, char * desc, Fn alg)
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
	printf("%s %d  time=%dus,  time/n=%f, time/nlogn=%f\n", desc, size, (int)time, 
				float(time)/size, float(time/(size*19.931568569324)));
}

template <typename Fn>
void benchmark(char * desc, Fn alg)
{
	benchmark(0, desc, alg);
}


void wmain(int argc, PWSTR argv[]) {
	 
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	if (GetThreadPriority(GetCurrentThread()) != THREAD_PRIORITY_HIGHEST) printf("Priority set failed\n");
	 
	if (!SetThreadAffinityMask(GetCurrentThread(), 1)) printf("Affinity set failed\n");
	 
	auto str = loadFile(argv[1]);
	
	benchmark("asu::Json API load",  [&str](){parseJson(str);});
	auto root = parseJson(str);
	benchmark("asu::Json API write", [&root](){writeJson(root);});
	
	printf("\n");
}


