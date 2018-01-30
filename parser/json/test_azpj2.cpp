
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
// #define WIN32_LEAN_AND_MEAN
// #include <windows.h>
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


// std::string loadFile(const wchar_t * path) {
	// std::string str;
	// auto h = CreateFileW(path, GENERIC_READ, 0,0, OPEN_EXISTING, 0,0);
	// if (h == INVALID_HANDLE_VALUE) {
		// std::cout << "cannot open file  " << GetLastError() << '\n';
		// return std::string();
	// }
	// auto size = GetFileSize(h, 0);
	// str.resize(size);
	// if (!ReadFile(h, &str[0], str.size(), 0,0)) {
		// std::cout << "cannot read file  " << GetLastError() << '\n';
	// }
	// CloseHandle(h);
	// return str;
// }


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


std::string writeJson(const JsonValue& root) {
	std::string stm;
	json_writer(stm, root);
	return stm;
}


template <typename Fn>
void benchmark(int size, const char * desc, Fn alg)
{
	//long long time = 0;
	// LARGE_INTEGER li, li2, freq;
	
	// QueryPerformanceFrequency(&freq);
	 
	// int i=0;
	// int maxi = 1500;
	
	// QueryPerformanceCounter(&li);
	
	// for (; i<maxi; ++i) {
		// alg();
	// }
	
	
	int i=0;
	int maxi = 1500;
	
	auto start = std::chrono::steady_clock::now();
	
	for (; i<maxi; ++i) {
		alg();
	}
	
	auto end = std::chrono::steady_clock::now();
	auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end-start);
	
	printf("%s %d  time=%dus\n", desc, size, (int)(diff.count()/maxi));
	
	// QueryPerformanceCounter(&li2);
	// time += li2.QuadPart - li.QuadPart;
	
	// time = time * 1000000 / (freq.QuadPart * i);
	// printf("%s %d  time=%dus,  time/n=%f, time/nlogn=%f\n", desc, size, (int)time, 
				// float(time)/size, float(time/(size*19.931568569324)));
}

template <typename Fn>
void benchmark(const char * desc, Fn alg)
{
	benchmark(0, desc, alg);
}


//void wmain(int argc, PWSTR argv[]) {
int main(int argc, char* argv[]) {
	 
	// SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	// if (GetThreadPriority(GetCurrentThread()) != THREAD_PRIORITY_HIGHEST) printf("Priority set failed\n");
	 
	// if (!SetThreadAffinityMask(GetCurrentThread(), 1)) printf("Affinity set failed\n");
	 
	auto str = loadFile(argv[1]);
	
	benchmark("Json API load",  [&str](){parseJson(str);});
	auto root = parseJson(str);
	benchmark("Json API write", [&root,&str](){/*if (str != */writeJson(root.first)/*) __debugbreak()*/;});
	
	printf("\n");
	return 0;
}


