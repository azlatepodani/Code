#include <algorithm>
#include "benchmark/benchmark.h"
#include <random>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "benchmark.lib")


using std::iter_swap;

/*
1. last - first + 1 >= 5; closed interval
2. operator< gives a weak order

a. *first will contain the first pivot
b. *last will contain the second pivot
Returns:
	true if the pivots are different
*/
template <typename RandIt>
bool two_of_five(RandIt first, RandIt last) {
	auto c1 = first;
	auto dist = last - first + 1;
	auto quarter = dist / 4;
	auto c2 = first + quarter;
	auto c3 = c2 + quarter; 
	auto c4 = c3 + quarter; 
	auto c5 = last;
	
	/*
	c1-------------c2-------------c3----
	c1-------------c3-------------c2----
	c2-------------c1-------------c3----
	c2-------------c3-------------c1----
	c3-------------c2-------------c1----
	c3-------------c1-------------c2----
	*/
	if (*c2 < *c1) {
		if (*c1 < *c3) { } // all good (*c1 is the median)
		else if (*c3 < *c2) iter_swap(c2, c1);
			 else iter_swap(c3, c1);
	}
	else {
		if (*c2 < *c3) iter_swap(c2, c1);
		else if (*c3 < *c1) { } // all good (*c1 is the median)
			 else iter_swap(c3, c1);
	}

	if (*c4 < *c5) {
		if (*c5 < *c3) { } // all good (*c5 is the median)
		else if (*c3 < *c4) iter_swap(c4, c5);
			 else iter_swap(c3, c5);
	}
	else {
		if (*c4 < *c3) iter_swap(c4, c5);
		else if (*c3 < *c5) { } // all good (*c5 is the median)
			 else iter_swap(c3, c5);
	}
	
	if (*c5 < *c1) iter_swap(c5, c1);
    
    return *c5 != *c1;
}


/*
1. [first, last] closed interval
2. operator< gives a weak order
3. *first will contain the first pivot
   *last will contain the second pivot
Returns:
	middle interval
*/
template <typename RandIt>
std::pair<RandIt, RandIt> three_way_partition(RandIt first, RandIt last) {
    using T = decltype(*first);
	T p1 = *first;
	T p2 = *last;
	
	auto lessPtr = first + 1;
	auto greaterPtr = last - 1;
    
    while (*lessPtr < p1) ++lessPtr;
    while (*greaterPtr > p2) --greaterPtr;
	
	for (auto cur = lessPtr; cur <= greaterPtr; ++cur) {
		if (*cur < p1) {
			iter_swap(cur, lessPtr);
			++lessPtr;
		}
		else if (*cur > p2) {
			iter_swap(cur, greaterPtr);
			--greaterPtr;
            
            while (*greaterPtr > p2 && cur < greaterPtr)
				--greaterPtr;

			if (*cur < p1) {
				iter_swap(cur, lessPtr);
				++lessPtr;
			}
		}
		else { }
	}
	
	iter_swap(first, lessPtr-1);
	iter_swap(last, greaterPtr+1);
	
	return std::make_pair(lessPtr-1, greaterPtr+1);
}


template <typename RandIt>
std::pair<RandIt, RandIt> remove_pivots(RandIt first, RandIt last) {
    using T = decltype(*first);
    T p1 = *first++;
    T p2 = *last--;
    
    auto less = first;
    auto greater = last;
    
    while (first != greater) {
        if (*first == p1) {
            iter_swap(less, first);
            ++less;
        }
        else if (*first == p2) {
            iter_swap(first, greater);
            --greater;
            
            if (*first == p1) {
                iter_swap(less, first);
                ++less;
            }
        }
    }
    
    return std::make_pair(less, greater);
}


template <typename RandIt>
void dpsort(RandIt first, RandIt last) {
    auto diff = last - first;
	if (diff >= 17) {
		bool differentPivots = two_of_five(first, last-1);
        
		auto mid = three_way_partition(first, last-1);
        
		dpsort(first, mid.first);
        
        // if ((mid.second - mid.first > diff - 13) && (*mid.first != *mid.second))
            // remove_pivots(mid.first, mid.second);
        
        if (differentPivots)
            dpsort(mid.first+1, mid.second); // skip the pivots since they are the min & max of the middle interval
        
		dpsort(mid.second+1, last);
	}
	else {
		std::sort(first, last);
	}
}


// int main() {
	// int v[] = {17,89,1,0,5,94,87,51,0,8,94,58,93,45,0,8,91,50,97,81,25};
	// dpsort(v, v+sizeof(v)/4);
	
// }



std::mt19937 g(0xCC6699);


static void CustomArgumentsInt(benchmark::internal::Benchmark* b) {
	int size = 100;
	for (int i = 0; i <= 10; ++i) {
		b->Arg(size);
		size *= 4;
	}
}

static void CustomArgumentsStr(benchmark::internal::Benchmark* b) {
	int size = 100;
	for (int i = 0; i <= 9; ++i) {
		b->Arg(size);
		size *= 2;
	}
}

template <typename U> struct other { typedef U type; };
template <> struct other<int8_t> { typedef int type; };
template <> struct other<uint8_t> { typedef int type; };
	
	
template <typename T>
void gen_random_int_array(int n, T min_v, T max_v,
							 std::vector<T>& vec,
							 std::mt19937& g)
{
	auto x = std::uniform_int_distribution<typename other<T>::type>(min_v, max_v);
	
	for (int i=0; i<n; ++i) {
		vec.emplace_back(x(g));
	}
}

inline
void gen_random_string_array(int n, int min_len, int max_len,
							 std::vector<std::wstring>& vec,
							 std::mt19937& g)
{
	auto x = std::uniform_int_distribution<int>(32,0xD7FF);
	auto y = std::uniform_int_distribution<int>(min_len, max_len);
	
	for (int i=0; i<n; ++i) {
		int len = y(g);
		std::wstring s;
		s.resize(len);
		for (int j=0; j<len; ++j) {
			s[j] = (wchar_t)x(g);
		}
		vec.emplace_back(std::move(s));
	}
}


inline 
void gen_random_string_array(int n, int min_len, int max_len,
							 std::vector<std::string>& vec,
							 std::mt19937& g)
{
	auto x = std::uniform_int_distribution<int>(32,127);
	auto y = std::uniform_int_distribution<int>(min_len, max_len);
	
	for (int i=0; i<n; ++i) {
		int len = y(g);
		std::string s;
		s.resize(len);
		for (int j=0; j<len; ++j) {
			s[j] = (char)x(g);
		}
		vec.emplace_back(std::move(s));
	}
}


template <class T>
class TestFixtureInt : public ::benchmark::Fixture {
public:
	void SetUp(const ::benchmark::State& st) {
		vec.reserve(st.range_x());

		gen_random_int_array<T>(st.range_x(), std::numeric_limits<T>::lowest(), std::numeric_limits<T>::max(), vec, g);
	}

	void TearDown(const ::benchmark::State&) {
	}

	static std::vector<T> vec;
};
template <class T> std::vector<T> TestFixtureInt<T>::vec;

template <typename T>
void destroy(std::vector<T>& ) { }

template <class T>
class TestFixtureStr : public ::benchmark::Fixture {
public:
	void SetUp(const ::benchmark::State& st) {
		vec.reserve(st.range_x());

		gen_random_string_array(st.range_x(), 2, 10240, vec, g);
	}

	void TearDown(const ::benchmark::State&) {
		destroy(vec);
		vec.clear();
	}

	static std::vector<T> vec;
};
template <class T> std::vector<T> TestFixtureStr<T>::vec;

using I32Fix = TestFixtureInt<int32_t>;
BENCHMARK_DEFINE_F(I32Fix, Obj)(benchmark::State& state)
{
	for (auto _ : state) {
		dpsort(&vec[0], &vec[0]+vec.size());
        //if (!std::is_sorted(vec.begin(), vec.end())) __debugbreak();
	}
}
BENCHMARK_REGISTER_F(I32Fix, Obj)->Apply(CustomArgumentsInt)->Unit(benchmark::kMicrosecond);


using SSFix = TestFixtureStr<std::string>;
BENCHMARK_DEFINE_F(SSFix, Obj)(benchmark::State& state)
{
	for (auto _ : state) {
		dpsort(&vec[0], &vec[0]+vec.size());
        //if (!std::is_sorted(vec.begin(), vec.end())) __debugbreak();
	}
}
BENCHMARK_REGISTER_F(SSFix, Obj)->Apply(CustomArgumentsStr)->Unit(benchmark::kMicrosecond);


using SwSFix = TestFixtureStr<std::wstring>;
BENCHMARK_DEFINE_F(SwSFix, Obj)(benchmark::State& state)
{
	for (auto _ : state) {
		dpsort(&vec[0], &vec[0]+vec.size());
        //if (!std::is_sorted(vec.begin(), vec.end())) __debugbreak();
	}
}
BENCHMARK_REGISTER_F(SwSFix, Obj)->Apply(CustomArgumentsStr)->Unit(benchmark::kMicrosecond);


BENCHMARK_MAIN();
