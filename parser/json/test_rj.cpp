
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
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"


using namespace rapidjson;


// https://github.com/Tencent/rapidjson

Document parseJson(const std::string& doc) {
	Document d;
    d.Parse(doc.c_str());

	return d;
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


std::string writeJson(const Document& d) {
	StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    d.Accept(writer);

	return buffer.GetString();
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


static std::string double_to_string(double dbl) {
    char buf[340] = { 0, };
    auto len = sprintf(buf, "%.*g", 17, dbl);
    return std::string(buf, buf+len);
}


void check() {
	StringBuffer s;
    Writer<StringBuffer> writer(s);

	writer.Double(1. / 3);
	
	auto str = double_to_string(1. / 3);
	
	 if (str == s.GetString()) {
		printf("check ok\n");
	}
	else {
		printf("check failed  %s  %s\n", str.c_str(), s.GetString());
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
	benchmark("Json API write", [&root,&str](){/*if (str != */writeJson(root)/*) __debugbreak()*/;});
	
	printf("\n");
	return 0;
}


