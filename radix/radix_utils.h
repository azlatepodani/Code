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
// The signed version will translate the origin for correct sorting order
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
		return ((uint16_t)val + 0x8000) & 0xFF;
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
// Converts the 'count' array in-place to an array of start positions and
// sets the next links in 'chain' for each non-empty partition.
// The 'end_pos' array will contain the end positions (open ended interval)
//
// Preconditions:
//  1. 'count' was initialized by compute_counts()
//
inline int compute_ranges(partitions_t& partitions, counters_t& valid_part) NEX
{
	int sum = partitions[0].count;
	partitions[0].offset = 0;
	
	int vp_size = 0;
	
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

template <bool val>
struct decide {
	template <typename Fn1, typename Fn2>
	decide(Fn1&& fn1, Fn2&&) {
		fn1();
	}
};

template <>
struct decide<false> {
	template <typename Fn1, typename Fn2>
	decide(Fn1&& , Fn2&& fn2) {
		fn2();
	}
};


//
// Sorts the data in the buffer pointed to by 'first' by the start positions in 'count' in linear time.
// The function uses the unqualified call to swap() to allow for client customisation.
// The 'count' array elements will be modified by the function.
//
// Preconditions:
//  1. 'count', 'end_pos' and 'chain' are the result of compute_ranges()
//  2. The buffer pointed to by 'first' is identical to the one that generated the ranges
//
template <typename RandomIt, typename ExtractKey>
void swap_elements_into_place(RandomIt first, partitions_t& partitions, counters_t& valid_part,
							  int vp_size, ExtractKey&& ek) NEX
{
	using std::swap;
	bool sorted = true;
	
	//
	// In adition to the technique in swap_elements_into_place() from above, we use the chain array
	// to jump to the next non-empty partition. After each round, the chain is reduced to the
	// current non-empty set.
	//
	if (!vp_size) return;
	
	do {
		sorted = true;
		int last_invalid = 0;
		for (int i=0; i<vp_size; ++i)
		{
			auto val = partitions[valid_part[i]];
			
			valid_part[last_invalid++] = (val.offset < val.next_offset)
									   ? valid_part[last_invalid] : valid_part[i];
									   
			for (; val.offset < val.next_offset; ++val.offset)
			{
				auto left = ek(first[val.offset]);
				auto right_index = partitions[left].offset++;

				if (std::is_scalar<std::iterator_traits<RandomIt>::value_type>::value) {
					sorted = false;
					swap(first[val.offset], first[right_index]);
				}
				else {
					if (val.offset != right_index) {
						sorted = false;
						swap(first[val.offset], first[right_index]);
					}
				}

				val.offset++;
			}
		}

		if (sorted) break;
		vp_size = last_invalid;
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
	
	for (int i=0; i<256; ++i) {
		auto begin_offset = i ? partitions[i-1].next_offset : 0;
		auto end_offset = partitions[i].next_offset;
		
		auto endp = first+end_offset;
		auto pp = std::partition(first+begin_offset, endp, [r=round+1](const auto& el) {
			return end_of_string(el, r);
		});
		
		if (pp == endp) continue;
		
		auto diff = endp - pp;
		if (diff > 50) {		// magic number empirically determined
			continuation(pp, endp);
		}
		else if (diff > 1) {
			auto comp = [round](const auto& l, const auto& r) {
				return compare(l, r, round);
			};
			std::sort(pp, endp, comp);
		}
	}
}

//
// Calls the continuation function for each partition.
//
template <typename RandomIt, typename ExtractKey, typename NextSort>
void recurse_depth_first(RandomIt first, const partitions_t& partitions,
						 ExtractKey&&, NextSort&& continuation, scalar_key_t) NEX
{
	for (int i=0; i<256; ++i) {
		auto begin_offset = i ? partitions[i-1].next_offset : 0;
		auto end_offset = partitions[i].next_offset;
		auto diff = end_offset - begin_offset;
		if (diff > 75) {		// magic number empirically determined
			continuation(first+begin_offset, first+end_offset);
		}
		else if (diff > 1) {
			std::sort(first+begin_offset, first+end_offset);
		}
	}
}

	
} // namespace azp
