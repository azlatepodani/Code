#pragma once


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
using vector_key_t = int;

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

//
// Functor for 16bit values - MSB
//
struct ExtractHighByte {
	// Type traits
	typedef scalar_key_t use_round;
	
	uint8_t operator()(uint16_t val) CST_NEX {
		return val >> 8;
	}
	
	uint8_t operator()(int16_t val) CST_NEX {
		return ((uint16_t)val + 0x8000) >> 8;
	}
};

//
// Functor for 16bit values - LSB
//
struct ExtractLowByte {
	// Type traits
	typedef scalar_key_t use_round;
	
	uint8_t operator()(uint16_t val) CST_NEX {
		return val & 0xFF;
	}
	
	uint8_t operator()(int16_t val) CST_NEX {
		return (uint16_t)val & 0xFF;
	}
};

//
// Functor for 32bit values - MSW
//
struct ExtractHighWord {
	// Type traits
	typedef uint32_t value_type;
	typedef scalar_key_t use_round;
	
	uint16_t operator()(const uint32_t& val) CST_NEX {
		return val >> 16;
	}
};

//
// Functor for 32bit values - LSW
//
struct ExtractLowWord {
	// Type traits
	typedef uint32_t value_type;
	typedef scalar_key_t use_round;
	
	uint16_t operator()(const uint32_t& val) CST_NEX {
		return val & 0xFFFF;
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
	
	explicit ExtractStringChar(int offset) NEX
		: offset(offset)
	{ }
	
	template <typename String>
	struct key_type {
		using type = uint8_t;
	};
	
	template <>
	struct key_type<std::wstring> {
		using type = uint16_t;
	};
	
	template <>
	struct key_type<wchar_t*> {
		using type = uint16_t;
	};
	
	template <typename String>
	using key_type_t = typename key_type<String>::type;
	
	key_type_t<String>
	operator()(const String& str) CST_NEX {
		return str[offset];
	}

	const int offset;
};

//
// Functor for composing functors. c(x) = f(g(x))
// This allows us to effectively combine some of the types above
//
template <typename T, typename U>
struct compose {
	// Type traits
	using use_round = typename T::use_round;

	compose(T&& f, U&& g) NEX
		: f(f), g(g)
	{ }
	
	uint8_t operator()(const typename U::value_type& val) CST_NEX
	{ return f(g(val)); }
	
	T& f;
	U& g;
};


struct partition_t {
	partition_t() NEX
		: count(0)
	{ }
	
	union {
		int count;
		int offset;
	};
	int next_offset;
};

using partitions_t = std::array<partition_t, 256>;


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
		++partitions[ek(*first)].count;
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
inline int compute_ranges(partitions_t& partitions, counters_t& valid_part) NEX
{
	int sum = partitions[0].count;
	partitions[0].offset = 0;
	
	int vp_size = sum ? 1 : 0;
	valid_part[0] = 0;
	
	for (int i=1; i<256; ++i) {
		auto count = partitions[i].count;

		partitions[i-1].next_offset = sum;
		partitions[i].offset = sum;
		
		if (count) {
			valid_part[vp_size++] = i;
			sum += count;
		}
	}
	
	partitions[255].next_offset = sum;
	
	return vp_size;
}

//
// Eliminates the negative values from the array and returns the updated size.
// A negative index means that the original partition is now processed.
// @see swap_elements_into_place()
//
inline int compress_vp(counters_t& valid_part, int vp_size) {
	int new_size = 0;
	int i = 0;
	
	while (vp_size--) {
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
							  int vp_size, ExtractKey&& ek) NEX
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
		
		for (int i=0; i<vp_size; ++i)
		{
			auto val = partitions[valid_part[i]];
			
			if (val.offset < val.next_offset) {
				for (;;)
				{
					auto left = ek(first[val.offset]);
					auto right_index = partitions[left].offset++;

					sorted = false;
					swap(first[val.offset], first[right_index]);
					
					if (++val.offset == val.next_offset) break;
				}
			}
			else {
				valid_part[i] = -1;	// mark as done
			}
		}

		if (sorted) break;
		
		vp_size = compress_vp(valid_part, vp_size);
	} while (true);
}


template <typename S>
using compose_ch = compose<ExtractHighByte, ExtractStringChar<S>>;

template <typename S>
using compose_cl = compose<ExtractLowByte,  ExtractStringChar<S>>;

//
// Helper functions for vector keys
//
template <typename S>
int get_key_round(const compose_ch<S>& ek) NEX {
	return ek.g.offset;
}

template <typename S>
int get_key_round(const compose_cl<S>& ek) NEX {
	return ek.g.offset;
}

template <typename S>
int get_key_round(const ExtractStringChar<S>& ek) NEX {
	return ek.offset;
}

//
// std::sort() comparison functions for strings. The first 'round' characters are
// identical, so the comparison should start with the next character.
//

bool compare(const std::string& l, const std::string& r, int round) NEX {
	return strcmp(&l[round], &r[round]) < 0;
}

bool compare(const std::wstring& l, const std::wstring& r, int round) NEX {
	return wcscmp(&l[round], &r[round]) < 0;
}

bool compare(const char* l, const char* r, int round) NEX {
	return strcmp(&l[round], &r[round]) < 0;
}

bool compare(const wchar_t* l, const wchar_t* r, int round) NEX {
	return wcscmp(&l[round], &r[round]) < 0;
}

template <typename String>
bool end_of_string(const String& str, int round) NEX {
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
	int round = get_key_round(ek);
	auto begin_offset = 0;
	
	for (int i=0; i<256; ++i) {
		auto end_offset = partitions[i].next_offset;
		
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
	for (int i=0; i<256; ++i) {
		auto end_offset = partitions[i].next_offset;
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
