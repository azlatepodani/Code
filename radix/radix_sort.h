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
//  1. RandomIt, ExtractKey, NextSort are defined in radix_utils.h
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
partitions_t radix_pass_basic(RandomIt first, RandomIt last, ExtractKey&& ek) NEX
{
	partitions_t partitions = compute_counts(first, last, ek);

	counters_t valid_part;
	
	auto vp_size = compute_ranges(partitions, valid_part);
	
	swap_elements_into_place(first, partitions, valid_part, vp_size, ek);
	
	return partitions;
}

//
// Convenience wrapper for uint8_t
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_sort(uint8_t* first, uint8_t* last) NEX {
	(void)radix_pass_basic(first, last, IdentityKey());
}

//
// Convenience wrapper for int8_t
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_sort(int8_t* first, int8_t* last) NEX {
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
inline void radix_sort(uint16_t* first, uint16_t* last) NEX {
	partitions_t partitions = radix_pass_basic(first, last, ExtractHighByte());
	
	int32_t pos = 0;
	for (int i=256; i; --i) {
		auto ep = partitions[256-i].next_offset;
		auto diff = ep-pos;
		if (diff > 150) {
			radix_pass_basic(first+pos, first+ep, ExtractLowByte());
		}
		else if (diff > 1) {
			std::sort(first+pos, first+ep);
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
inline void radix_sort(int16_t* first, int16_t* last) NEX {
	uint16_t* f=(uint16_t*)first;
	uint16_t* l=(uint16_t*)last;
	
	for (; f!=l; ++f) *f += 0x8000;
	
	f = (uint16_t*)first;
	radix_sort(f, l);
	
	for (; f!=l; ++f) *f -= 0x8000;
}

//
// Performs radix sort on the range using the partitions based algorithm.
// This is suited for larger more complex types. The 'continuation' functor will
// be called for each partition in order to enable sorting on multiple bytes
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
template <typename RandomIt, typename ExtractKey, typename NextSort>
void radix_pass_recurse(RandomIt first, RandomIt last, ExtractKey&& ek,
						NextSort&& continuation) NEX
{
	partitions_t partitions = compute_counts(first, last, ek);
	
	counters_t valid_part;

	auto vp_size = compute_ranges(partitions, valid_part);
	
	swap_elements_into_place(first, partitions, valid_part, vp_size, ek);
	
	recurse_depth_first(first, partitions, ek, continuation, typename ExtractKey::use_round());
}

//
// Performs radix sort on unsigned 32bit values
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_sort(uint32_t* first, uint32_t* last) NEX
{
	using compose4 = compose<ExtractHighByte, ExtractHighWord>;
	using compose3 = compose<ExtractLowByte,  ExtractHighWord>;
	using compose2 = compose<ExtractHighByte, ExtractLowWord>;
	using compose1 = compose<ExtractLowByte,  ExtractLowWord>;

	radix_pass_recurse(first, last, compose4(ExtractHighByte(), ExtractHighWord()), [](uint32_t* first, uint32_t* last) {
		radix_pass_recurse(first, last, compose3(ExtractLowByte(), ExtractHighWord()), [](uint32_t* first, uint32_t* last) {
			radix_pass_recurse(first, last, compose2(ExtractHighByte(), ExtractLowWord()), [](uint32_t* first, uint32_t* last) {
				radix_pass_basic(first, last, compose1(ExtractLowByte(), ExtractLowWord()));
			});
		});
	});
}

//
// Performs radix sort on signed 32bit values
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_sort(int32_t* first, int32_t* last) NEX
{
	uint32_t* f = (uint32_t*)first;
	uint32_t* l = (uint32_t*)last;
	
	for (; f!=l; ++f) *f += 0x80000000;
	
	f = (uint32_t*)first;
	radix_sort(f,l);
	
	for (; f!=l; ++f) *f -= 0x80000000;
}

//
// Utility function for std::string values
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_string(std::string* first, std::string* last, int round) NEX {
	radix_pass_recurse(first, last, ExtractStringChar<std::string>(round),
		[round](std::string* first, std::string* last) {
			radix_string(first, last, round+1);
		});
}

//
// Performs radix sort on std::string values
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_sort(std::string* first, std::string* last) NEX {
	radix_string(first, last, 0);
}

//
// Utility function for char* values
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_string(char** first, char** last, int round) NEX {
	radix_pass_recurse(first, last, ExtractStringChar<char*>(round),
		[round](char** first, char** last) {
			radix_string(first, last, round+1);
		});
}

//
// Performs radix sort on char* values
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_sort(char** first, char** last) NEX {
	radix_string(first, last, 0);
}

//
// Utility function for std::wstring values
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_string(std::wstring* first, std::wstring* last, int round) NEX
{
	radix_pass_recurse(first, last, compose_ch<std::wstring>(ExtractHighByte(),
		ExtractStringChar<std::wstring>(round)),
		[round](std::wstring* first, std::wstring* last)
		{
			radix_pass_recurse(first, last, compose_cl<std::wstring>(ExtractLowByte(),
				ExtractStringChar<std::wstring>(round)),
				[round](std::wstring* first, std::wstring* last)
				{
					radix_string(first, last, round+1);
				});
		});
}

//
// Performs radix sort on std::wstring values
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_sort(std::wstring* first, std::wstring* last) NEX {
	radix_string(first, last, 0);
}

//
// Utility function for wchar_t* values
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_string(wchar_t** first, wchar_t** last, int round) NEX
{
	radix_pass_recurse(first, last, compose_ch<wchar_t*>(ExtractHighByte(),
		ExtractStringChar<wchar_t*>(round)),
		[round](wchar_t** first, wchar_t** last)
		{
			radix_pass_recurse(first, last, compose_cl<wchar_t*>(ExtractLowByte(),
				ExtractStringChar<wchar_t*>(round)),
				[round](wchar_t** first, wchar_t** last)
				{
					radix_string(first, last, round+1);
				});
		});
}

//
// Performs radix sort on wchar_t* values
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//
inline void radix_sort(wchar_t** first, wchar_t** last) NEX {
	radix_string(first, last, 0);
}





//
// TODO:
// - try the hits again
// - reuse counters & chains
// - small ranges
// - less random data opt
//

} // namespace azp