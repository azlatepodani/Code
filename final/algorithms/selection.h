#include <array>


namespace azp {

namespace detail {
	
	template <typename RandomIt>
	RandomIt top_element_candidate(RandomIt first, RandomIt last, int threshold);

	template <int K, typename RandomIt>
	std::array<RandomIt, K> top_k_candidates(RandomIt first, RandomIt last, int threshold);

}


//
// Determines in O(n) the element wich appears more often than the others
// Uses O(1) aditional space
//
// Precondition: N < INT_MAX
//
template <typename RandomIt>
RandomIt top_element(RandomIt first, RandomIt last)
{
	return detail::top_element_candidate(first, last, 1);
}

//
// Determines in O(n) the element wich appears >50% of the time in the array
// aka Boyer-Moore majority vote 
// Uses O(1) aditional space
//
// Precondition: N < INT_MAX
//
template <typename RandomIt>
RandomIt majority_element(RandomIt first, RandomIt last)
{
	return detail::top_element_candidate(first, last, (last - first) / 2);
}


//
// Computes the top K elements by frequency in O(N) time and O(K) space
// The resulting array will have 'last' as a marker for each unused entry
//
// Precondition: N < INT_MAX
//
template <int K, typename RandomIt>
std::array<RandomIt, K> top_k_elements(RandomIt first, RandomIt last)
{
	return detail::top_k_candidates<K>(first, last, 1);
}


//
// Computes up to K elements that appear more often than N/K in the input
// Runtime O(N) and O(K) space required
// The resulting array will have 'last' as a marker for each unused entry
//
// Precondition: N < INT_MAX
//
template <int K, typename RandomIt>
std::array<RandomIt, K> heavy_hitters(RandomIt first, RandomIt last)
{
	if (last - first <= K) {	// nothing to compute
		std::array<RandomIt, K> result;
		result.fill(last);
		return result;
	}

	return detail::top_k_candidates<K>(first, last, (last - first) / K);
}




namespace detail {
	
//
// Determines in O(N) the element wich appears more than the others in a collection
// Uses O(1) aditional space
// Precondition: threshold and N < INT_MAX
//
template <typename RandomIt>
RandomIt top_element_candidate(RandomIt first, RandomIt last, int threshold)
{
	RandomIt value = first;
	int count = 0;
	RandomIt saved = first;
	
	// find the likely majority candidate
	while (first != last) {
		if (*value == *first) {
			count++;
		}
		else {
			count--;
			if (count == 0) {
				value = first;
				count = 1;
			}
		}
		
		++first;
	}
	
	if (!count) return last;

	// verify the candidate against the threshold
	first = saved;
	threshold++;	// we want the item to apear at least threshold + 1 times
	while (first != last) {
		if (*first++ == *value) {
			threshold--;
			if (!threshold) return value;
		}
	}
	
	return last;
}

	
//
// Checks if an element appears at least threshold times in the input
// 'value' is set to 'last' if not.
// Precondition: threshold < INT_MAX
//
template <typename RandomIt>
void check_heavy_hitter(RandomIt& value, RandomIt first, RandomIt last, int threshold)
{
	threshold++;	// we want the item to apear at least threshold + 1 times
	for (; first != last; ++first) {
		if (*value == *first) {
			threshold--;
			if (!threshold) return;
		}
	}
	
	value = last;
}


//
// Computes the top K elements by frequency in O(N) time and O(K) space
//
// Precondition: threshold and N < INT_MAX
//
template <int K, typename RandomIt>
std::array<RandomIt, K> top_k_candidates(RandomIt first, RandomIt last, int threshold)
{
	std::array<RandomIt, K> value;
	int count[K] = {0};
	int base = 0;
	RandomIt f = first;
	
	value.fill(last);
	
	int i = 0;
	while (first != last) {
		for (int saved=i; ;) {
			bool positive_count = count[i] > base;
			
			if (positive_count && *(value[i]) == *first) {
				auto cnt = count[i];
				count[i] = cnt ? cnt + 1 : base + 1;
				break;
			}
			else if (!positive_count) {
				value[i] = first;
				count[i] = base + 1;
				break;
			}
			else {
				++i;
				if (i == K) {
					i = 0;	// wrap around
				}
				if (i == saved) {
					base++;	// the table is full -> start replacing elements
				}
			}
		}
		
		++first;
	}
	
	for (int i=0; i<K; ++i) {
		if (count[i] <= base) value[i] = last;
		else check_heavy_hitter(value[i], f, last, threshold);
	}
	
	return value;
}

} // namespace detail


} // namespace azp


