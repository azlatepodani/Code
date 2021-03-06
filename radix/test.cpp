
#if defined(_MSC_VER)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif // _MSC_VER

#include "../include/test_utils.h"
#include "radix_sort.h"



using namespace azp;



bool compare(unsigned a, unsigned b, int) { return a<b; }
bool compare(int a, int b, int) { return a<b; }
bool compare(float a, float b, int) { return a<b; }

template <typename T>
void CheckSorted(const std::vector<T>& vec) {
	for (auto l=vec.begin(), r=l+1; r != vec.end(); ++l,++r) {
		if (compare(*r, *l, 0)) {
			print(*l, *r);
		}
	}
}


int main() {
	 
#if defined(_MSC_VER)
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	if (GetThreadPriority(GetCurrentThread()) != THREAD_PRIORITY_HIGHEST) printf("Priority set failed\n");
	 
	if (!SetThreadAffinityMask(GetCurrentThread(), 1)) printf("Affinity set failed\n");
#endif

	std::mt19937 g(0xCC6699);

	printf("%-10s %-10s    time\n", "  type", "# elements");

	{
	std::vector<float> vec1;
	gen_random_float_array<float>(1500000, -1000.f, 1000.f, vec1, g);
	*(uint32_t*)&vec1[100] = 0x7f800000;	// + inf
	*(uint32_t*)&vec1[1000] = 0xff800000;	// - inf
	benchmark("v float", vec1, g, [](float* f, float* l){radix_sort(f,l);});
	}{
	std::vector<uint8_t> vec1;
	gen_random_int_array<uint8_t>(1500000, 0, 255, vec1, g);
	benchmark("v uint8", vec1, g, [](uint8_t* f, uint8_t* l){radix_sort(f,l);});
	}{
	std::vector<int8_t> vec2;
	gen_random_int_array<int8_t>(1500000, -128, 127, vec2, g);
	benchmark("v int8", vec2, g, [](int8_t* f, int8_t* l){radix_sort(f,l);});
	}{
	std::vector<uint16_t> vec3;
	gen_random_int_array<uint16_t>(1500000, 0, 0xFFFF, vec3, g);
	benchmark("v uint16", vec3, g, [](uint16_t* f, uint16_t* l){radix_sort(f,l);});
	}{
	std::vector<int16_t> vec4;
	gen_random_int_array<int16_t>(1500000, -32768, 0x7FFF, vec4, g);
	benchmark("v int16", vec4, g, [](int16_t* f, int16_t* l){radix_sort(f,l);});
	}{
	std::vector<uint32_t> vec7; 
	gen_random_int_array<uint32_t>(1500000, 0, 0xFFFFFFFF, vec7, g);
	benchmark("v uint32", vec7, g, [](uint32_t* f, uint32_t* l){radix_sort(f,l);});
	}{
	std::vector<int32_t> vec8;
	gen_random_int_array<int32_t>(1500000, 0x80000000, 0x7FFFFFFF, vec8, g);
	benchmark("v int32", vec8, g, [](int32_t* f, int32_t* l){radix_sort(f,l);});
	}{//*/
	std::vector<std::string> vec5;
	gen_random_string_array(50000, 2, 10240, vec5, g);
	benchmark("v string", vec5, g, [](std::string* f, std::string* l){radix_sort(f,l);});
	}{
	std::vector<char *> vec9;
	gen_random_string_array(50000, 2, 10240, vec9, g);
	benchmark("v char*", vec9, g, [](char** f, char** l){radix_sort(f,l);});
	}{
	std::vector<std::wstring> vec6;
	gen_random_string_array(50000, 2, 10240, vec6, g);
	benchmark("v wstring", vec6, g, [](std::wstring* f, std::wstring* l){radix_sort(f,l);});
	}{
	std::vector<wchar_t *> vec10;
	gen_random_string_array(50000, 2, 10240, vec10, g);
	benchmark("v wchar_t*", vec10, g, [](wchar_t** f, wchar_t** l){radix_sort(f,l);});
	}
	
	printf("\n");
	return 0;
}


