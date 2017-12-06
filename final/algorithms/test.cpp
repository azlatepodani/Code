#include <iostream>
#include <sstream>
#include "selection.h"

using namespace std;
using namespace azp;

template <typename T, int K>
string write(array<T, K> a, T end) {
	stringstream ss;
	
	for (auto& e : a) {
		if (e != end) {
			ss << *e << ", ";
		}
		else {
			ss << -1 << ", ";
		}
	}
	
	return ss.str();
}

void main() {
	{
		int v[] = {0,1};
		auto it = top_element(begin(v), end(v));
		cout << "mfe one " << ( (it == end(v)) ? -1 : *it ) << '\n';
		
		it = majority_element(begin(v), end(v));
		cout << "mfe one " << ( (it == end(v)) ? -1 : *it ) << '\n';
	}
	{
		int v[] = {0,1,1};
		auto it = top_element(begin(v), end(v));
		cout << "mfe two " << ( (it == end(v)) ? -1 : *it ) << '\n';
		
		it = majority_element(begin(v), end(v));
		cout << "mfe two " << ( (it == end(v)) ? -1 : *it ) << '\n';
	}
	{
		int v[] = {0,1,1,2};
		auto it = top_element(begin(v), end(v));
		cout << "mfe three " << ( (it == end(v)) ? -1 : *it ) << '\n';
		
		it = majority_element(begin(v), end(v));
		cout << "mfe three " << ( (it == end(v)) ? -1 : *it ) << '\n';
	}
	
	cout << "============\n";
	
	{
		int v[] = {0};
		cout << "tk one " << write(top_k_elements<2>(begin(v), end(v)), end(v)).c_str() << '\n';
		cout << "ht one " << write(heavy_hitters<2>(begin(v), end(v)), end(v)).c_str() << '\n';
	}
	
	{
		int v[] = {0,1};
		cout << "tk two " << write(top_k_elements<2>(begin(v), end(v)), end(v)).c_str() << '\n';
		cout << "ht two " << write(heavy_hitters<2>(begin(v), end(v)), end(v)).c_str() << '\n';
	}
	
	{
		int v[] = {0,1,1};
		cout << "tk three " << write(top_k_elements<2>(begin(v), end(v)), end(v)).c_str() << '\n';
		cout << "ht three " << write(heavy_hitters<2>(begin(v), end(v)), end(v)).c_str() << '\n';
	}
	
	{
		int v[] = {0,1,1,2};
		cout << "tk four " << write(top_k_elements<2>(begin(v), end(v)), end(v)).c_str() << '\n';
		cout << "ht four " << write(heavy_hitters<2>(begin(v), end(v)), end(v)).c_str() << '\n';
	}
	
	{
		int v[] = {0,1,2,3,4,5};
		cout << "tk five " << write(top_k_elements<2>(begin(v), end(v)), end(v)).c_str() << '\n';
		cout << "ht five " << write(heavy_hitters<2>(begin(v), end(v)), end(v)).c_str() << '\n';
	}
	
	{
		int v[] = {4,1,2,1,4,1,6,1,2,1,10,2,2};
		cout << "tk six " << write(top_k_elements<4>(begin(v), end(v)), end(v)).c_str() << '\n';
		cout << "ht six " << write(heavy_hitters<4>(begin(v), end(v)), end(v)).c_str() << '\n';
	}
}
