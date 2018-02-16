#pragma once
#include <utility>
#include <iterator>


namespace azp {


constexpr int sort_threshold = 16;


template <typename RandIt>
void rotate_last(RandIt last, RandIt pos) {
	using std::swap;
	using T = std::iterator_traits<RandIt>::value_type;
	
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
	using T = std::iterator_traits<RandIt>::value_type;
	
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
	using T = std::iterator_traits<RandIt>::value_type;
	
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
	using T = std::iterator_traits<RandIt>::value_type;
	insertion_sort_imp<RandIt, T>()(first, last, *first);
}


template <typename RandIt, typename Compare>
void insertion_sort(RandIt first, RandIt last, Compare&& comp) {
	using T = std::iterator_traits<RandIt>::value_type;
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
		while (*first < pivot) {
			++first;
		}
		
		--last;

		while (pivot < *last) {
			--last;
		}
		
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
		while (comp(*first, pivot)) {
			++first;
		}
		
		--last;

		while (comp(pivot, *last)) {
			--last;
		}
		
		if (!(first < last)) return first;
		
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
void sort_imp(RandIt first, RandIt last, int max_recursion)
{
	while (last - first > sort_threshold) {
		if (max_recursion == 0) {
			return std::partial_sort(first, last, last);
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
			return std::partial_sort(first, last, last);
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
	if (diff) {
		azp::sort_imp(first, last, log2(diff) * 2);
		azp::insertion_sort(first, last);
	}
}


template <typename RandIt, typename Compare>
void sort(RandIt first, RandIt last, Compare&& comp) {
	auto diff = last - first;
	if (diff) {
		azp::sort_imp(first, last, comp, log2(diff) * 2);
		azp::insertion_sort(first, last, comp);
	}
}


} // namespace azp