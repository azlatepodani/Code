#pragma once
#include <utility>
#include <iterator>


namespace azp {

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


template <typename RandIt>
void unguarded_insertion_sort(RandIt first, RandIt last) {
	for (; first != last; ++first) {
		unguarded_linear_insert(first);
	}
}


template <typename RandIt>
void insertion_sort(RandIt first, RandIt last) {
	if (first == last) return;
	for (RandIt i=first+1; i != last; ++i) {
		//linear_insert(first, i);
		if (*i < *first) {
			rotate_last(i, first);
		}
		else {
			unguarded_linear_insert(i);
		}
	}
}


constexpr int sort_threshold = 16;


template <typename RandIt>
void final_insertion_sort(RandIt first, RandIt last) {
	if (last - first > sort_threshold) {
		insertion_sort(first, first + sort_threshold);
		unguarded_insertion_sort(first + sort_threshold, last);
	}
	else {
		insertion_sort(first, last);
	}
}


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

}