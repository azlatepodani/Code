
#include <utility>
#include <cstdint>
#include <array>
#include <algorithm>
#include <vector>
#include <random>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
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
	int next;
};


template <typename T, typename ExtractKey>
void update_counts(std::array<partition_t, 256>& partitions, T first, T last, ExtractKey& ek)
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
void apply_prefixes(T first, std::array<partition_t, 256>& partitions, int chain_start, const std::array<int32_t, 256>& prefix, ExtractKey& ek)
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
void radix_byte_p(T first, T last, ExtractKey& ek)
{
	std::array<partition_t, 256> count = {0};
	
	update_counts(count, first, last, ek);
	
	std::array<int32_t, 256> prefix;

	auto start = compute_prefixes(count, prefix);
	
	apply_prefixes(first, count, start, prefix, ek);
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


// void radix_uint16_p2(uint16_t* first, uint16_t* last) {
	// std::array<partition_t, 256> count = {0};
	
	// update_counts(count, first, last, ExtractHighByte());
	
	// auto recurse_table = fill_recurse_table(count);
	// std::array<int32_t, 256> prefix;

	// auto start = compute_prefixes(count, prefix);
	
	// apply_prefixes(first, count, start, prefix, ExtractHighByte());
	
	// for (int i=0; recurse_table[i].second; ++i) {
		// radix_byte_p(first+recurse_table[i].first, first+recurse_table[i].second, ExtractLowByte());
	// }
// }

void radix_uint16_p(uint16_t* first, uint16_t* last) {
	std::array<partition_t, 256> count = {0};
	
	update_counts(count, first, last, ExtractHighByte());
	
	std::array<int32_t, 256> prefix;

	auto start = compute_prefixes(count, prefix);
	
	apply_prefixes(first, count, start, prefix, ExtractHighByte());
	
	int32_t pos = 0;

	for (int i=0; i<256; ++i) {
		if (prefix[i]-pos > 1) {
			radix_byte_p(first+pos, first+prefix[i], ExtractLowByte());
		}
		
		pos = prefix[i];
	}
}


template <typename T, typename NextSort>
void recurse_down(T first, const std::array<int32_t, 256>& prefix, NextSort& continuation) {
	int32_t pos = 0;

	for (int i=0; i<256; ++i) {
		if (prefix[i]-pos > 1) {
			continuation(first+pos, first+prefix[i]);
		}
		
		pos = prefix[i];
	}
}


template <typename T>
struct NoContinuation {
	void operator()(T, T) {}
};


template <typename T, typename ExtractKey, typename NextSort>
void radix_word_p(T first, T last, ExtractKey& ek, NextSort& continuation) {
	std::array<partition_t, 256> count = {0};
	
	update_counts(count, first, last, ek);
	
	std::array<int32_t, 256> prefix;

	auto start = compute_prefixes(count, prefix);
	
	apply_prefixes(first, count, start, prefix, ek);
	
	recurse_down(first, prefix, continuation);
}


struct ExtractHighWord {
	typedef uint32_t value_type;
	
	uint16_t operator()(const uint32_t& val) {
		return val >> 16;
	}
};


struct ExtractLowWord {
	typedef uint32_t value_type;
	
	uint16_t operator()(const uint32_t& val) {
		return val & 0xFFFF;
	}
};


template <typename T, typename U>
struct compose {
	compose(T& f, U& g) : f(f), g(g) {}

	uint8_t operator()(const typename U::value_type& val) { return f(g(val)); }
	
	T& f;
	U& g;
};

using compose4 = compose<ExtractHighByte, ExtractHighWord>;
using compose3 = compose<ExtractLowByte,  ExtractHighWord>;
using compose2 = compose<ExtractHighByte, ExtractLowWord>;
using compose1 = compose<ExtractLowByte,  ExtractLowWord>;


void radix_uint32_p(uint32_t* first, uint32_t* last) {
	radix_word_p(first, last, compose4(ExtractHighByte(), ExtractHighWord()), [](uint32_t* first, uint32_t* last) {
		radix_word_p(first, last, compose3(ExtractLowByte(), ExtractHighWord()), [](uint32_t* first, uint32_t* last) {
			radix_word_p(first, last, compose2(ExtractHighByte(), ExtractLowWord()), [](uint32_t* first, uint32_t* last) {
				radix_word_p(first, last, compose1(ExtractLowByte(), ExtractLowWord()), NoContinuation<uint32_t*>());
			});
		});
	});
}




struct ExtractStringChar {
	typedef std::wstring value_type;
	
	explicit ExtractStringChar(int offset) : offset(offset) {}
	uint8_t operator()(const std::string& str) {
		return str[offset];
	}
	
	uint16_t operator()(const std::wstring& str) {
		return str[offset];
	}
	
	int offset;
};





// void radix_string(std::string* first, std::string* last, int round) {
	// std::array<partition_t, 256> count = {0};
	
	// update_counts(count, first, last, ExtractStringChar(round));
	
	// std::array<int32_t, 256> prefix;

	// auto start = compute_prefixes(count, prefix);
	
	// apply_prefixes(first, count, start, prefix, ExtractStringChar(round));
	
	// int32_t pos = 0;
	// //int32_t old_prefix = 0;

	// for (int i=0; i<256; ++i) {
		// while (first[pos].size() <= round+1) {
				// ++pos; if (pos == prefix[i]) goto _after_sort;
			// }
			
		// /*if (prefix[i]-pos < 128) {
			// //ExtractStringChar ek(round+1);
			// std::sort(first+pos, first+prefix[i], [r = round+1](const std::string& left, const std::string& right) {
				// //ExtractLowByte ek;
				// return left.length() < right.length() || strcmp(&left[r], &right[r]) < 0;
			// });
		// }
		// else */if (prefix[i]-pos > 1) {
			
			// radix_string(first+pos, first+prefix[i], round+1);
		// }
		
		// pos = prefix[i];
		// _after_sort:;
		// //old_prefix = prefix[i];
	// }
// }



void radix_string(std::string* first, std::string* last, int round) {
	std::array<partition_t, 256> count = {0};
	
	update_counts(count, first, last, ExtractStringChar(round));
	
	std::array<int32_t, 256> prefix;

	auto recurse_table = fill_recurse_table(count);
	auto start = compute_prefixes(count, prefix);
	
	apply_prefixes(first, count, start, prefix, ExtractStringChar(round));
	
	for (int i=0; recurse_table[i].second; ++i) {
		while (first[recurse_table[i].first].size() <= round+1) {
				++recurse_table[i].first; if (recurse_table[i].first == recurse_table[i].second) goto _after_sort;
			}
			
		auto diff = recurse_table[i].second - recurse_table[i].first;
		
		//if (diff <= 1) { }
		//else
		if (diff > 128) {
			radix_string(first+recurse_table[i].first, first+recurse_table[i].second, round+1);
		}
		else {
			std::sort(first+recurse_table[i].first, first+recurse_table[i].second);
		}
		
		_after_sort:;
	}
}


template <typename T, typename ExtractKey, typename NextSort>
void recurse_down_r(T first, std::array<std::pair<int32_t, int32_t>, 257>& recurse_table, ExtractKey& ek, NextSort& continuation) {
	int round = get_key_round(ek);
	for (int i=0; recurse_table[i].second; ++i) {
		while (first[recurse_table[i].first].size() <= round+1) {
				++recurse_table[i].first; if (recurse_table[i].first == recurse_table[i].second) goto _after_sort;
			}
			
		auto diff = recurse_table[i].second - recurse_table[i].first;
		
		if (diff > 128) {
			continuation(first+recurse_table[i].first, first+recurse_table[i].second);
		}
		else {
			std::sort(first+recurse_table[i].first, first+recurse_table[i].second);
		}
		
		_after_sort:;
	}
}


template <typename T, typename ExtractKey, typename NextSort>
void radix_word_pr(T first, T last, ExtractKey& ek, NextSort& continuation) {
	std::array<partition_t, 256> count = {0};
	
	update_counts(count, first, last, ek);
	
	std::array<int32_t, 256> prefix;

	auto recurse_table = fill_recurse_table(count);
	auto start = compute_prefixes(count, prefix);
	
	apply_prefixes(first, count, start, prefix, ek);
	
	recurse_down_r(first, recurse_table, ek, continuation);
}


using compose_ch = compose<ExtractHighByte, ExtractStringChar>;
using compose_cl = compose<ExtractLowByte,  ExtractStringChar>;

int get_key_round(const compose_ch& ek) {
	return ek.g.offset;
}


int get_key_round(const compose_cl& ek) {
	return ek.g.offset;
}

void radix_string(std::wstring* first, std::wstring* last, int round) {
	radix_word_pr(first, last, compose_ch(ExtractHighByte(), ExtractStringChar(round)),
		[round](std::wstring* first, std::wstring* last) {
			radix_word_pr(first, last, compose_cl(ExtractLowByte(), ExtractStringChar(round)),
				[round](std::wstring* first, std::wstring* last) {
					radix_string(first, last, round+1);
				});
		});
}
		
	
	
	
	
	// std::array<partition_t, 256> count = {0};
	
	// update_counts(count, first, last, ExtractStringChar(round));
	
	// std::array<int32_t, 256> prefix;

	// auto recurse_table = fill_recurse_table(count);
	// auto start = compute_prefixes(count, prefix);
	
	// apply_prefixes(first, count, start, prefix, ExtractStringChar(round));
	
	// for (int i=0; recurse_table[i].second; ++i) {
		// while (first[recurse_table[i].first].size() <= round+1) {
				// ++recurse_table[i].first; if (recurse_table[i].first == recurse_table[i].second) goto _after_sort;
			// }
			
		// auto diff = recurse_table[i].second - recurse_table[i].first;
		
		// //if (diff <= 1) { }
		// //else
		// if (diff > 128) {
			// radix_string(first+recurse_table[i].first, first+recurse_table[i].second, round+1);
		// }
		// else {
			// std::sort(first+recurse_table[i].first, first+recurse_table[i].second);
		// }
		
		// _after_sort:;
	// }
//}



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



void main() {
	//std::vector<uint8_t> vec;
	//std::vector<uint16_t> vec;//={1,5,4,0,0};
	//std::vector<uint32_t> vec;//={1,5,4,0,0};
	//std::vector<int16_t> vec;//={1,5,4,0,0};
	// uint8_t vec[256] = {128, 15, 77, 127, 126, 0, 125, 124, 116,
						// 207, 123, 122, 0, 121, 120, 0, 119,
						// 118, 0, 117, 11,	6, 76, 115, 114,
						// 0, 113, 112, 2, 76, 111, 110, 2, 76,
						// 109, 108, 247, 10, 107, 106, 248, 10, 105, 1,
						// 04, 116, 207, 103, 102, 183, 139, 101, 100, 255,
						// 255, 99, 98, 247, 10, 97, 96, 16, 202, 95,
						// 94, 8, 9, 3, 92, 8, 91, 90,
						// 0, 89, 88, 247, 10, 87, 86, 51, 50,
						// 85, 84, 8, 83, 82, 128, 38, 81, 80, 
						// 0, 79, 78, 159, 76, 77, 76, 0,
						// 75, 74, 0, 73, 72, 1, 115, 71, 70,
						// 148, 202, 69, 68, 8, 67, 66, 247, 10,
						// 6, 5, 64, 29, 202, 63, 62, 0, 61,
						// 60, 213, 76, 59, 58, 0, 57, 56, 213,
						// 76, 55, 54, 128, 38, 53, 52, 0, 51,
						// 50, 158, 76, 49, 48, 0, 47, 46,
						// 0, 45, 44, 81, 75, 43, 42, 86, 51, 41,
						// 40, 247, 10, 39, 38, 1, 1, 15, 37, 36,
						// 200, 55, 35, 34, 15, 33, 32, 0,
						// 31, 30, 247, 10, 29, 28, 125, 53, 27, 26,
						// 200, 55, 25, 2, 4, 15, 23, 22, 
						// 0, 21, 20, 125, 53, 19, 18, 221, 170, 17,
						// 16, 247, 10, 15, 14, 247, 10, 13, 12, 247,
						// 10, 11, 10, 105, 50, 9, 8, 0, 7,
						// 6, 32, 51, 5, 4, 21};//0, 1, 4, 5};//1,5,4,0,0};
						
	//std::vector<std::string> vec;// = {"kal","jasds","oeael","cnalsda","adaadasd","adada","ppkod"};
	//std::vector<char *> vec;
	
	std::vector<std::wstring> vec;
						
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
	
	gen_random_string_array(50000, 2, 10240, vec, g);
	// std::generate_n(std::back_inserter(vec), 1000000, 
			// [urg=std::uniform_int_distribution<int16_t>(std::numeric_limits<int16_t>::min()), &g]{
				// return urg(g);
			// });
	// auto x = std::uniform_int_distribution<int>(0,0xFFFF);
	// std::generate_n(std::back_inserter(vec), 1000000, 
			// [urg=x, &g]{
				// return urg(g);
			// });
	 long long time = 0;
	 LARGE_INTEGER li, li2;
	 
	 for (int i=0; i<1; ++i) {
		 std::shuffle(vec.begin(), vec.end(), g);
		 QueryPerformanceCounter(&li);
		 //radix_uint16_p(&vec[0], &vec[0]+vec.size());
		 //radix_uint32_p(&vec[0], &vec[0]+vec.size());
		 //radix_byte_p(&vec[0], &vec[0]+vec.size(), IdentityKey());
		 radix_string(&vec[0], &vec[0]+vec.size(), 0);
		 //std::sort(&vec[0], &vec[0]+vec.size());
		 //ska_sort(&vec[0], &vec[0]+vec.size());
		 QueryPerformanceCounter(&li2); time += li2.QuadPart - li.QuadPart;
	 }

	 QueryPerformanceFrequency(&li);
	 time = time * 1000000 / li.QuadPart;
	 printf("time=%dus,  time/n=%f, time/nlogn=%f\n", (int)time, time/1000000.f, float(time/19931568.569324));
	
	auto old = vec.front();
	for (auto val : vec) {
		//if (old > val) printf("%d > %d\t", (int)old, (int)val);
		//if (old > val) printf("%s >>>>> %s\n\n", old.c_str(), val.c_str());
		if (old > val) wprintf(L"%s >>>>> %s\n\n", old.c_str(), val.c_str());
		//if (strcmp(old, val) > 0) printf("%s >>>>> %s\n\n", old, val);
		//printf("%s\n", val.c_str());
		old = val;
	}
	printf("\n");
}


