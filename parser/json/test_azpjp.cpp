
#include <utility>
#include <cstdint>
#include <array>
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <chrono>
#include <ratio>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "azp_json.h"


using namespace azp;

parser_t parseJson(const std::string& odoc) {
	parser_t p;
	//std::string doc = odoc;
	if (!parseJson(p, &odoc[0], &odoc[0]+odoc.size())) {
		std::cout << "parse failure\n";
	}
	return p;
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
	if (!ReadFile(h, &str[0], str.size(), 0,0)) {
		std::cout << "cannot read file  " << GetLastError() << '\n';
	}
	CloseHandle(h);
	return str;
}


// std::string writeJson(const asu::JsonValue& root) {
	// std::stringstream stm;
	// asu::json_writer(stm, root);
	// return stm.str();
// }


template <typename Fn>
void benchmark(int size, char * desc, Fn alg)
{ 
	int i=0;
	int maxi = 1500;
	
	auto start = std::chrono::steady_clock::now();
	
	for (; i<maxi; ++i) {
		alg();
	}
	
	auto end = std::chrono::steady_clock::now();
	auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end-start);
	
	printf("%s %d  time=%dus\n", desc, size, (int)(diff.count()/maxi));
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
	// auto root = parseJson(str);
	// benchmark("asu::Json API write", [&root](){writeJson(root);});
	
	printf("\n");
}


