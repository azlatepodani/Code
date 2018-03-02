
#if defined(_MSC_VER)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif // _MSC_VER

#undef max
#include <limits>
#include "../include/test_utils.h"
#include "benchmark/benchmark.h"
#include "ska/ska_sort.hpp"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "benchmark.lib")

using namespace azp;


std::mt19937 g(0xCC6699);


static void CustomArgumentsInt(benchmark::internal::Benchmark* b) {
	int size = 100;
	for (int i = 0; i <= 7; ++i) {
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

template <typename T>
void destroy(std::vector<T*>& vec)
{
	for (auto ptr : vec) {
		delete[] ptr;
	}
}

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


using UI8Fix = TestFixtureInt<uint8_t>;
BENCHMARK_DEFINE_F(UI8Fix, Obj)(benchmark::State& state)
{
	for (auto _ : state) {
		ska_sort(&vec[0], &vec[0]+vec.size());
	}
}
BENCHMARK_REGISTER_F(UI8Fix, Obj)->Apply(CustomArgumentsInt)->Unit(benchmark::kMicrosecond);

using I8Fix = TestFixtureInt<int8_t>;
BENCHMARK_DEFINE_F(I8Fix, Obj)(benchmark::State& state)
{
	for (auto _ : state) {
		ska_sort(&vec[0], &vec[0]+vec.size());
	}
}
BENCHMARK_REGISTER_F(I8Fix, Obj)->Apply(CustomArgumentsInt)->Unit(benchmark::kMicrosecond);

using UI16Fix = TestFixtureInt<uint16_t>;
BENCHMARK_DEFINE_F(UI16Fix, Obj)(benchmark::State& state)
{
	for (auto _ : state) {
		ska_sort(&vec[0], &vec[0]+vec.size());
	}
}
BENCHMARK_REGISTER_F(UI16Fix, Obj)->Apply(CustomArgumentsInt)->Unit(benchmark::kMicrosecond);

using I16Fix = TestFixtureInt<int16_t>;
BENCHMARK_DEFINE_F(I16Fix, Obj)(benchmark::State& state)
{
	for (auto _ : state) {
		ska_sort(&vec[0], &vec[0]+vec.size());
	}
}
BENCHMARK_REGISTER_F(I16Fix, Obj)->Apply(CustomArgumentsInt)->Unit(benchmark::kMicrosecond);

using UI32Fix = TestFixtureInt<uint32_t>;
BENCHMARK_DEFINE_F(UI32Fix, Obj)(benchmark::State& state)
{
	for (auto _ : state) {
		ska_sort(&vec[0], &vec[0]+vec.size());
	}
}
BENCHMARK_REGISTER_F(UI32Fix, Obj)->Apply(CustomArgumentsInt)->Unit(benchmark::kMicrosecond);

using I32Fix = TestFixtureInt<int32_t>;
BENCHMARK_DEFINE_F(I32Fix, Obj)(benchmark::State& state)
{
	for (auto _ : state) {
		ska_sort(&vec[0], &vec[0]+vec.size());
	}
}
BENCHMARK_REGISTER_F(I32Fix, Obj)->Apply(CustomArgumentsInt)->Unit(benchmark::kMicrosecond);

using SSFix = TestFixtureStr<std::string>;
BENCHMARK_DEFINE_F(SSFix, Obj)(benchmark::State& state)
{
	for (auto _ : state) {
		ska_sort(&vec[0], &vec[0]+vec.size());
	}
}
BENCHMARK_REGISTER_F(SSFix, Obj)->Apply(CustomArgumentsStr)->Unit(benchmark::kMicrosecond);

using CSFix = TestFixtureStr<char*>;
BENCHMARK_DEFINE_F(CSFix, Obj)(benchmark::State& state)
{
	for (auto _ : state) {
		ska_sort(&vec[0], &vec[0]+vec.size());
	}
}
BENCHMARK_REGISTER_F(CSFix, Obj)->Apply(CustomArgumentsStr)->Unit(benchmark::kMicrosecond);

using SwSFix = TestFixtureStr<std::wstring>;
BENCHMARK_DEFINE_F(SwSFix, Obj)(benchmark::State& state)
{
	for (auto _ : state) {
		ska_sort(&vec[0], &vec[0]+vec.size());
	}
}
BENCHMARK_REGISTER_F(SwSFix, Obj)->Apply(CustomArgumentsStr)->Unit(benchmark::kMicrosecond);

using CwSFix = TestFixtureStr<wchar_t*>;
BENCHMARK_DEFINE_F(CwSFix, Obj)(benchmark::State& state)
{
	for (auto _ : state) {
		ska_sort(&vec[0], &vec[0]+vec.size());
	}
}
BENCHMARK_REGISTER_F(CwSFix, Obj)->Apply(CustomArgumentsStr)->Unit(benchmark::kMicrosecond);


BENCHMARK_MAIN();


