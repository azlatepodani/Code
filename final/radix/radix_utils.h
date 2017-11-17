#pragma once


//
// Concepts:
//  1. RandomIt == random iterator; it has the same meaning as the STL's RandomAccessIterator
//  2. ExtractKey is a callable entity with the following signature:
//   	  uint8_t (const RandomIt::value_type&)
//
//


namespace azp {


using counters_t = std::array<int32_t, 256>;
using long_counters_t = std::array<int64_t, 256>;


//
// The following types implement the ExtractKey concept's requirements
//
	
//	
// Helper functor for the simplest case - identity mapping
// The signed version will translate the origin for correct sorting order
//
struct IdentityKey {
	// Type traits
	typedef bool use_round;	// scalar type
	
	uint8_t operator()(uint8_t val) {
		return val;
	}
	
	uint8_t operator()(int8_t val) {
		return (uint8_t)val + 128;
	}
};

//
// Functor for 16bit values - MSB
//
struct ExtractHighByte {
	// Type traits
	typedef bool use_round; // scalar type
	
	uint8_t operator()(uint16_t val) {
		return val >> 8;
	}
	
	uint8_t operator()(int16_t val) {
		return ((uint16_t)val + 0x8000) >> 8;
	}
};

//
// Functor for 16bit values - LSB
//
struct ExtractLowByte {
	// Type traits
	typedef bool use_round; // scalar type
	
	uint8_t operator()(uint16_t val) {
		return val & 0xFF;
	}
	
	uint8_t operator()(int16_t val) {
		return ((uint16_t)val + 0x8000) & 0xFF;
	}
};

//
// Functor for 32bit values - MSW
//
struct ExtractHighWord {
	// Type traits
	typedef uint32_t value_type;
	typedef bool use_round; // scalar type
	
	uint16_t operator()(const uint32_t& val) {
		return val >> 16;
	}
};

//
// Functor for 32bit values - LSW
//
struct ExtractLowWord {
	// Type traits
	typedef uint32_t value_type;
	typedef bool use_round; // scalar type
	
	uint16_t operator()(const uint32_t& val) {
		return val & 0xFFFF;
	}
};

//
// Functor for string values, returns a (w)char at a given position
//
struct ExtractStringChar {
	// Type traits
	typedef std::wstring value_type;  // see 'compose' functor and its uses
	typedef int use_round;            // vector type
	
	explicit ExtractStringChar(int offset) : offset(offset) {}
	uint8_t operator()(const std::string& str) {
		return str[offset];
	}
	
	uint16_t operator()(const std::wstring& str) {
		return str[offset];
	}
	
	int offset;
};

//
// Functor for composing functors. c(x) = f(g(x))
// This allows us to effectivelly combine some of the types above
//
template <typename T, typename U>
struct compose {
	// Type traits
	using use_round = typename T::use_round;

	compose(T& f, U& g) : f(f), g(g) {}
	
	uint8_t operator()(const typename U::value_type& val) { return f(g(val)); }
	
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
void compute_counts(counters_t& count, RandomIt first, RandomIt last, ExtractKey&& ek)
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
inline void compute_ranges(counters_t& count, counters_t& end_pos)
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
inline void compute_ranges(counters_t& count, counters_t& end_pos, counters_t& chain)
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
inline void reduce_chain(const counters_t& count, const counters_t& end_pos, counters_t& chain) {
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
void swap_elements_into_place(RandomIt first, counters_t& count, const counters_t& end_pos, ExtractKey&& ek)
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
							  counters_t& chain, ExtractKey&& ek)
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


	
} // namespace azp
