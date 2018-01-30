#pragma once
#include <exception>
#include <algorithm>
#include "azp_allocator.h"

namespace azp {
	
	
inline size_t round_up(size_t s, size_t a) { return (s+a-1) & (~a); }
	
template <typename T, typename Allocator=default_alloc_t>
struct vector {
	T* _start;
	T* _end;
	T* _max;
	Allocator& _a;
	
	vector(Allocator& a) noexcept : _start(0), _end(0), _max(0), _a(a) { }
	vector(Allocator& a, size_t requested) : vector(a) {
		reserve(requested);
	}
	
	vector(vector<T, Allocator>&& other) noexcept;
	vector& operator=(vector<T, Allocator>&& other) noexcept;
	
	vector(const vector<T, Allocator>& other);
	vector& operator=(const vector<T, Allocator>& other);
	
	~vector();
	
	void reserve(size_t requested);
	void resize(size_t size);

	size_t capacity() const { return _max - _start; }
	size_t size() const { return _end - _start; }
	
	void set_size(size_t s) noexcept { _end = _start + s; }	// extension

	T* begin() { return _start; }
	const T* begin() const { return _start; }
	const T* cbegin() const { return _start; }

	T* end() { return _end; }
	const T* end() const { return _end; }
	const T* cend() const { return _end; }
	
	T& operator[](size_t pos) { return _start[pos];	}
	
	const T& operator[](size_t pos) const { return _start[pos];	}
	
	void push_back(const T& val);
	void push_back(T&& val);
	
	void pop_back() { --_end; }
	
	T& back() { return *(_end-1); }
	const T& back() const { return *(_end-1); }
};

template <typename T, typename Allocator>
bool operator==(const vector<T, Allocator>& l, const vector<T, Allocator>& r)
{
	if (l.size() != r.size()) return false;
	for (auto it=l.begin(), itr = r.begin(); it != l.end(); ++it,++itr) {
		if (*it != *itr) return false;
	}
	return true;
}

template <typename T, typename Allocator>
void vector<T, Allocator>::reserve(size_t requested) {
	size_t cap = capacity();
	if (requested < cap) return;
	
	cap = cap * 3 / 2;
	if (requested < cap) requested = cap;
	
	auto b = _a.alloc(requested * sizeof(T));
	if (!b.p) throw std::bad_alloc();
	
	T* target = (T*)b.p;
	for (auto it=_start; it != _end; ++it) {
		new (target) T(std::move(*it));
		target++;
	}
	
	if (_start) {
		_a.free({_start, cap*sizeof(T)});
	}
	
	_start = (T*)b.p;
	_end = target;
	_max = _start + requested;
}


template <typename T, typename Allocator>	
void vector<T, Allocator>::resize(size_t n) {
	auto current = size();
	if (n > current) {
		reserve(n);
		n -= current;
		while (n--) {
			new (_end) T();
			_end++;
		}
	}
	else {
		current -= n;
		while (current--) {
			(--_end)->~T();
		}
	}
}


template <typename T, typename Allocator>
vector<T, Allocator>::vector(vector<T, Allocator>&& other) noexcept
	: _start(other._start)
	, _end(other._end)
	, _max(other._max)
	, _a(other._a)
{
	other._start = 0;
	other._end = 0;
	other._max = 0;
}


template <typename T, typename Allocator>
vector<T, Allocator>& vector<T, Allocator>::operator=(vector<T, Allocator>&& other) noexcept
{
	vector<T, Allocator> tmp(std::move(other));
	std::swap(tmp._start, _start);
	std::swap(tmp._end, _end);
	std::swap(tmp._max, _max);
	return *this;
}


template <typename T, typename Allocator>
vector<T, Allocator>::vector(const vector<T, Allocator>& other)
	: vector(other._a, other.size())
{
	try {
		for (auto it=other._start; it != other._end; ++it) {
			new (_end) T(*it);
			_end++;
		}
	} catch(...) {
		this->~vector();
		throw;
	}
}


template <typename T, typename Allocator>
vector<T, Allocator>& vector<T, Allocator>::operator=(const vector<T, Allocator>& other)
{
	if (other.size() > capacity()) {
		vector<T, Allocator> tmp(other);
		std::swap(tmp, *this);
	}
	else {
		resize(other.size());
		std::copy(other._start, other._end, _start);
	}
	
	return *this;
}

template <typename T, typename Allocator>
void vector<T, Allocator>::push_back(const T& val)
{
	push_back(std::move(T(val)));
}

template <typename T, typename Allocator>
void vector<T, Allocator>::push_back(T&& val)
{
	reserve(size()+1);
	new (_end) T(std::move(val));
	_end++;
}

template <typename T, typename Allocator>
vector<T, Allocator>::~vector() {
	if (!_start) return;
	
	for (auto it = _start; it != _end; ++it) {
		it->~T();
	}
	
	_a.free({_start, capacity() * sizeof(T)});
}

	
} // namespace azp


