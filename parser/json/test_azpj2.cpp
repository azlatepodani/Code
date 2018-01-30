
#include <utility>
#include <cstdint>
#include <array>
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ratio>
#if defined(_MSC_VER)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif // _MSC_VER
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

#if defined(_MSC_VER)
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

#else
	std::string loadFile(const char * path) {
		std::string str;
		std::ifstream stm(path, std::ios::binary);
		
		if (!stm.good()) {
			std::cout << "cannot open file\n";
			return std::string();
		}
		
		stm.seekg(0, std::ios_base::end);
		auto size = stm.tellg();
		stm.seekg(0, std::ios_base::beg);
		
		str.resize(size);
		stm.read(&str[0], size);
		return str;
	}
#endif // _MSC_VER

std::string writeJson(const JsonValue& root) {
	std::string stm;
	json_writer(stm, root);
	return stm;
}


template <typename Fn>
void benchmark(int, const char * desc, Fn alg)
{
	int i=0;
	int maxi = 1500;
	
	auto start = std::chrono::steady_clock::now();
	
	for (; i<maxi; ++i) {
		alg();
	}
	
	auto end = std::chrono::steady_clock::now();
	auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end-start);
	
	printf("%s  time=%dus\n", desc, (int)(diff.count()/maxi));
}

template <typename Fn>
void benchmark(const char * desc, Fn alg)
{
	benchmark(0, desc, alg);
}


void check() {
	JsonValue v(1.E10 / 3);
	std::string j;
	json_writer(j, v);
	auto p = json_reader(j);
	
	if (p.first.type == JsonValue::FLOAT_NUM && p.first.u.float_num == v.u.float_num) {
		printf("check ok\n");
	}
	else {
		printf("check failed\n");
	}
}


#if defined(_MSC_VER)
int wmain(int, PWSTR argv[])
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	if (GetThreadPriority(GetCurrentThread()) != THREAD_PRIORITY_HIGHEST) printf("Priority set failed\n");
	 
	if (!SetThreadAffinityMask(GetCurrentThread(), 1)) printf("Affinity set failed\n");

#else
int main(int, char* argv[]) {
#endif

	check();
	 
	auto str = loadFile(argv[1]);
	
	benchmark("Json API load",  [&str](){parseJson(str);});
	auto root = parseJson(str);
	benchmark("Json API write", [&root,&str](){/*if (str != */writeJson(root.first)/*) __debugbreak()*/;});
	
	printf("\n");
	return 0;
}


