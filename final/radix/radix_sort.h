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
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_sort(uint8_t* first, uint8_t* last) {
	radix_pass_basic(first, last, IdentityKey());
}

//
// Convenience wrapper for int8_t
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_sort(int8_t* first, int8_t* last) {
	uint8_t* f=(uint8_t*)first;
	uint8_t* l=(uint8_t*)last;
	
	for (; f!=l; ++f) *f += 128;
	f = (uint8_t*)first;
	
	radix_pass_basic(f, l, IdentityKey());
	
	for (; f!=l; ++f) *f -= 128;
}

//
// Performs radix sort on unsigned 16bit values
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_sort(uint16_t* first, uint16_t* last) {
	counters_t end_pos = radix_pass_basic(first, last, ExtractHighByte());
	
	int32_t pos = 0;
	for (int i=0; i<256; ++i) {
		auto ep = end_pos[i];
		if (end_pos[i]-pos > 1) {
			radix_pass_basic(first+pos, first+ep, ExtractLowByte());
		}

		pos = ep;
	}
}

//
// Performs radix sort on signed 16bit values
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_sort(int16_t* first, int16_t* last) {
	uint16_t* f=(uint16_t*)first;
	uint16_t* l=(uint16_t*)last;
	
	for (; f!=l; ++f) *f += 0x8000;
	
	f = (uint16_t*)first;
	counters_t end_pos = radix_pass_basic(f, l, ExtractHighByte());
	
	int32_t pos = 0;
	for (int i=0; i<256; ++i) {
		if (end_pos[i]-pos > 1) {
			radix_pass_basic(f+pos, f+end_pos[i], ExtractLowByte());
		}

		pos = end_pos[i];
	}
	
	for (; f!=l; ++f) *f -= 0x8000;
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







bool compare(const std::string& l, const std::string& r, int round) {
	return strcmp(&l[round], &r[round]) < 0;
}


bool compare(const std::wstring& l, const std::wstring& r, int round) {
	return wcscmp(&l[round], &r[round]) < 0;
}


template <typename T, typename ExtractKey, typename NextSort>
void recurse_down_r(T first, std::array<std::pair<int32_t, int32_t>, 257>& recurse_table, ExtractKey& ek, NextSort& continuation, vector_key_t) {
	int round = get_key_round(ek);
	for (int i=0; recurse_table[i].second; ++i) {
		while (first[recurse_table[i].first].size() <= unsigned(round+1)) {
				++recurse_table[i].first;
				if (recurse_table[i].first == recurse_table[i].second) goto _after_sort;
			}
		
		auto diff = recurse_table[i].second - recurse_table[i].first;
		if (diff > 50) {		// magic number empirically determined
			continuation(first+recurse_table[i].first, first+recurse_table[i].second);
		}
		else if (diff > 1) {
			std::sort(first+recurse_table[i].first, first+recurse_table[i].second, [round](const auto& l, const auto& r) {
				return compare(l, r, round);
			});
		}
		
		_after_sort:;
	}
}


template <typename T, typename ExtractKey, typename NextSort>
void recurse_down_r(T first, std::array<std::pair<int32_t, int32_t>, 257>& recurse_table, ExtractKey& ek, NextSort& continuation, scalar_key_t) {
	for (int i=0; recurse_table[i].second; ++i) {
		auto diff = recurse_table[i].second - recurse_table[i].first;
		if (diff > 75) {		// magic number empirically determined
			continuation(first+recurse_table[i].first, first+recurse_table[i].second);
		}
		else if (diff > 1) {
			std::sort(first+recurse_table[i].first, first+recurse_table[i].second);
		}
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
	
	recurse_down_r(first, recurse_table, ek, continuation, typename ExtractKey::use_round());
}


using compose_ch = compose<ExtractHighByte, ExtractStringChar>;
using compose_cl = compose<ExtractLowByte,  ExtractStringChar>;

int get_key_round(const compose_ch& ek) {
	return ek.g.offset;
}


int get_key_round(const compose_cl& ek) {
	return ek.g.offset;
}

int get_key_round(const ExtractStringChar& ek) {
	return ek.offset;
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


void radix_string(std::string* first, std::string* last, int round) {
	radix_word_pr(first, last, ExtractStringChar(round),
		[round](std::string* first, std::string* last) {
			radix_string(first, last, round+1);
		});
}
		



void radix_sort(uint32_t* first, uint32_t* last) {
	radix_word_pr(first, last, compose4(ExtractHighByte(), ExtractHighWord()), [](uint32_t* first, uint32_t* last) {
		radix_word_pr(first, last, compose3(ExtractLowByte(), ExtractHighWord()), [](uint32_t* first, uint32_t* last) {
			radix_word_pr(first, last, compose2(ExtractHighByte(), ExtractLowWord()), [](uint32_t* first, uint32_t* last) {
				radix_pass_basic(first, last, compose1(ExtractLowByte(), ExtractLowWord()));
			});
		});
	});
}






//
// TODO:
// - try the hits again
// - reuse counters & chains
// - 
//

} // namespace azp