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
	uint8_t operator()(uint16_t val) {
		return val & 0xFF;
	}
	
	uint8_t operator()(int16_t val) {
		return ((uint16_t)val + 0x8000) & 0xFF;
	}
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
	for (auto it = first; it != last; ++it) {
		++count[ek(*it)];
	}
}


// struct partition_t {
	// int count;
	// int next;    // next non-empty partition
// };


// using partitions_t = std::array<partition_t, 256>;
constexpr int Part_chain_end = 256;

//
// Counts the number of times each distinct 8bit key appears in the data.
//
// Preconditions:
//  1. The range [first, last) will have less than INT_MAX elements
//  2. The output parameter 'partitions' was 0-initialized
//
// template <typename RandomIt, typename ExtractKey>
// void compute_counts(partitions_t& partitions, RandomIt first, RandomIt last, ExtractKey&& ek)
// {
	// for (auto it = first; it != last; ++it) {
		// ++partitions[ek(*it)].count;
	// }
// }

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
int compute_ranges(counters_t& count, counters_t& end_pos)
{
	int sum = count[0];
	count[0] = 0;
	int hits = 0;
	
	for (int i=1; i<256; ++i) {
		auto val = count[i];
		end_pos[i-1] = sum;
		count[i] = sum;
		sum += val;
		hits += val?1:0;
	}
	
	end_pos[255] = sum;
	
	return hits;
}

//
// Converts the 'partitions' array in-place to an array of start positions and
// sets the next links for each non-empty partition.
// The 'end_pos' array will contain the end positions (open ended interval)
//
// Preconditions:
//  1. 'partitions' was initialized by compute_counts()
//
// int compute_ranges(partitions_t& partitions, counters_t& end_pos)
// {
	// int sum = partitions[0].count;
	// partitions[0].count = 0;
	
	// partition_t* prev = &partitions[255];
	// partitions[255].next = Part_chain_end;
	
	// for (int i=1; i<256; ++i) {
		// auto val = partitions[i].count;
		// if (val) {
			// prev->next = i;
			// prev = &partitions[i];
		// }
		
		// end_pos[i-1] = sum;
		// partitions[i].count = sum;
		// sum += val;
	// }
	
	// int chain_start = partitions[255].next;
	// prev->next = Part_chain_end;
	// end_pos[255] = sum;
	// partitions[255].next = Part_chain_end; // it used to hold the first position in the chain
	
	// return chain_start;
// }

int compute_ranges(counters_t& count, counters_t& end_pos, counters_t& chain)
{
	int sum = count[0];
	count[0] = 0;
	
	int prev = 0;
	chain[0] = Part_chain_end;
	
	int hits = 0;
	
	for (int i=1; i<256; ++i) {
		auto val = count[i];
		if (val) {
			chain[prev++] = i;
			hits++;
		}
		
		end_pos[i-1] = sum;
		count[i] = sum;
		sum += val;
	}
	
	chain[prev] = Part_chain_end;
	end_pos[255] = sum;
	
	return hits;
}

//
// Removes from the partitions chain the nodes that were finished or empty
//
// Preconditions:
//  1. the parameters were computed using compute_ranges()
//
void reduce_chain(const counters_t& count, const counters_t& end_pos, counters_t& chain) {
	int pos = 0;
	int i = 0;
	auto key = chain[0];
	
	while (key != Part_chain_end) {
		auto next_key = chain[pos+1];
		
		if (count[key] != end_pos[key]) {
			chain[i++] = chain[pos];
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
		// The standard radix algorithm would swap the element pointed to by first
		// until the right value lands in the first position, then increment first
		// and repeat. This introduces a data dependency that slows down the loop.
		// Instead, we iterate through the start positions array and use it to move
		// the elements into place. The elements that are already in their final 
		// position will not be visited twice.
		//
		for (int i=0; i < 256; ++i) {
			for (auto val = count[i]; val < end_pos[i]; ++val) {
				auto left = ek(first[val]);
				auto right_index = count[left]++;
				if (right_index != val) {
					sorted = false;
					swap(first[val], first[right_index]);
				}
			}
		}
	} while (!sorted);
}

//
// Sorts the data in the buffer pointed to by 'first' by the start positions in 'partitions' in linear time.
// The function uses the unqualified call to swap() to allow for client customisation.
// The partitions array elements will be modified by the function.
//
// Preconditions:
//  1. 'partitions', 'end_pos' and 'chain_start' are the result of compute_ranges()
//  2. The buffer pointed to by 'first' is identical to the one that generated the ranges
//
template <typename RandomIt, typename ExtractKey>
void swap_elements_into_place(RandomIt first, counters_t& count, const counters_t& end_pos,
							  counters_t& chain, ExtractKey&& ek)
{
	using std::swap;
	bool sorted = true;
	
	//
	// In adition to the technique in swap_elements_into_place(RandomIt first, counters_t count...),
	// we use the partition_t::next field to jump to the next non-empty partition. After each round,
	// the chain is reduced to the current non-empty set.
	//

	do {
		sorted = true;
		
		int pos = 0;
		auto key = chain[0];
		
		while (key != Part_chain_end) {
			auto next_key = chain[pos+1];
			
			for (auto val = count[key]; val < end_pos[key]; ++val) {
				auto left = ek(first[val]);
				auto right_index = count[left]++;
				if (right_index != val) {
					sorted = false;
					swap(first[val], first[right_index]);
				}
			}
			
			pos++;
			key = next_key;
		}

		if (sorted) break;
		reduce_chain(count, end_pos, chain);
	} while (!sorted);
}


	
} // namespace azp