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

//with VC it seems that adding noexcept slows things down a little
#define NEX  /*noexcept*/
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
// This allows us to effectivelly combine some of the types above
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




//
// Counts the number of times each distinct 8bit key appears in the data.
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//  2. The output parameter 'count' was 0-initialized
//
template <typename RandomIt, typename ExtractKey>
void compute_counts(counters_t& count, RandomIt first, RandomIt last,
					ExtractKey&& ek) NEX
{
	for (; first != last; ++first) {
		++count[ek(*first)];
	}
}

//
// Converts the 'count' array in-place to an array of start positions
// The 'end_pos' array will contain the end positions (open ended interval)
// in count   2 4 3
// out count  0 2 6
//   end_pos  2 6 9
//
// Preconditions:
//  1. 'count' was initialized by compute_counts()
//
inline void compute_ranges(counters_t& count, counters_t& end_pos) NEX
{
	int sum = count[0];
	count[0] = 0;
	
	for (int i=1; i<256; ++i) {
		auto val = count[i];
		end_pos[i-1] = sum;
		count[i] = sum;
		sum += val;
	}
	
	end_pos[255] = sum;
}

constexpr int Part_chain_end = 256;

//
// Converts the 'count' array in-place to an array of start positions and
// sets the next links in 'chain' for each non-empty partition.
// The 'end_pos' array will contain the end positions (open ended interval)
//
// Preconditions:
//  1. 'count' was initialized by compute_counts()
//
inline void compute_ranges(counters_t& count, counters_t& end_pos,
						   counters_t& chain) NEX
{
	int sum = count[0];
	count[0] = 0;
	
	int prev = 0;
	
	for (int i=1; i<256; ++i) {
		auto val = count[i];

		end_pos[i-1] = sum;

		if (count[i]) {
			chain[prev++] = i;
		}
		
		count[i] = sum;
		sum += val;
	}
	
	chain[prev] = Part_chain_end;
	end_pos[255] = sum;
}

//
// Removes from the partitions chain the nodes that were finished or empty
//
// Preconditions:
//  1. the parameters were computed using compute_ranges()
//
inline void reduce_chain(const counters_t& count, const counters_t& end_pos,
						 counters_t& chain) NEX
{
	int pos = 0;
	int i = 0;
	auto key = chain[0];
	
	while (key != Part_chain_end) {
		auto next_key = chain[pos+1];
		
		if (count[key] != end_pos[key]) {
			chain[i++] = key;
		}
		
		pos++;
		key = next_key;
	}
	
	chain[i] = Part_chain_end;
}

//
// Sorts the data in the buffer pointed to by 'first' by the start positions in 'count' in linear time.
// The function uses the unqualified call to swap() to allow for client customisation.
// The count array elements will be modified by the function.
//
// Preconditions:
//  1. 'count', 'end_pos' are the result of compute_ranges()
//  2. The buffer pointed to by 'first' is identical to the one that generated the ranges
//
template <typename RandomIt, typename ExtractKey>
void swap_elements_into_place(RandomIt first, counters_t& count, const counters_t& end_pos,
							  ExtractKey&& ek) NEX
{
	using std::swap;
	bool sorted = true;
	
	do {
		sorted = true;

		//
		// We iterate through the start positions array and use it to move
		// the elements into place. The elements that are already in their final 
		// position will not be visited twice.
		// The following loop used to be two nested loops, but the current version is 15% faster
		// for (i in 0..255)
		//   for (val in count[i]..end_pos[i])
		//
		
		auto val = count[0];
		for (int i=0; ; ) {
			if (val == end_pos[i]) {
				i++;
				if (i == 256) break;
				val = count[i];
			}
			else {
				auto left = ek(first[val]);
				auto right_index = count[left]++;
				if (right_index != val) {
					sorted = false;
					swap(first[val], first[right_index]);
				}
				val++;
			}
		}
	} while (!sorted);
}

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
void swap_elements_into_place(RandomIt first, counters_t& count, const counters_t& end_pos,
							  counters_t& chain, ExtractKey&& ek) NEX
{
	using std::swap;
	bool sorted = true;
	
	//
	// In adition to the technique in swap_elements_into_place() from above, we use the chain array
	// to jump to the next non-empty partition. After each round, the chain is reduced to the
	// current non-empty set.
	//
	if (chain[0] == Part_chain_end) return;
	
	do {
		sorted = true;
		
		auto key = chain[0];
		auto val = count[chain[0]];
		for (int pos = 0; ;)			// this was initially two nested loops as well
		{
			auto next_key = chain[pos+1];
			
			if (val == end_pos[key]) {
				if (next_key == Part_chain_end) break;
				pos++;
				key = next_key;
				val = count[next_key];
			}
			else {
				auto left = ek(first[val]);
				auto right_index = count[left]++;
				if (right_index != val) {
					sorted = false;
					swap(first[val], first[right_index]);
				}
				val++;
			}
		}

		if (sorted) break;
		reduce_chain(count, end_pos, chain);
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

//
// Functions that check if a string has reached its end for a given round.
// These are already sorted.
//

template <typename String>
bool end_of_string(const String& str, int round) NEX {
	return str.size() <= unsigned(round);
}

template <>
bool end_of_string<char *>(char* const& str, int round) NEX {
	return str[round] == 0;
}

template <>
bool end_of_string<wchar_t *>(wchar_t* const& str, int round) NEX {
	return str[round] == 0;
}

//
// Calls the continuation function for each partition.
// This function treats the key like a vector, the 'round' being the index
//
template <typename RandomIt, typename ExtractKey, typename NextSort>
void recurse_depth_first(RandomIt first, recurse_table_t& recurse_table, ExtractKey&& ek,
						 NextSort&& continuation, vector_key_t) NEX
{
	int round = get_key_round(ek);
	
	for (int i=0; recurse_table[i].second; ++i) {
		while (end_of_string(first[recurse_table[i].first], round+1)) {
			++recurse_table[i].first;
			if (recurse_table[i].first == recurse_table[i].second) goto _after_sort;
		}
		
		auto diff = recurse_table[i].second - recurse_table[i].first;
		if (diff > 50) {		// magic number empirically determined
			continuation(first+recurse_table[i].first, first+recurse_table[i].second);
		}
		else if (diff > 1) {
			auto comp = [round](const auto& l, const auto& r) {
				return compare(l, r, round);
			};
			std::sort(first+recurse_table[i].first, first+recurse_table[i].second, comp);
		}
		
		_after_sort:;
	}
}

//
// Calls the continuation function for each partition.
//
template <typename RandomIt, typename ExtractKey, typename NextSort>
void recurse_depth_first(RandomIt first, recurse_table_t& recurse_table, ExtractKey&&,
						 NextSort&& continuation, scalar_key_t) NEX
{
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

//
// Creates a table for each partition that has at least 2 elements.
//
recurse_table_t fill_recurse_table(const counters_t& count) NEX {
	recurse_table_t recurse_table;
	
	int32_t start = 0;
	int pos = 0;
	for (int i=0; i<256; ++i) {
        auto val = count[i];
		if (count[i] > 1) {
			recurse_table[pos++] = std::make_pair(start, start+val);
		}
		
		start += val;
	}
	
	recurse_table[pos].second = 0;
	
	return recurse_table;
}
	
} // namespace azp
