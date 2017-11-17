
#include <utility>
#include <cstdint>
#include <array>
#include <algorithm>
#include <vector>
#include <random>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "..\..\radix\ska\ska_sort.hpp"




void gen_random_string_array(int n, int min_len, int max_len,
							 std::vector<std::wstring>& vec,
							 std::mt19937& g)
{
	auto x = std::uniform_int_distribution<int>(32,0xD7FF);
	auto y = std::uniform_int_distribution<int>(min_len, max_len);
	
	for (int i=0; i<n; ++i) {
		int len = y(g);
		std::wstring s;
		s.resize(len);
		for (int j=0; j<len; ++j) {
			s[j] = x(g);
		}
		vec.emplace_back(std::move(s));
	}
}


void gen_random_string_array(int n, int min_len, int max_len,
							 std::vector<std::string>& vec,
							 std::mt19937& g)
{
	auto x = std::uniform_int_distribution<int>(32,127);
	auto y = std::uniform_int_distribution<int>(min_len, max_len);
	
	for (int i=0; i<n; ++i) {
		int len = y(g);
		std::string s;
		s.resize(len);
		for (int j=0; j<len; ++j) {
			s[j] = x(g);
		}
		vec.emplace_back(std::move(s));
	}
}


void gen_random_string_array(int n, int min_len, int max_len,
							 std::vector<char *>& vec,
							 std::mt19937& g)
{
	auto x = std::uniform_int_distribution<int>(32,127);
	auto y = std::uniform_int_distribution<int>(min_len, max_len);
	
	for (int i=0; i<n; ++i) {
		int len = y(g);
		char * s = new char[len];
		for (int j=0; j<len; ++j) {
			s[j] = x(g);
		}
		vec.emplace_back(std::move(s));
	}
}

	template <typename U> struct other { typedef U type; };
	template <> struct other<int8_t> { typedef int type; };
	template <> struct other<uint8_t> { typedef int type; };
	
	
template <typename T>
void gen_random_int_array(int n, T min_v, T max_v,
							 std::vector<T>& vec,
							 std::mt19937& g)
{

	auto x = std::uniform_int_distribution<other<T>::type>(min_v, max_v);
	
	for (int i=0; i<n; ++i) {
		vec.emplace_back(x(g));
	}
}



template <typename T, typename Fn>
void benchmark(int size, char * desc, std::vector<T>& vec, std::mt19937& g, Fn alg)
{
	long long time = 0;
	LARGE_INTEGER li, li2, freq;
	
	QueryPerformanceFrequency(&freq);
	 
	std::shuffle(vec.begin(), vec.begin()+size, g);
	std::vector<T> backup(vec.begin(), vec.begin()+size);
	alg(&vec[0], &vec[0]+size);
	
	if (!std::is_sorted(&vec[0], &vec[0]+size)) __debugbreak();
	
	int i=0;
	int maxi = 1500000 / size;
	for (; i<maxi; ++i) {
		vec = std::vector<T>(backup.begin(), backup.begin()+size);
		QueryPerformanceCounter(&li);

		alg(&vec[0], &vec[0]+size);

		QueryPerformanceCounter(&li2);
		
		time += li2.QuadPart - li.QuadPart;
		if (time/freq.QuadPart > 8) break;
	}

	
	time = time * 1000000 / (freq.QuadPart * i);
	printf("%s %d  time=%dus,  time/n=%f, time/nlogn=%f\n", desc, size, (int)time, 
				float(time)/size, float(time/(size*19.931568569324)));
}

template <typename T, typename Fn>
void benchmark(char * desc, std::vector<T>& vec, std::mt19937& g, Fn alg)
{
	benchmark(vec.size(), desc, vec, g, alg);
}


void main() {
	std::vector<uint8_t> vec1;
	std::vector<int8_t> vec2;
	std::vector<uint16_t> vec3;
	std::vector<int16_t> vec4;
	std::vector<std::string> vec5;
	//std::vector<char *> vec;
	std::vector<std::wstring> vec6;
	std::vector<uint32_t> vec7;
	std::vector<int32_t> vec8;

	std::mt19937 g(0xCC6699);
	
	gen_random_int_array<uint8_t>(1500000, 0, 255, vec1, g);
	gen_random_int_array<int8_t>(1500000, -128, 127, vec2, g);
	gen_random_int_array<uint16_t>(1500000, 0, 0xFFFF, vec3, g);
	gen_random_int_array<int16_t>(1500000, 0x8000, 0x7FFF, vec4, g);
	gen_random_string_array(50000, 2, 10240, vec5, g);
	gen_random_string_array(50000, 2, 10240, vec6, g);
	gen_random_int_array<uint32_t>(1500000, 0, 0xFFFFFFFF, vec7, g);
	gen_random_int_array<int32_t>(1500000, 0x80000000, 0x7FFFFFFF, vec8, g);
		 
	 
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	if (GetThreadPriority(GetCurrentThread()) != THREAD_PRIORITY_HIGHEST) printf("Priority set failed\n");
	 
	if (!SetThreadAffinityMask(GetCurrentThread(), 1)) printf("Affinity set failed\n");
	 
	
	benchmark("ss uint8", vec1, g, [](uint8_t* f, uint8_t* l){ska_sort(f,l);});
	benchmark("ss int8", vec2, g, [](int8_t* f, int8_t* l){ska_sort(f,l);});
	benchmark("ss uint16", vec3, g, [](uint16_t* f, uint16_t* l){ska_sort(f,l);});
	benchmark("ss int16", vec4, g, [](int16_t* f, int16_t* l){ska_sort(f,l);});
	benchmark("ss uint32", vec7, g, [](uint32_t* f, uint32_t* l){ska_sort(f,l);});
	benchmark("ss int32", vec8, g, [](int32_t* f, int32_t* l){ska_sort(f,l);});
	//benchmark("ss string", vec5, g, [](std::string* f, std::string* l){ska_sort(f,l);});
	//benchmark("ss wstring", vec6, g, [](std::wstring* f, std::wstring* l){ska_sort(f,l);});//*/
	
	/*benchmark("s uint8", vec2, g, [](uint8_t* f, uint8_t* l){std::sort(f,l);});
	benchmark("s int8", vec2, g, [](int8_t* f, int8_t* l){std::sort(f,l);});
	benchmark("s uint16", vec3, g, [](uint16_t* f, uint16_t* l){std::sort(f,l);});
	benchmark("s int16", vec4, g, [](int16_t* f, int16_t* l){std::sort(f,l);});//*/
	benchmark("s string", vec5, g, [](std::string* f, std::string* l){std::sort(f,l);});
	benchmark("s wstring", vec6, g, [](std::wstring* f, std::wstring* l){std::sort(f,l);});//
	
	
	printf("\n");
}


