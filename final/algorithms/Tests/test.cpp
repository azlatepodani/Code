
#include <utility>
#include <cstdint>
#include <array>
#include <algorithm>
#include <vector>
#include <random>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "..\selection.h"

#undef min
#undef max

using namespace azp;


template <typename U> struct other { typedef U type; };
template <> struct other<int8_t> { typedef int type; };
template <> struct other<uint8_t> { typedef int type; };
	
	
template <typename T>
void gen_uniform_int_array(int n, T min_v, T max_v,
							 std::vector<T>& vec,
							 std::mt19937& g)
{

	auto x = std::uniform_int_distribution<other<T>::type>(min_v, max_v);
	
	for (int i=0; i<n; ++i) {
		vec.emplace_back(x(g));
	}
}

template <typename T>
void gen_exponential_int_array(int n, T min_v, T max_v,
							   std::vector<T>& vec,
							   std::mt19937& g)
{

	auto x = std::exponential_distribution<double>(0.5);
	auto min_d = x.min();
	auto max_d = x.max();
	double scale = (double)(max_v - min_v) / (20.0);
	
	for (int i=0; i<n; ++i) {
		double val = (x(g)) * scale + min_v;
		vec.emplace_back(T(val));
	}
}

template <typename T>
void gen_normal_int_array(int n, T min_v, T max_v,
							 std::vector<T>& vec,
							 std::mt19937& g)
{

	auto x = std::normal_distribution<double>(0, 0.2);
	auto min_d = x.min();
	auto max_d = x.max();
	double scale = (double)(max_v - min_v) / (8.0);

	for (int i=0; i<n; ++i) {
		double val = (x(g)) * scale + min_v;
		vec.emplace_back(T(val));
	}
}


template <typename T, typename Fn>
void benchmark(int size, char * desc, std::vector<T>& vec, std::mt19937& g, Fn alg)
{
	long long time = 0;
	LARGE_INTEGER li, li2, freq;
	
	QueryPerformanceFrequency(&freq);
	 
	alg(&vec[0], &vec[0]+size);

	
	int i=0;
	int maxi = 15;//00000 / size;
	for (; i<maxi; ++i) {
		QueryPerformanceCounter(&li);

		alg(&vec[0], &vec[0]+size);

		QueryPerformanceCounter(&li2);
		
		time += li2.QuadPart - li.QuadPart;
		if (time/freq.QuadPart > 8) break;
	}

	
	time = time * 1000000 / (freq.QuadPart * i);
	printf("%s %d  time=%dus\n", desc, size, (int)time);
}

template <typename T, typename Fn>
void benchmark(char * desc, std::vector<T>& vec, std::mt19937& g, Fn alg)
{
	benchmark(vec.size(), desc, vec, g, alg);
}


void main() {
	

	std::mt19937 g(0xCC6699);
	
	 
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	if (GetThreadPriority(GetCurrentThread()) != THREAD_PRIORITY_HIGHEST) printf("Priority set failed\n");
	 
	if (!SetThreadAffinityMask(GetCurrentThread(), 1)) printf("Affinity set failed\n");


	// {
	// std::vector<uint32_t> vec7; 
	// gen_uniform_int_array<uint32_t>(1500000, 0, 0xFFFFFFFF, vec7, g);
	// benchmark("HT 1000", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<150>(f,l); });
	// }{
	// std::vector<uint32_t> vec7; 
	// gen_exponential_int_array<uint32_t>(1500000, 0, 0xFFFFFFFF, vec7, g);
	// benchmark("HT 1000", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<150>(f,l); });
	// }{
	// std::vector<uint32_t> vec7; 
	// gen_normal_int_array<uint32_t>(1500000, 0, 0xFFFFFFFF, vec7, g);
	// benchmark("HT 1000", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<150>(f,l); });
	// }

// return;

	{
	std::vector<uint32_t> vec7; 
	gen_uniform_int_array<uint32_t>(1500000, 0, 0xFFFFFFFF, vec7, g);
	benchmark("TK 5  ", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<5>(f,l); });
	benchmark("HT 5  ", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<5>(f,l); });

	benchmark("TK 10 ", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<10>(f,l); });
	benchmark("HT 10 ", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<10>(f,l); });

	benchmark("TK 20 ", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<20>(f,l); });
	benchmark("HT 20 ", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<20>(f,l); });

	benchmark("TK 40 ", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<40>(f,l); });
	benchmark("HT 40 ", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<40>(f,l); });

	benchmark("TK 80 ", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<80>(f,l); });
	benchmark("HT 80 ", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<80>(f,l); });

	benchmark("TK 160", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<160>(f,l); });
	benchmark("HT 160", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<160>(f,l); });
	}{
	std::vector<uint32_t> vec7; 
	gen_exponential_int_array<uint32_t>(1500000, 0, 0xFFFFFFFF, vec7, g);
	benchmark("TK 5  ", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<5>(f,l); });
	benchmark("HT 5  ", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<5>(f,l); });

	benchmark("TK 10 ", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<10>(f,l); });
	benchmark("HT 10 ", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<10>(f,l); });

	benchmark("TK 20 ", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<20>(f,l); });
	benchmark("HT 20 ", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<20>(f,l); });

	benchmark("TK 40 ", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<40>(f,l); });
	benchmark("HT 40 ", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<40>(f,l); });

	benchmark("TK 80 ", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<80>(f,l); });
	benchmark("HT 80 ", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<80>(f,l); });

	benchmark("TK 160", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<160>(f,l); });
	benchmark("HT 160", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<160>(f,l); });
	}{
	std::vector<uint32_t> vec7; 
	gen_normal_int_array<uint32_t>(1500000, 0, 0xFFFFFFFF, vec7, g);
	benchmark("TK 5  ", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<5>(f,l); });
	benchmark("HT 5  ", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<5>(f,l); });

	benchmark("TK 10 ", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<10>(f,l); });
	benchmark("HT 10 ", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<10>(f,l); });

	benchmark("TK 20 ", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<20>(f,l); });
	benchmark("HT 20 ", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<20>(f,l); });

	benchmark("TK 40 ", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<40>(f,l); });
	benchmark("HT 40 ", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<40>(f,l); });

	benchmark("TK 80 ", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<80>(f,l); });
	benchmark("HT 80 ", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<80>(f,l); });

	benchmark("TK 160", vec7, g, [](uint32_t* f, uint32_t* l){ top_k_elements<160>(f,l); });
	benchmark("HT 160", vec7, g, [](uint32_t* f, uint32_t* l){ heavy_hitters<160>(f,l); });
	}
	
	printf("\n");
}


