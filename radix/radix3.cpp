
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

// template <typename T, typename ExtractKey>
// void update_counts(std::array<int32_t, 256>& count, T first, T last, ExtractKey& ek)
// {
	// for (auto it = first; it != last; ++it) {
		// ++count[ek(*it)];
	// }
// }

// void compute_prefixes(std::array<int32_t, 256>& count, std::array<int32_t, 256>& prefix)
// {
	// int sum = count[0];
	// count[0] = 0;
	
	// for (int i=1; i<256; ++i) {
		// auto val = count[i];
		// prefix[i-1] = sum;
		// count[i] = sum;
		// sum += val;
	// }
	
	// prefix[255] = sum;
// }


// template <typename T, typename ExtractKey>
// void apply_prefixes(T first, std::array<int32_t, 256>& count, const std::array<int32_t, 256>& prefix, ExtractKey& ek)
// {
	// using std::swap;
	// bool sorted = true;
	// do {
		// sorted = true;
		// // may do better
		// for (int pos=0; pos < count.size(); ++pos) {
			// for (auto val = count[pos]; val < prefix[pos]; ++val) {
				// auto left = ek(first[val]);
				// auto right_index = count[left]++;
				// if (right_index != val) {
					// sorted = false;
					// swap(first[val], first[right_index]);
				// }
			// }
		// }
	// } while (!sorted);
// }


// template <typename T, typename ExtractKey>
// void radix_byte(T first, T last, ExtractKey& ek)
// {
	// std::array<int32_t, 256> count = {0};
	
	// update_counts(count, first, last, ek);
	
	// std::array<int32_t, 256> prefix;

	// compute_prefixes(count, prefix);
	
	// apply_prefixes(first, count, prefix, ek);
// }


// void radix_uint8(uint8_t* first, uint8_t* last) {
	// radix_byte(first, last, IdentityKey());
// }


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


// void radix_uint16(uint16_t* first, uint16_t* last) {
	// using std::swap;
	// std::array<int32_t, 256> count = {0};
	
	// update_counts(count, first, last, ExtractHighByte());
	
	// std::array<int32_t, 256> prefix;

	// compute_prefixes(count, prefix);
	
	// apply_prefixes(first, count, prefix, ExtractHighByte());
	
	// int32_t pos = 0;
	// int32_t old_prefix = 0;
	// for (int i=0; i<256; ++i) {
		// if (prefix[i]-old_prefix < 128) {
			// std::sort(first+pos, first+prefix[i]);
		// }
		// else {
			// radix_byte(first+pos, first+prefix[i], ExtractLowByte());
		// }
		// pos += prefix[i] - old_prefix;
		// old_prefix = prefix[i];
	// }
// }


struct partition_t {
	int count;
	//int key_position;
	int next;
};


template <typename T, typename ExtractKey>
void update_counts(std::array<partition_t, 256>& partitions, T first, T last, int, ExtractKey& ek)
{
	for (auto it = first; it != last; ++it) {
		++partitions[ek(*it)].count;
	}
}

int compute_prefixes(std::array<partition_t, 256>& partitions, std::array<int32_t, 256>& prefix)
{
	int sum = partitions[0].count;
	partitions[0].count = 0;
	
	partition_t* prev = &partitions[255];
	partitions[255].next = 256;	// end of chain
	
	for (int i=1; i<256; ++i) {
		auto val = partitions[i].count;
		if (val) {
			prev->next = i;
			prev = &partitions[i];
		}
		
		prefix[i-1] = sum;
		partitions[i].count = sum;
		sum += val;
	}
	
	int chain_start = partitions[255].next;
	prev->next = 256; 		// mark as end of chain
	prefix[255] = sum;
	partitions[255].next = 256; // mark as end of chain; it used to hold the first position in the chain
	
	return chain_start;
}


int reduce_chain(std::array<partition_t, 256>& partitions, int chain_start, const std::array<int32_t, 256>& prefix) {
	int pos = chain_start;
	while (pos != 256) {
		if (partitions[pos].count != prefix[pos]) {
			chain_start = pos;
			pos = partitions[pos].next;
			break;
		}
		
		pos = partitions[pos].next;
	}
	
	int pos2 = chain_start;
	while (pos != 256) {
		if (partitions[pos].count != prefix[pos]) {
			partitions[pos2].next = pos;
			pos2 = pos;
		}
			
		pos = partitions[pos].next;
	}
	
	return chain_start;
}


template <typename T, typename ExtractKey>
void apply_prefixes(T first, std::array<partition_t, 256>& partitions, int chain_start, const std::array<int32_t, 256>& prefix, int, ExtractKey& ek)
{
	using std::swap;
	bool sorted = true;

	do {
		sorted = true;
		
		int pos = chain_start;
		while (pos != 256) {
			int next = partitions[pos].next;
			
			for (auto val = partitions[pos].count; val < prefix[pos]; ++val) {
				auto left = ek(first[val]);
				auto right_index = partitions[left].count++;
				if (right_index != val) {
					sorted = false;
					swap(first[val], first[right_index]);
				}
			}
			
			pos = next;
		}

		if (sorted) break;
		chain_start = reduce_chain(partitions, chain_start, prefix);
	} while (!sorted);
}


template <typename T, typename ExtractKey>
void radix_byte_p(T first, T last, int, ExtractKey& ek)
{
	std::array<partition_t, 256> count = {0};
	
	update_counts(count, first, last, 0, ek);
	
	std::array<int32_t, 256> prefix;

	auto start = compute_prefixes(count, prefix);
	
	apply_prefixes(first, count, start, prefix, 0, ek);
}


std::array<std::pair<int32_t, int32_t>, 257> fill_recurse_table(const std::array<partition_t, 256>& partitions) {
	std::array<std::pair<int32_t, int32_t>, 257> recurse_table;
	
	int32_t start = 0;
	int pos = 0;
	for (int i=0; i<256; ++i) {
		if (partitions[i].count > 1) {
			recurse_table[pos++] = std::make_pair(start, start+partitions[i].count);
		}
		
		start += partitions[i].count;
	}
	
	recurse_table[pos].second = 0;
	
	return recurse_table;
}


// void radix_uint16_p(uint16_t* first, uint16_t* last) {
	// using std::swap;
	// std::array<partition_t, 256> count = {0};
	
	// update_counts(count, first, last, 0, ExtractHighByte());
	
	// auto recurse_table = fill_recurse_table(count);
	// std::array<int32_t, 256> prefix;

	// auto start = compute_prefixes(count, prefix);
	
	// apply_prefixes(first, count, start, prefix, 0, ExtractHighByte());
	
	// for (int i=0; recurse_table[i].second; ++i) {
		// if (recurse_table[i].second - recurse_table[i].first < 128) {
			// std::sort(first+recurse_table[i].first, first+recurse_table[i].second, [](uint16_t left, uint16_t right) {
				// ExtractLowByte ek;
				// return ek(left) < ek(right);
			// });
		// }
		// else {
			// radix_byte_p(first+recurse_table[i].first, first+recurse_table[i].second, 0, ExtractLowByte());
		// }
	// }
// }

void radix_uint16_p(uint16_t* first, uint16_t* last) {
	using std::swap;
	std::array<partition_t, 256> count = {0};
	
	update_counts(count, first, last, 0, ExtractHighByte());
	
	std::array<int32_t, 256> prefix;

	auto start = compute_prefixes(count, prefix);
	
	apply_prefixes(first, count, start, prefix, 0, ExtractHighByte());
	
	int32_t pos = 0;
	int32_t old_prefix = 0;

	for (int i=0; i<256; ++i) {
		/*if (prefix[i]-old_prefix < 128) {
			std::sort(first+pos, first+prefix[i], [](uint16_t left, uint16_t right) {
				ExtractLowByte ek;
				return ek(left) < ek(right);
			});
		}
		else*/ {
			radix_byte_p(first+pos, first+prefix[i], 0, ExtractLowByte());
		}
		
		pos += prefix[i] - old_prefix;
		old_prefix = prefix[i];
	}
}



void radix_string(std::string& first, std::string& last) {
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
	// std::generate_n(std::back_inserter(vec), 1000000, 
	//		[urg=std::uniform_int_distribution<int16_t>(std::numeric_limits<int16_t>::min()), &g]{
	//			return urg(g);
	//		});
	auto x = std::uniform_int_distribution<int>(0,0xFFFF);
	 std::generate_n(std::back_inserter(vec), 1000000, 
			[urg=x, &g]{
				return urg(g);
			});
	 long long time = 0;
	 LARGE_INTEGER li, li2;
	 
	 for (int i=0; i<15; ++i) {
		 std::shuffle(vec.begin(), vec.end(), g);
		 QueryPerformanceCounter(&li);
		 //radix_uint16_p(&vec[0], &vec[0]+vec.size());
		 //radix_byte_p(&vec[0], &vec[0]+vec.size(), 0, IdentityKey());
		 //std::sort(&vec[0], &vec[0]+vec.size());
		 ska_sort(&vec[0], &vec[0]+vec.size());
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


