#pragma once
#include <cstdint>
#include <array>
#include <utility>
#include <algorithm>
#include <string.h>


//
// Concepts:
//  1. RandomIt == random iterator; it has the same meaning as the STL's RandomAccessIterator
//
//  2. ExtractKey is a callable entity with the following signature:
//   	  uint8_t (const RandomIt::value_type&)
//     It returns a byte from the current element which will be used as the sorting key
//
//  3. NextSort is a callable entity with the following signature:
//        template <typename RandomIt>
//		  void (RandomIt first, RandomIt last)
//     This function will be called to sort the elements within a radix partition.
//


namespace azp {


using part_indeces_t = std::array<uint8_t, 256>;

using scalar_key_t = bool;
using vector_key_t = int32_t;

//
// The following types implement the ExtractKey concept's requirements
//
	
//	
// Helper functor for the simplest case - identity mapping
// The signed version will translate the value for correct sorting order
//
struct IdentityKey {
	// Type traits
	typedef scalar_key_t use_round;
	
	uint8_t operator()(uint8_t val) const {
		return val;
	}
	
	uint8_t operator()(int8_t val) const {
		return (uint8_t)val + 128;
	}
};


template <typename T, int32_t off>
struct ExtractByOffset {
	// Type traits
	typedef T value_type;
	typedef scalar_key_t use_round;
	
	uint8_t operator()(const T& val) const {
		return ((const uint8_t*)&val)[off];
	}
};


//
// Functor for string values, returns a (w)char at a given position
//
template <typename String>
struct ExtractStringChar {
	// Type traits
	typedef String value_type;
	typedef vector_key_t use_round;
	
	explicit ExtractStringChar(int32_t offset) 
		: offset(offset)
	{ }
	
	uint8_t	operator()(const String& str) const {
		return str[offset];
	}

	const int32_t offset;
};


template <typename String, bool high>
struct ExtractWStringChar {
	// Type traits
	typedef String value_type;
	typedef vector_key_t use_round;
	
	explicit ExtractWStringChar(int32_t offset) 
		: offset(offset)
	{ }
	
	uint16_t operator()(const String& str) const {
		uint16_t ch = str[offset];
		if (high) return ch >> 8;
		return ch & 0xFF;
	}

	const int32_t offset;
};


struct partitions_t {
	partitions_t()  { }
	
	union {
		int32_t count[256] = {0};
		int32_t offset[256];
	};
	int32_t next_offset[256];
};


//
// Counts the number of times each distinct 8bit key appears in the data.
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//  2. The output parameter 'count' was 0-initialized
//
template <typename RandomIt, typename ExtractKey>
partitions_t compute_counts(RandomIt first, RandomIt last, ExtractKey&& ek) 
{
	partitions_t partitions;
	
	for (uint32_t len=uint32_t(last-first); len; --len,++first) {
		++partitions.count[ek(*first)];
	}
	
	return partitions;
}


//
// Converts the 'count' field in-place to corresponding start & end positions in the 'partitions' array.
// Creates the vector of indeces to non-empty partitions 'valid_part' and returns its size
//
// Preconditions:
//  1. 'partitions' was initialized by compute_counts()
//
inline int32_t compute_ranges(partitions_t& partitions, part_indeces_t& valid_part) 
{
	int32_t sum = partitions.count[0];
	partitions.offset[0] = 0;
	
	int32_t vp_size = sum ? 1 : 0;
	valid_part[0] = 0;
	
	for (int32_t i=1; i<256; ++i) {
		auto count = partitions.count[i];

		partitions.next_offset[i-1] = sum;
		partitions.offset[i] = sum;
		
		valid_part[vp_size] = uint8_t(i);
		vp_size += count ? 1 : 0;
		sum += count;
	}
	
	partitions.next_offset[255] = sum;
	
	return vp_size;
}


//
// Sorts in linear time the data in the buffer pointed to by 'first' by using the info from 'partitions'.
// The function uses the unqualified call to swap() to allow for client customisation.
// The 'offset' field in the 'partitions' array will be modified by the function.
//
// Preconditions:
//  1. 'partitions', 'valid_part' and 'vp_size' are the result of compute_ranges()
//  2. The buffer pointed to by 'first' is identical to the one that generated the ranges
//
template <typename RandomIt, typename ExtractKey>
void swap_elements_into_place(RandomIt first, partitions_t& partitions, part_indeces_t& valid_part,
							  int32_t vp_size, ExtractKey&& ek) 
{
	using std::swap;
	
	//
	// We iterate through the non-empty partitions and pick elements from the buffer that will
	// be swapped into place. A partition is empty if the offset and next_offset fields are equal.
	// The empty partitions are eliminated from 'valid_part' array each iteration.
	//
	if (vp_size < 2) return;
	
	do {
		bool sorted = true;
		
		int32_t new_i = -1;
		
		for (int32_t i=0; i<vp_size; ++i)
		{
			auto val_off = partitions.offset[valid_part[i]];
			auto val_next = partitions.next_offset[valid_part[i]];
			
			if (val_off < val_next) {
				do {
					auto left = ek(first[val_off]);
					auto right_index = partitions.offset[left]++;

					sorted = false;
					swap(first[val_off], first[right_index]);
				}
				while (++val_off != val_next);

				if (new_i >= 0) {
					valid_part[new_i++] = valid_part[i];
				}
			}
			else {
				if (new_i < 0) new_i = i;
			}
		}

		if (sorted) break;
		
		vp_size = (new_i >= 0) ? new_i : vp_size;
	} while (true);
}


template <typename RandomIt, typename ExtractKey>
void swap_elements_us_flag(RandomIt first, partitions_t& partitions, part_indeces_t& valid_part,
						   int32_t vp_size, ExtractKey&& ek) 
{
	using std::swap;
	
	//
	// We iterate through the non-empty partitions and pick elements from the buffer that will
	// be swapped into place. We repeat the process until the correct element occupies the
	// current position.
	//
	if (vp_size < 2) return;
	
	for (int32_t i=0; i<vp_size-1; ++i) {
		auto cur_part = valid_part[i];
		auto val_off = partitions.offset[cur_part];
		auto val_next = partitions.next_offset[cur_part];

		for (;val_off != val_next;) {
			auto dest_part = ek(first[val_off]);
			if (dest_part != cur_part) {
				auto right_index = partitions.offset[dest_part];

				while (ek(first[right_index]) == dest_part) {
					++right_index;
				}

				partitions.offset[dest_part] = right_index + 1;
				
				swap(first[val_off], first[right_index]);
			}
			else {
				++val_off;
			}
		}
	}
}


//
// Helper functions for vector keys
//

template <typename S>
int32_t get_key_round(const ExtractStringChar<S>& ek)  {
	return ek.offset;
}

template <typename S>
int32_t get_key_round(const ExtractWStringChar<S, true>& ek)  {
	return ek.offset;
}

template <typename S>
int32_t get_key_round(const ExtractWStringChar<S, false>& ek)  {
	return ek.offset;
}

//
// std::sort() comparison functions for strings. The first 'round' characters are
// identical, so the comparison should start with the next character.
//

bool compare(const std::string& l, const std::string& r, int32_t round)  {
	return strcmp(&l[round], &r[round]) < 0;
}

bool compare(const std::wstring& l, const std::wstring& r, int32_t round)  {
	return wcscmp(&l[round], &r[round]) < 0;
}

bool compare(const char* l, const char* r, int32_t round)  {
	return strcmp(&l[round], &r[round]) < 0;
}

bool compare(const wchar_t* l, const wchar_t* r, int32_t round)  {
	return wcscmp(&l[round], &r[round]) < 0;
}

template <typename String>
bool end_of_string(const String& str, int32_t round)  {
	return str[round] == 0;
}

//
// Calls the continuation function for each partition.
// This function treats the key like a vector, the 'round' being the index
//
template <typename RandomIt, typename ExtractKey, typename NextSort>
void recurse_depth_first(RandomIt first, const partitions_t& partitions,
						 ExtractKey&& ek, NextSort&& continuation, vector_key_t) 
{
	int32_t round = get_key_round(ek);
	auto begin_offset = 0;
	
	for (int32_t i=0; i<256; ++i) {
		auto end_offset = partitions.next_offset[i];
		
		auto endp = first+end_offset;
		auto pp = std::partition(first+begin_offset, endp, [round](const auto& el) {
			return end_of_string(el, round);
		});
		
		if (pp >= endp-1) continue;
		
		auto diff = endp - pp;
		if (diff > 50) {		// magic number empirically determined
			continuation(pp, endp);
		}
		else {
			auto comp = [round](const auto& l, const auto& r) {
				return compare(l, r, round);
			};
			std::sort(pp, endp, comp);
		}
		
		begin_offset = end_offset;
	}
}

//
// Hack to get the sorting of wchar* arrays working
//
template <typename RandomIt>
void call_sort(RandomIt first, RandomIt last) {
	std::sort(first, last);
}

template <>
void call_sort<wchar_t**>(wchar_t** first, wchar_t** last) {
	auto comp = [](const wchar_t* l, const wchar_t* r) {
		return compare(l, r, 0);
	};
	std::sort(first, last, comp);
}

template <>
void call_sort<char**>(char** first, char** last) {
	auto comp = [](const char* l, const char* r) {
		return compare(l, r, 0);
	};
	std::sort(first, last, comp);
}

//
// Calls the continuation function for each partition.
//
template <typename RandomIt, typename ExtractKey, typename NextSort>
void recurse_depth_first(RandomIt first, const partitions_t& partitions,
						 ExtractKey&&, NextSort&& continuation, scalar_key_t) 
{
	auto begin_offset = 0;
	for (int32_t i=0; i<256; ++i) {
		auto end_offset = partitions.next_offset[i];
		auto diff = end_offset - begin_offset;
		if (diff > 75) {		// magic number empirically determined
			continuation(first+begin_offset, first+end_offset);
		}
		else if (diff > 1) {
			call_sort(first+begin_offset, first+end_offset);
		}
		
		begin_offset = end_offset;
	}
}

	
} // namespace azp
