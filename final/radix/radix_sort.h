#pragma once

#include <utility>
#include <cstdint>
#include <array>
#include <algorithm>
// #define WIN32_LEAN_AND_MEAN
// #include <windows.h>

#include "radix_utils.h"


//
// Concepts:
//  1. RandomIt, ExtractKey are defined in radix_utils.h
//  2.
//
//


namespace azp {
	
	
constexpr int Std_sort_threshold = 128;


//
// Performs radix sort on the range using the basic algorithm.
// This is suited for small types like 8 and 16 bit arrays.
// Returns the resulting partitions array.
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
template <typename RandomIt, typename ExtractKey>
counters_t radix_pass_basic(RandomIt first, RandomIt last, ExtractKey&& ek)
{
	counters_t count = {0};
	
	compute_counts(count, first, last, ek);
	
	counters_t end_pos;

	compute_ranges(count, end_pos);
	
	swap_elements_into_place(first, count, end_pos, ek);
	
	return end_pos;
}

//
// Convenience wrapper for uint8_t
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_sort(uint8_t* first, uint8_t* last) {
	radix_pass_basic(first, last, IdentityKey());
}

//
// Convenience wrapper for int8_t
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_sort(int8_t* first, int8_t* last) {
	uint8_t* f=(uint8_t*)first;
	uint8_t* l=(uint8_t*)last;
	for (auto p=f; p!=l; ++p) *p+=128;
	radix_pass_basic(f, l, IdentityKey());
	for (auto p=f; p!=l; ++p) *p-=128;
}



inline void radix_sort(uint16_t* first, uint16_t* last) {
	counters_t end_pos = radix_pass_basic(first, last, ExtractHighByte());
	
	int32_t pos = 0;
	for (int i=0; i<256; ++i) {
		if (end_pos[i]-pos < 2) {//Std_sort_threshold) { // it makes no difference here
			//std::sort(first+pos, first+end_pos[i]);
		}
		else {
			radix_pass_basic(first+pos, first+end_pos[i], ExtractLowByte());
		}

		pos = end_pos[i];
	}
}


inline void radix_sort(int16_t* first, int16_t* last) {
	uint16_t* f=(uint16_t*)first;
	uint16_t* l=(uint16_t*)last;
	for (auto p=f; p!=l; ++p) *p+=0x8000;
	
	counters_t end_pos = radix_pass_basic(f, l, ExtractHighByte());
	
	int32_t pos = 0;
	for (int i=0; i<256; ++i) {
		if (end_pos[i]-pos < 2) {//Std_sort_threshold) {
			//std::sort(first+pos, first+end_pos[i]);
		}
		else {
			radix_pass_basic(f+pos, f+end_pos[i], ExtractLowByte());
		}

		pos = end_pos[i];
	}
	for (auto p=f; p!=l; ++p) *p-=0x8000;
}





template <typename T, typename ExtractKey>
void radix_byte_p(T first, T last, ExtractKey& ek)
{
	counters_t count = {0};
	
	compute_counts(count, first, last, ek);
	
	counters_t chain;
	counters_t end_pos;

	compute_ranges(count, end_pos, chain);
	
	swap_elements_into_place(first, count, end_pos, chain, ek);
}


std::array<std::pair<int32_t, int32_t>, 257> fill_recurse_table(const counters_t& count) {
	std::array<std::pair<int32_t, int32_t>, 257> recurse_table;
	
	int32_t start = 0;
	int pos = 0;
	for (int i=0; i<256; ++i) {
		if (count[i] > 1) {
			recurse_table[pos++] = std::make_pair(start, start+count[i]);
		}
		
		start += count[i];
	}
	
	recurse_table[pos].second = 0;
	
	return recurse_table;
}


// void radix_uint16_p2(uint16_t* first, uint16_t* last) {
	// std::array<partition_t, 256> count = {0};
	
	// compute_counts(count, first, last, ExtractHighByte());
	
	// auto recurse_table = fill_recurse_table(count);
	// counters_t end_pos;

	// auto start = compute_ranges(count, end_pos);
	
	// swap_elements_into_place(first, count, start, end_pos, ExtractHighByte());
	
	// for (int i=0; recurse_table[i].second; ++i) {
		// radix_byte_p(first+recurse_table[i].first, first+recurse_table[i].second, ExtractLowByte());
	// }
// }

void radix_uint16_p(uint16_t* first, uint16_t* last) {
	counters_t count = {0};
	
	compute_counts(count, first, last, ExtractHighByte());

	counters_t chain;	
	counters_t end_pos;

	compute_ranges(count, end_pos, chain);
	
	swap_elements_into_place(first, count, end_pos, chain, ExtractHighByte());
	
	int32_t pos = 0;

	for (int i=0; i<256; ++i) {
		if (end_pos[i]-pos > 1) {
			radix_byte_p(first+pos, first+end_pos[i], ExtractLowByte());
		}
		
		pos = end_pos[i];
	}
}


template <typename T, typename NextSort>
void recurse_down(T first, const counters_t& end_pos, NextSort& continuation) {
	int32_t pos = 0;

	for (int i=0; i<256; ++i) {
		if (end_pos[i]-pos > 1) {
			continuation(first+pos, first+end_pos[i]);
		}
		
		pos = end_pos[i];
	}
}


template <typename T>
struct NoContinuation {
	void operator()(T, T) {}
};


template <typename T, typename ExtractKey, typename NextSort>
void radix_word_p(T first, T last, ExtractKey& ek, NextSort& continuation) {
	counters_t count = {0};
	
	compute_counts(count, first, last, ek);
	
	counters_t chain;
	counters_t end_pos;

	compute_ranges(count, end_pos, chain);
	
	swap_elements_into_place(first, count, end_pos, chain, ek);
	
	recurse_down(first, end_pos, continuation);
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
	
	// compute_counts(count, first, last, ExtractStringChar(round));
	
	// counters_t end_pos;

	// auto start = compute_ranges(count, end_pos);
	
	// swap_elements_into_place(first, count, start, end_pos, ExtractStringChar(round));
	
	// int32_t pos = 0;
	// //int32_t old_prefix = 0;

	// for (int i=0; i<256; ++i) {
		// while (first[pos].size() <= round+1) {
				// ++pos; if (pos == end_pos[i]) goto _after_sort;
			// }
			
		// /*if (end_pos[i]-pos < 128) {
			// //ExtractStringChar ek(round+1);
			// std::sort(first+pos, first+end_pos[i], [r = round+1](const std::string& left, const std::string& right) {
				// //ExtractLowByte ek;
				// return left.length() < right.length() || strcmp(&left[r], &right[r]) < 0;
			// });
		// }
		// else */if (end_pos[i]-pos > 1) {
			
			// radix_string(first+pos, first+end_pos[i], round+1);
		// }
		
		// pos = end_pos[i];
		// _after_sort:;
		// //old_prefix = end_pos[i];
	// }
// }



void radix_string(std::string* first, std::string* last, int round) {
	counters_t count = {0};
	
	compute_counts(count, first, last, ExtractStringChar(round));
	
	counters_t chain;
	counters_t end_pos;

	auto recurse_table = fill_recurse_table(count);
	compute_ranges(count, end_pos, chain);
	
	swap_elements_into_place(first, count, end_pos, chain, ExtractStringChar(round));
	
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
		while (first[recurse_table[i].first].size() <= unsigned(round+1)) {
				++recurse_table[i].first;
				if (recurse_table[i].first == recurse_table[i].second) goto _after_sort;
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
	counters_t count = {0};
	
	compute_counts(count, first, last, ek);
	
	counters_t chain;
	counters_t end_pos;

	auto recurse_table = fill_recurse_table(count);
	compute_ranges(count, end_pos, chain);
	
	swap_elements_into_place(first, count, end_pos, chain, ek);
	
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
	
	// compute_counts(count, first, last, ExtractStringChar(round));
	
	// counters_t end_pos;

	// auto recurse_table = fill_recurse_table(count);
	// auto start = compute_ranges(count, end_pos);
	
	// swap_elements_into_place(first, count, start, end_pos, ExtractStringChar(round));
	
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












} // namespace azp