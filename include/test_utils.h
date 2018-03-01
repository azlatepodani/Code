#pragma once

#include <vector>
#include <random>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ratio>
#include <algorithm>


template <typename T>
void CheckSorted(const std::vector<T>& vec);


namespace azp {

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


inline 
void gen_random_string_array(int n, int min_len, int max_len,
							 std::vector<char *>& vec,
							 std::mt19937& g)
{
	auto x = std::uniform_int_distribution<int>(32,127);
	auto y = std::uniform_int_distribution<int>(min_len, max_len);
	
	for (int i=0; i<n; ++i) {
		int len = y(g);
		char * s = new char[len];
		for (int j=0; j<len; ++j) {
			s[j] = (char)x(g);
		}
		vec.emplace_back(std::move(s));
	}
}


inline 
void gen_random_string_array(int n, int min_len, int max_len,
							 std::vector<wchar_t *>& vec,
							 std::mt19937& g)
{
	auto x = std::uniform_int_distribution<int>(32,0xD7FF);
	auto y = std::uniform_int_distribution<int>(min_len, max_len);
	
	for (int i=0; i<n; ++i) {
		int len = y(g);
		wchar_t * s = new wchar_t[len];
		for (int j=0; j<len; ++j) {
			s[j] = (wchar_t)x(g);
		}
		vec.emplace_back(std::move(s));
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


template <typename T, typename Fn>
void benchmark(const char * desc, std::vector<T>& vec, std::mt19937& g, Fn alg)
{
	int size = vec.size();
	long long time = 0x7FFFFFFFFFFFFFFLL;
	 
	std::shuffle(vec.begin(), vec.begin()+size, g);
	std::vector<T> backup(vec.begin(), vec.begin()+size);
	alg(&vec[0], &vec[0]+size);
	
	CheckSorted(vec);

	int i=0;
	int maxi = 15;
	for (; i<maxi; ++i) {
		vec = std::vector<T>(backup.begin(), backup.begin()+size);
		auto start = std::chrono::steady_clock::now();

		alg(&vec[0], &vec[0]+size);

		auto end = std::chrono::steady_clock::now();
		auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start);
		
		if (time > diff.count()) time = diff.count();
	}

	time /= 1000;
	printf("%-10s %10d %7dus\n", desc, size, (int)time);
}


template <typename Fn>
void benchmark(const char * desc, Fn alg)
{
	int i=0;
	int maxi = 1500;
	
	auto start = std::chrono::steady_clock::now();
	
	for (; i<maxi/2; ++i) {
		alg();
	}
	
	auto end = std::chrono::steady_clock::now();
	auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() / i;
	
	start = std::chrono::steady_clock::now();
	
	for (; i<maxi; ++i) {
		alg();
	}
	
	end = std::chrono::steady_clock::now();
	auto diff2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() / (maxi - maxi/2);
	
	if (diff > diff2) diff = diff2;
	
	printf("%-15s  time=%dus\n", desc, (int)(diff/1000));
}



template<typename T>
void print(const T& l, const T& r) {
	std::cout << l << "\n<<<<<<<<<<<\n" << r << "\n\n";
}

template<>
void print<std::string>(const std::string& l, const std::string& r) {
	std::cout << l.c_str() << L"\n<<<<<<<<<<<\n" << r.c_str() << L"\n\n";
}

template<>
void print<std::wstring>(const std::wstring& l, const std::wstring& r) {
	std::wcout << l.c_str() << L"\n<<<<<<<<<<<\n" << r.c_str() << L"\n\n";
}

template<>
void print<const wchar_t*>(const wchar_t* const& l, const wchar_t* const& r) {
	std::wcout << l << L"\n<<<<<<<<<<<\n" << r << L"\n\n";
}



#if defined(_MSC_VER)
	inline std::string loadFile(const wchar_t * path) {
		std::string str;
		auto h = CreateFileW(path, GENERIC_READ, 0,0, OPEN_EXISTING, 0,0);
		if (h == INVALID_HANDLE_VALUE) {
			std::cout << "cannot open file  " << GetLastError() << '\n';
			return std::string();
		}
		auto size = GetFileSize(h, 0);
		str.resize(size);
		if (!ReadFile(h, &str[0], (ULONG)str.size(), 0,0)) {
			std::cout << "cannot read file  " << GetLastError() << '\n';
		}
		CloseHandle(h);
		return str;
	}

#else
	inline std::string loadFile(const char * path) {
		std::string str;
		std::ifstream stm(path, std::ios::binary);
		
		if (!stm.good()) {
			std::cout << "cannot open file\n";
			return std::string();
		}
		
		stm.seekg(0, std::ios_base::end);
		auto size = stm.tellg();
		stm.seekg(0, std::ios_base::beg);
		
		str.resize(size);
		stm.read(&str[0], size);
		return str;
	}
#endif // _MSC_VER

}