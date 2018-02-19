#pragma once
#include <utility>
#include <iterator>
#include <stddef.h>


namespace azp {


constexpr int sort_threshold = 16;


template <typename RandIt>
void rotate_last(RandIt last, RandIt pos) {
	using std::swap;
	using T = typename std::iterator_traits<RandIt>::value_type;
	
	T val(std::move(*last));
	RandIt next = last - 1;
	
	while (pos != last) {
		*last = std::move(*next);
		last = next;
		--next;
	}
	
	*last = std::move(val);
}


template <typename RandIt>
void unguarded_linear_insert(RandIt last) {
	using std::swap;
	using T = typename std::iterator_traits<RandIt>::value_type;
	
	T val(std::move(*last));
	RandIt next = last-1;
	
	while (val < *next) {
		*last = std::move(*next);
		last = next;
		--next;
	}
	
	*last = std::move(val);
}


template <typename RandIt, typename Compare>
void unguarded_linear_insert(RandIt last, Compare&& comp) {
	using std::swap;
	using T = typename std::iterator_traits<RandIt>::value_type;
	
	T val(std::move(*last));
	RandIt next = last-1;
	
	while (comp(val, *next)) {
		*last = std::move(*next);
		last = next;
		--next;
	}
	
	*last = std::move(val);
}



// template <typename RandIt>
// void unguarded_insertion_sort(RandIt first, RandIt last) {
	// for (; first != last; ++first) {
		// unguarded_linear_insert(first);
	// }
// }


template <typename RandIt, typename T>
struct insertion_sort_imp {
	void operator()(RandIt first, RandIt last, const T&) {
		for (RandIt i=first+1; i != last; ++i) {
			if (*i < *first) {
				rotate_last(i, first);
			}
			else {
				unguarded_linear_insert(i);
			}
		}
	}
};
	
template <typename RandIt, typename T, typename Compare>
struct insertion_sort_imp2 {
	void operator()(RandIt first, RandIt last, Compare&& comp, const T&) {
		for (RandIt i=first+1; i != last; ++i) {
			if (comp(*i,*first)) {
				rotate_last(i, first);
			}
			else {
				unguarded_linear_insert(i, comp);
			}
		}
	}
};


// template <typename RandIt, typename Compare>
// struct insertion_sort_imp2<RandIt, std::wstring, Compare> {
	// void operator()(RandIt first, RandIt last, Compare&& comp, const std::wstring&) {
		// for (RandIt i=first+1; i != first+sort_threshold; ++i) {
			// if (comp(*i,*first)) {
				// rotate_last(i, first);
			// }
			// else {
				// unguarded_linear_insert(i, comp);
			// }
		// }
		
		// for (RandIt i=first+sort_threshold; i != last; ++i) {
			// auto ptr = azp::upper_bound(first, i, *i, comp);
			// if (ptr != i) {
				// rotate_last(i, ptr);
			// }
		// }
	// }
// };


template <typename RandIt>
void insertion_sort(RandIt first, RandIt last) {
	using T = typename std::iterator_traits<RandIt>::value_type;
	insertion_sort_imp<RandIt, T>()(first, last, *first);
}


template <typename RandIt, typename Compare>
void insertion_sort(RandIt first, RandIt last, Compare&& comp) {
	using T = typename std::iterator_traits<RandIt>::value_type;
	insertion_sort_imp2<RandIt, T, Compare>()(first, last, comp, *first);
}


template <typename RandIt, typename T>
RandIt upper_bound(RandIt first, RandIt last, const T& val)
{
	ptrdiff_t len = last - first;
	ptrdiff_t half;
	RandIt middle;

	while (len > 0) {
		half = len >> 1;
		middle = first + half;
		
		if (val < *middle) {
			len = half;
		}
		else {
			first = middle;
			++first;
			len = len - half - 1;
		}
	}
	
	return first;
}


template <typename RandIt, typename T, typename Compare>
RandIt upper_bound(RandIt first, RandIt last, const T& val, Compare&& comp)
{
	ptrdiff_t len = last - first;
	ptrdiff_t half;
	RandIt middle;

	while (len > 0) {
		half = len >> 1;
		middle = first + half;
		
		if (comp(val, *middle)) {
			len = half;
		}
		else {
			first = middle;
			++first;
			len = len - half - 1;
		}
	}
	
	return first;
}


// template <typename RandIt>
// void final_insertion_sort(RandIt first, RandIt last) {
	// if (last - first > sort_threshold) {
		// insertion_sort(first, first + sort_threshold);
		// unguarded_insertion_sort(first + sort_threshold, last);
	// }
	// else {
		// insertion_sort(first, last);
	// }
// }


template <typename RandIt, typename T>
RandIt unguarded_partition(RandIt first, RandIt last, T pivot) {
	using std::swap;
	
	while (true) {
		while (*first < pivot) ++first;
		
		--last;

		while (pivot < *last) --last;
		
		if (!(first < last)) return first;
		
		swap(*first, *last);
		
		++first;
	}
	
	return first;
}


template <typename RandIt, typename T, typename Compare>
RandIt unguarded_partition(RandIt first, RandIt last, T pivot, Compare&& comp) {
	using std::swap;
	
	while (true) {
		while (comp(*first, pivot)) ++first;
		
		--last;

		while (comp(pivot, *last)) --last;
		
		if (!(first < last)) return first;
		
		swap(*first, *last);
		
		++first;
	}
	
	return first;
}


template <typename RandIt, typename Predicate>
RandIt partition(RandIt first, RandIt last, Predicate&& pred) {
	using std::swap;
	
	while (true) {
		while (first != last && pred(*first)) ++first;

		while (first != last && !pred(*--last)) { }
		
		if (first == last) return first;
		
		swap(*first, *last);
		
		++first;
	}
	
	return first;
}


template <typename T>
const T& median(const T& a, const T& b, const T& c) {
	if (a < b) {
		if (b < c) return b;
		if (a < c) return c;
		return a;
	}
	
	if (a < c) return a;
	if (b < c) return c;
	return b;
}


template <typename T, typename Compare>
const T& median(const T& a, const T& b, const T& c, Compare&& comp) {
	if (comp(a, b)) {
		if (comp(b, c)) return b;
		if (comp(a, c)) return c;
		return a;
	}
	
	if (comp(a, c)) return a;
	if (comp(b, c)) return c;
	return b;
}


template <typename RandIt>
void partial_sort(RandIt first, RandIt middle, RandIt last);

template <typename RandIt, typename Compare>
void partial_sort(RandIt first, RandIt middle, RandIt last, Compare comp);


template <typename RandIt>
void sort_imp(RandIt first, RandIt last, int max_recursion)
{
	while (last - first > sort_threshold) {
		if (max_recursion == 0) {
			return azp::partial_sort(first, last, last);
		}
		
		max_recursion--;
		
		auto& pivot = median(*first, *(first + (last - first) / 2), *(last-1));

		auto ppoint = azp::unguarded_partition(first, last, pivot);
		
		azp::sort_imp(ppoint, last, max_recursion);
		last = ppoint;
	}
}


template <typename RandIt, typename Compare>
void sort_imp(RandIt first, RandIt last, Compare&& comp, int max_recursion)
{
	while (last - first > sort_threshold) {
		if (max_recursion == 0) {
			return azp::partial_sort(first, last, last);
		}
		
		max_recursion--;
		
		auto& pivot = median(*first, *(first + (last - first) / 2), *(last-1), comp);

		auto ppoint = azp::unguarded_partition(first, last, pivot, comp);
		
		azp::sort_imp(ppoint, last, comp, max_recursion);
		last = ppoint;
	}
}


// precondition n >= 1
inline int log2(ptrdiff_t n) {
	int k = 0;
	while (n != 1) {
		++k;
		n >>= 1;
	}
	return k;
}


template <typename RandIt>
void sort(RandIt first, RandIt last) {
	auto diff = last - first;
	if (diff > 1) {
		azp::sort_imp(first, last, log2(diff) * 2);
		azp::insertion_sort(first, last);
	}
}


template <typename RandIt, typename Compare>
void sort(RandIt first, RandIt last, Compare&& comp) {
	auto diff = last - first;
	if (diff > 1) {
		azp::sort_imp(first, last, comp, log2(diff) * 2);
		azp::insertion_sort(first, last, comp);
	}
}


template <typename RandIt, typename T>
void push_heap(RandIt first, ptrdiff_t holeIndex, ptrdiff_t topIndex, T value)
{
	ptrdiff_t parent = (holeIndex - 1) / 2;
	
	while (holeIndex > topIndex && *(first + parent) < value)
	{
		*(first + holeIndex) = std::move(*(first + parent));
		holeIndex = parent;
		parent = (holeIndex - 1) / 2;
	}
	
	*(first + holeIndex) = std::move(value);
}


template <typename RandIt, typename T, typename Compare>
void push_heap(RandIt first, ptrdiff_t holeIndex, ptrdiff_t topIndex, T value, Compare comp)
{
	ptrdiff_t parent = (holeIndex - 1) / 2;
	
	while (holeIndex > topIndex && comp(first[parent], value))
	{
		first[holeIndex] = std::move(first[parent]);
		holeIndex = parent;
		parent = (holeIndex - 1) / 2;
	}
	
	first[holeIndex] = std::move(value);
}


template <typename RandIt, typename T>
void adjust_heap(RandIt first, ptrdiff_t holeIndex, ptrdiff_t len, T value)
{
	ptrdiff_t topIndex = holeIndex;
	ptrdiff_t secondChild = 2 * holeIndex + 2;
	
	while (secondChild < len) {
		if (*(first + secondChild) < *(first + (secondChild - 1)))
			secondChild--;
		
		*(first + holeIndex) = std::move(*(first + secondChild));
		
		holeIndex = secondChild;
		secondChild = 2 * (secondChild + 1);
	}
	
	if (secondChild == len) {
		*(first + holeIndex) = std::move(*(first + (secondChild - 1)));
		holeIndex = secondChild - 1;
	}
	
	azp::push_heap(first, holeIndex, topIndex, value);
}


template <typename RandIt, typename T>
void pop_heap(RandIt first, RandIt last, RandIt result, T value)
{
	*result = std::move(*first);
	azp::adjust_heap(first, ptrdiff_t(0), ptrdiff_t(last - first), value);
}


template <typename RandIt>
void pop_heap(RandIt first, RandIt last)
{
	azp::pop_heap(first, last - 1, last - 1, *(last - 1));
}


template <typename RandIt, typename T, typename Compare>
void adjust_heap(RandIt first, ptrdiff_t holeIndex, ptrdiff_t len, T value, Compare comp)
{
	ptrdiff_t topIndex = holeIndex;
	ptrdiff_t secondChild = 2 * holeIndex + 2;
	
	while (secondChild < len) {
		if (comp(first[secondChild], first[secondChild - 1]))
			secondChild--;
		
		first[holeIndex] = std::move(first[secondChild]);
		
		holeIndex = secondChild;
		secondChild = 2 * (secondChild + 1);
	}
	
	if (secondChild == len) {
		first[holeIndex] = std::move(first[secondChild - 1]);
		holeIndex = secondChild - 1;
	}
	
	azp::push_heap(first, holeIndex, topIndex, value, comp);
}


template <typename RandIt, typename T, typename Compare>
void pop_heap(RandIt first, RandIt last, RandIt result, T value, Compare comp)
{
	*result = std::move(*first);
	azp::adjust_heap(first, ptrdiff_t(0), ptrdiff_t(last - first), value, comp);
}


template <typename RandIt, typename Compare>
void pop_heap(RandIt first, RandIt last, Compare comp)
{
	azp::pop_heap(first, last - 1, last - 1, std::move(*(last - 1)), comp);
}


template <typename RandIt>
void make_heap(RandIt first, RandIt last)
{
	if (last - first < 2) return;
	
	ptrdiff_t len = last - first;
	ptrdiff_t parent = (len - 2) / 2;

	while (true) {
		azp::adjust_heap(first, parent, len, *(first + parent));
		if (parent == 0) return;
		parent--;
	}
}


template <typename RandIt, typename Compare>
void make_heap(RandIt first, RandIt last, Compare comp)
{
	if (last - first < 2) return;
	
	ptrdiff_t len = last - first;
	ptrdiff_t parent = (len - 2) / 2;

	while (true) {
		azp::adjust_heap(first, parent, len, std::move(first[parent]), comp);
		if (parent == 0) return;
		parent--;
	}
}


template <typename RandIt>
void sort_heap(RandIt first, RandIt last)
{
	while (last - first > 1)
		azp::pop_heap(first, last--);
}


template <typename RandIt, typename Compare>
void sort_heap(RandIt first, RandIt last, Compare comp)
{
	while (last - first > 1)
		azp::pop_heap(first, last--, comp);
}


template <typename RandIt>
void partial_sort(RandIt first, RandIt middle, RandIt last) {
	azp::make_heap(first, middle);
	
	for (RandIt i = middle; i < last; ++i)
		if (*i < *first)
			azp::pop_heap(first, middle, i, *i);
		
	azp::sort_heap(first, middle);
}


template <typename RandIt, typename Compare>
void partial_sort(RandIt first, RandIt middle, RandIt last, Compare comp) {
	azp::make_heap(first, middle, comp);
	
	for (RandIt i = middle; i < last; ++i)
		if (comp(*i, *first))
			azp::pop_heap(first, middle, i, std::move(*i), comp);
		
	azp::sort_heap(first, middle, comp);
}

} // namespace azp