#pragma once
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

#define NEX  noexcept
#define CST_NEX const NEX 


using counters_t = std::array<int32_t, 256>;
using long_counters_t = std::array<int64_t, 256>;

using recurse_table_t = std::array<std::pair<int32_t, int32_t>, 257>;


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
	
	uint8_t operator()(uint8_t val) CST_NEX {
		return val;
	}
	
	uint8_t operator()(int8_t val) CST_NEX {
		return (uint8_t)val + 128;
	}
};


template <typename T, int32_t off>
struct ExtractByOffset {
	// Type traits
	typedef T value_type;
	typedef scalar_key_t use_round;
	
	uint8_t operator()(const T& val) CST_NEX {
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
	
	explicit ExtractStringChar(int32_t offset) NEX
		: offset(offset)
	{ }
	
	uint8_t	operator()(const String& str) CST_NEX {
		return str[offset];
	}

	const int32_t offset;
};


template <typename String, bool high>
struct ExtractWStringChar {
	// Type traits
	typedef String value_type;
	typedef vector_key_t use_round;
	
	explicit ExtractWStringChar(int32_t offset) NEX
		: offset(offset)
	{ }
	
	uint16_t operator()(const String& str) CST_NEX {
		uint16_t ch = str[offset];
		if (high) return ch >> 8;
		return ch & 0xFF;
	}

	const int32_t offset;
};


struct partitions_t {
	partitions_t() NEX { }
	
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
partitions_t compute_counts(RandomIt first, RandomIt last, ExtractKey&& ek) NEX
{
	partitions_t partitions;
	
	for (; first != last; ++first) {
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
inline int32_t compute_ranges(partitions_t& partitions, counters_t& valid_part) NEX
{
	int32_t sum = partitions.count[0];
	partitions.offset[0] = 0;
	
	int32_t vp_size = sum ? 1 : 0;
	valid_part[0] = 0;
	
	for (int32_t i=1; i<256; ++i) {
		auto count = partitions.count[i];

		partitions.next_offset[i-1] = sum;
		partitions.offset[i] = sum;
		
		valid_part[vp_size] = i;
		vp_size += count ? 1 : 0;
		sum += count;
	}
	
	partitions.next_offset[255] = sum;
	
	return vp_size;
}


//
// Eliminates the negative values from the array and returns the updated size.
// A negative index means that the original partition is now processed.
// @see swap_elements_into_place()
//
inline int32_t compress_vp(counters_t& valid_part, int32_t vp_size) {
	int32_t new_size = 0;
	int32_t i = 0;
	
	while (vp_size) {
		vp_size--;
		if (valid_part[i++] < 0) { break; }
		new_size++;
	}
	
	while (vp_size) {
		vp_size--;
		if (valid_part[i] >= 0) valid_part[new_size++] = valid_part[i];
		i++;
	}
	
	return new_size;
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
void swap_elements_into_place(RandomIt first, partitions_t& partitions, counters_t& valid_part,
							  int32_t vp_size, ExtractKey&& ek) NEX
{
	using std::swap;
	bool sorted = true;
	
	//
	// We iterate through the non-empty partitions and pick elements from the buffer that will
	// be swapped into place. A partition is empty if the offset and next_offset fields are equal.
	// The empty partitions are marked in the 'valid_part' array by using a negative value.
	//
	if (vp_size < 2) return;
	
	do {
		sorted = true;
		
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
			}
			else {
				valid_part[i] = -1;	// mark as done
			}
		}

		if (sorted) break;
		
		vp_size = compress_vp(valid_part, vp_size);
	} while (true);
}


//
// Helper functions for vector keys
//

template <typename S>
int32_t get_key_round(const ExtractStringChar<S>& ek) NEX {
	return ek.offset;
}

template <typename S>
int32_t get_key_round(const ExtractWStringChar<S, true>& ek) NEX {
	return ek.offset;
}

template <typename S>
int32_t get_key_round(const ExtractWStringChar<S, false>& ek) NEX {
	return ek.offset;
}

//
// std::sort() comparison functions for strings. The first 'round' characters are
// identical, so the comparison should start with the next character.
//

bool compare(const std::string& l, const std::string& r, int32_t round) NEX {
	return strcmp(&l[round], &r[round]) < 0;
}

bool compare(const std::wstring& l, const std::wstring& r, int32_t round) NEX {
	return wcscmp(&l[round], &r[round]) < 0;
}

bool compare(const char* l, const char* r, int32_t round) NEX {
	return strcmp(&l[round], &r[round]) < 0;
}

bool compare(const wchar_t* l, const wchar_t* r, int32_t round) NEX {
	return wcscmp(&l[round], &r[round]) < 0;
}

template <typename String>
bool end_of_string(const String& str, int32_t round) NEX {
	return str[round] == 0;
}

//
// Calls the continuation function for each partition.
// This function treats the key like a vector, the 'round' being the index
//
template <typename RandomIt, typename ExtractKey, typename NextSort>
void recurse_depth_first(RandomIt first, const partitions_t& partitions,
						 ExtractKey&& ek, NextSort&& continuation, vector_key_t) NEX
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

//
// Calls the continuation function for each partition.
//
template <typename RandomIt, typename ExtractKey, typename NextSort>
void recurse_depth_first(RandomIt first, const partitions_t& partitions,
						 ExtractKey&&, NextSort&& continuation, scalar_key_t) NEX
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
