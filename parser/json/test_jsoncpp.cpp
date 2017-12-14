
#include <utility>
#include <cstdint>
#include <array>
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "json/value.h"
#include "json/writer.h"
#include "json/reader.h"
#include "asw\generic\strings.h"


// https://github.com/open-source-parsers/jsoncpp

Json::Value parseJson(const std::string& doc) {
	Json::Value root;
	if (!Json::Reader().parse(doc, root)) {
		std::cout << "parse failure\n";
	}
	return root;
}


std::string loadFile(const wchar_t * path) {
	std::string str;
	auto h = CreateFileW(path, GENERIC_READ, 0,0, OPEN_EXISTING, 0,0);
	if (h == INVALID_HANDLE_VALUE) {
		std::cout << "cannot open file  " << GetLastError() << '\n';
		return "";
	}
	auto size = GetFileSize(h, 0);
	str.resize(size);
	if (!ReadFile(h, &str[0], str.size(), 0,0)) {
		std::cout << "cannot read file  " << GetLastError() << '\n';
	}
	CloseHandle(h);
	return str;
}


std::string writeJson(const Json::Value& root) {
	std::string jsonString = Json::FastWriter().write(root);
	return jsonString;
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
	 
	std::string str = loadFile(argv[1]);
	
	benchmark("JsonCPP load",  [&str](){parseJson(str);});
	auto root = parseJson(str);
	benchmark("JsonCPP write", [&root](){writeJson(root);});
	
	printf("\n");
}


