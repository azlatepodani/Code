
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


struct parser_t {
	char * parsed;
};
bool parseJson(parser_t& p, char * first, char * last);


parser_t parseJson(const std::string& odoc) {
	parser_t p = {0};
	std::string doc = odoc;
	if (!parseJson(p, &doc[0], &doc[0]+doc.size())) {
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
	// auto root = parseJson(str);
	// benchmark("asu::Json API write", [&root](){writeJson(root);});
	
	printf("\n");
}


