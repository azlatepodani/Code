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

template <typename RandomIt>
using value_type = typename std::iterator_traits<RandomIt>::value_type;

//
// Prepares the 'values' array for a call to reservoir_sampling()
// Returns: 'first' + K.
//
// Preconditions: *(first+K-1) is within the range
//
template <size_t K, typename RandomIt>
RandomIt reservoir_sampling_start(RandomIt first, std::array<value_type<RandomIt>, K>& values)
{
	for (size_t i=0; i<K; ++i) {
		values[i] = *first++;
	}
	
	return first;
}

//
// Selects random k distinct elements from a stream of data with k/n individual probability
// The function updates the previous selection and can be used to process the data in chunks
// Runtime is O(N) and uses O(K) space
// Returns: N + last - first
//
// Preconditions:
// 'values' should have been initialized by reservoir_sampling_start()
// RNG is a uniform random bit generator (UniformRandomBitGenerator)
// N is the number of elements already processed from the stream. 
// 'first' should point to the element that logically follows the first N in the stream
//
template <size_t K, typename RandomIt, typename RNG>
int reservoir_sampling(RandomIt first, RandomIt last, std::array<value_type<RandomIt>, K>& values, int N, RNG& g)
{
	std::array<const value_type<RandomIt>*, K> selection;
	int currentN = N;
	std::uniform_int_distribution<size_t> uni;
	
	selection.fill(nullptr);
	
	for (; first != last; ++first) {
		auto pos = uni(g, {0, currentN++});
		if (pos < K) {
			selection[pos] = &(*first);
		}
	}

	// copy the individual values
	for (size_t i=0; i<K; ++i) {
		if (selection[i]) values[i] = *selection[i]; 
	}

	return currentN;
}

//
// Selects random k distinct elements from an array of data with k/n individual probability
// If the range has less than K elements, the result it undefined
//
template <size_t K, typename RandomIt>
std::array<value_type<RandomIt>, K> sample_k_elements(RandomIt first, RandomIt last)
{
	std::array<size_t, K> selection;
	
	for (size_t i=0; i<K; ++i) {
		selection[i] = i;
	}
	
	LARGE_INTEGER ln;
	QueryPerformanceCounter(&ln);
	std::mt19937 g(ln.QuadPart);
	
	std::uniform_int_distribution<size_t> uni;
	size_t N = last - first;

	for (size_t i=K; i < N; ++i) {
		auto pos = uni(g, {0, i});
		if (pos < K) {
			selection[pos] = i;
		}
	}
	
	using T = typename std::iterator_traits<RandomIt>::value_type;
	std::array<T, K> values;
	
	for (size_t i=0; i<K; ++i) {
		values[i] = first[selection[i]]; 
	}
	
	return values;
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
		for (int saved=i; ;) {	// not starting at i=0 is a 30% speed improvement.
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


