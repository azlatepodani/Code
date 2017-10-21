
#include <utility>
#include <cstdint>
#include <array>
#include <algorithm>
#include <vector>
#include <random>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//#include "benchmark/benchmark.h"
#include "ska\ska_sort.hpp"


using std::uint8_t;


struct IdentityKey {
	uint8_t operator()(const uint8_t& val) {
		return val;
	}
	
	uint8_t operator()(const int8_t& val) {
		return (uint8_t)val + 128;
	}
};

template <typename T, typename ExtractKey>
void update_counts(std::array<int32_t, 256>& count, T first, T last, ExtractKey& ek)
{
	for (auto it = first; it != last; ++it) {
		++count[ek(*it)];
	}
}

void compute_prefixes(std::array<int32_t, 256>& count, std::array<int32_t, 256>& prefix)
{
	int sum = count[0];
	count[0] = 0;
	
	for (int i=1; i<256; ++i) {
		auto val = count[i];
		prefix[i-1] = sum;
		count[i] = sum;
		sum += val;
	}
	
	prefix[255] = sum;
}


template <typename T, typename ExtractKey>
void apply_prefixes(T first, std::array<int32_t, 256>& count, const std::array<int32_t, 256>& prefix, ExtractKey& ek)
{
	using std::swap;
	bool sorted = true;
	do {
		sorted = true;
		// may do better
		for (int pos=0; pos < count.size(); ++pos) {
			for (auto val = count[pos]; val < prefix[pos]; ++val) {
				auto left = ek(first[val]);
				auto right_index = count[left]++;
				if (right_index != val) {
					sorted = false;
					swap(first[val], first[right_index]);
				}
			}
		}
	} while (!sorted);
}


template <typename T, typename ExtractKey>
void radix_byte(T first, T last, ExtractKey& ek)
{
	std::array<int32_t, 256> count = {0};
	
	update_counts(count, first, last, ek);
	
	std::array<int32_t, 256> prefix;

	compute_prefixes(count, prefix);
	
	apply_prefixes(first, count, prefix, ek);
}


void radix_uint8(uint8_t* first, uint8_t* last) {
	radix_byte(first, last, IdentityKey());
}


struct ExtractHighByte {
	uint8_t operator()(const uint16_t& val) {
		return val>>8;
	}
	
	uint8_t operator()(const int16_t& val) {
		return ((uint16_t)val + 0x8000)>>8;	// can do better
	}
};


struct ExtractLowByte {
	uint8_t operator()(const uint16_t& val) {
		return val&0xFF;
	}
	
	uint8_t operator()(const int16_t& val) {
		return ((uint16_t)val + 0x8000)&0xFF;	// can do better
	}
};


void radix_uint16(uint16_t* first, uint16_t* last) {
	using std::swap;
	std::array<int32_t, 256> count = {0};
	
	update_counts(count, first, last, ExtractHighByte());
	
	std::array<int32_t, 256> prefix;

	compute_prefixes(count, prefix);
	
	apply_prefixes(first, count, prefix, ExtractHighByte());
	
	int32_t pos = 0;
	int32_t old_prefix = 0;
	for (int i=0; i<256; ++i) {
		if (prefix[i]-old_prefix < 128) {
			std::sort(first+pos, first+prefix[i]);
		}
		else {
			radix_byte(first+pos, first+prefix[i], ExtractLowByte());
		}
		pos += prefix[i] - old_prefix;
		old_prefix = prefix[i];
	}
}


void main() {
	//std::vector<uint8_t> vec;
	std::vector<uint16_t> vec;//={1,5,4,0,0};
	//std::vector<int16_t> vec;//={1,5,4,0,0};
	// uint8_t vec[256] = {128, 15, 77, 127, 126, 0, 0, 125, 124, 116,
						// 207, 123, 122, 0, 0, 121, 120, 0, 0, 119,
						// 118, 0, 0, 117, 11,	6, 0, 76, 115, 114,
						// 0, 0, 113, 112, 2, 76, 111, 110, 2, 76,
						// 109, 108, 247, 10, 107, 106, 248, 10, 105, 1,
						// 04, 116, 207, 103, 102, 183, 139, 101, 100, 255,
						// 255, 99, 98, 247, 10, 97, 96, 16, 202, 95,
						// 94, 8, 0, 9, 3, 92, 8, 0, 91, 90,
						// 0, 0, 89, 88, 247, 10, 87, 86, 51, 50,
						// 85, 84, 8, 0, 83, 82, 128, 38, 81, 80, 
						// 0, 0, 79, 78, 159, 76, 77, 76, 0, 0,
						// 75, 74, 0, 0, 73, 72, 1, 115, 71, 70,
						// 148, 202, 69, 68, 8, 0, 67, 66, 247, 10,
						// 6, 5, 64, 29, 202, 63, 62, 0, 0, 61,
						// 60, 213, 76, 59, 58, 0, 0, 57, 56, 213,
						// 76, 55, 54, 128, 38, 53, 52, 0, 0, 51,
						// 50, 158, 76, 49, 48, 0, 0, 47, 46, 0,
						// 0, 45, 44, 81, 75, 43, 42, 86, 51, 41,
						// 40, 247, 10, 39, 38, 1, 1, 15, 37, 36,
						// 200, 55, 35, 34, 15, 0, 33, 32, 0, 0,
						// 31, 30, 247, 10, 29, 28, 125, 53, 27, 26,
						// 200, 55, 25, 2, 4, 15, 0, 23, 22, 0, 
						// 0, 21, 20, 125, 53, 19, 18, 221, 170, 17,
						// 16, 247, 10, 15, 14, 247, 10, 13, 12, 247,
						// 10, 11, 10, 105, 50, 9, 8, 0, 0, 7,
						// 6, 32, 51, 5, 4, 21};//0, 0, 1, 4, 5};//1,5,4,0,0};
	// for (int i=0; i<256; ++i) {
		// vec[uint8_t(i*2+i%2)] = 256-i;
	// }
	// for (int i=0; i<256; ++i) {
		// vec[i] = 255-i;
	// }
	//radix_uint16(&vec[0], &vec[0]+vec.size());
	 //for (auto val : vec) {
		// printf("%d ", (int)val);
	 //}
	// printf("\n");
	// printf("\n");
	
	 std::mt19937 g(0xCC6699);
	#undef min
	 std::generate_n(std::back_inserter(vec), 1000000, 
			[urg=std::uniform_int_distribution<int16_t>(std::numeric_limits<int16_t>::min()), &g]{
				return urg(g);
			});
	 long long time = 0;
	 LARGE_INTEGER li, li2;
	 
	 for (int i=0; i<15; ++i) {
		 std::shuffle(vec.begin(), vec.end(), g);
		 QueryPerformanceCounter(&li);
		 radix_uint16(&vec[0], &vec[0]+vec.size());
		 //std::sort(&vec[0], &vec[0]+vec.size());
		 //ska_sort(&vec[0], &vec[0]+vec.size());
		 QueryPerformanceCounter(&li2); time += li2.QuadPart - li.QuadPart;
	 }

	 QueryPerformanceFrequency(&li);
	 time = time * 1000000 / li.QuadPart;
	 printf("time=%dus,  time/n=%f, time/nlogn=%f\n", (int)time, time/1000000.f, float(time/19931568.569324));
	
	auto old = vec.front();
	for (auto val : vec) {
		if (old > val) printf("%d > %d\t", (int)old, (int)val);
		old = val;
	}
	printf("\n");
}


