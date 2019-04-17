#pragma once
#include <exception>
#include <algorithm>
#include <string.h>
#include <string>
#include <type_traits>
#include "azp_allocator.h"

namespace azp {


struct string_view_t {
	const char* str;	// not necessarily 0-terminated
	size_t len;
};
	
	
	
template <typename Allocator=default_alloc_t>
struct string {
	char* _start;
	char* _end;
	union {
		char  _buf[16];
		char* _max;
	} u;
	Allocator& _a;
	
	// cppcheck-suppress noExplicitConstructor
	string(Allocator& a) noexcept
		: _start(u._buf), _end(_start), _a(a)
	{
		*_start = 0;
	}
	
	string(Allocator& a, size_t requested) : string(a) {
		reserve(requested);
	}
	
	string(string<Allocator>&& other) noexcept;
	string& operator=(string<Allocator>&& other) noexcept;
	
	string(const string<Allocator>& other);
	string& operator=(const string<Allocator>& other);
	
	~string();
	
	void move_to(std::string& str) noexcept {
		static_assert(std::is_same_v<Allocator, std::string::allocator_type>);
		auto& data = str._Get_data();
		data._Mysize = size();
		
		if (_start == u._buf) {
			data._Myres = sizeof(u._buf) - 1;
			memcpy(data._Bx._Buf, u._buf, sizeof(u._buf));
		}
		else {
			data._Bx._Ptr = _start;
			data._Myres = u._max - _start - 1;
			_start = u._buf;
			_end = u._buf;
		}
	}
	
	void reserve(size_t requested);
	void resize(size_t n);

	size_t capacity() const { return (_start == u._buf) ? sizeof(u._buf) : (u._max - _start); }
	size_t size() const { return _end - _start; }
	
	// extension
	void set_size(size_t s) noexcept {
		reserve(s+1);
		_end = _start + s;
		*_end = 0;
	}

	char* begin() { return _start; }
	const char* begin() const { return _start; }
	const char* cbegin() const { return _start; }

	char* end() { return _end; }
	const char* end() const { return _end; }
	const char* cend() const { return _end; }
	
	char& operator[](size_t pos) { return _start[pos];	}
	
	const char& operator[](size_t pos) const { return _start[pos];	}
	
	void push_back(char val);
	void pop_back() { --_end; }
	
	char& back() { return *(_end-1); }
	const char& back() const { return *(_end-1); }
	
	bool empty() const { return _start == _end; }
	const char* c_str() const { return _start; }
	
	void append(const string& str) { append(str.c_str(), str.size()); }
	void append(const string_view_t& s) { append(s.str, s.len); }
	void append(const char* s, size_t count);
	void append(const char* s);
	void append(const char* first, const char* last) { append(first, size_t(last-first)); }
};

template <typename Alloc1, typename Alloc2>
bool operator==(const string<Alloc1>& l, const string<Alloc2>& r)
{
	if (l.size() != r.size()) return false;
	return (memcmp(l.c_str(), r.c_str(), l.size()) == 0);
}

template <typename Alloc1, typename Alloc2>
bool operator!=(const string<Alloc1>& l, const string<Alloc2>& r)
{
	return !(l == r);
}

template <typename Alloc1, typename Alloc2>
bool operator<(const string<Alloc1>& l, const string<Alloc2>& r)
{
	return (strcmp(l.c_str(), r.c_str()) < 0);
}

template <typename Alloc1, typename Alloc2>
bool operator>(const string<Alloc1>& l, const string<Alloc2>& r)
{
	return r < l;
}

template <typename Alloc1, typename Alloc2>
bool operator<=(const string<Alloc1>& l, const string<Alloc2>& r)
{
	return !(r < l);
}

template <typename Alloc1, typename Alloc2>
bool operator>=(const string<Alloc1>& l, const string<Alloc2>& r)
{
	return !(l < r);
}


#ifndef __ROUND_UP__
inline size_t round_up(size_t s, size_t a) { return (s+a-1) & (~a); }
#define __ROUND_UP__
#endif


template <typename Allocator>
void string<Allocator>::reserve(size_t requested) {
	size_t cap = capacity();
	if (requested < cap) return;
	
	cap += cap / 2;
	if (requested < cap) requested = cap;
	
	//auto b = _a.alloc(round_up(requested, 16));
	block_t b = {0, round_up(requested, 16)};
	b.p = _a.allocate(b.size);
	if (!b.p) throw std::bad_alloc();
	
	auto len = size();
	memcpy(b.p, _start, len+1);
	
	if (_start != u._buf) {
		//_a.free({_start, cap});
		_a.deallocate(_start, capacity());
	}
	
	_start = (char*)b.p;
	_end = _start + len;
	u._max = _start + b.size;
}


template <typename Allocator>	
void string<Allocator>::resize(size_t n) {
	auto current = size();
	if (n > current) {
		reserve(n+1);
		n -= current;
		while (n--) {
			*++_end = 0;
		}
	}
	else {
		current -= n;
		_end -= current;
		*_end = 0;
	}
}


template <typename Allocator>
string<Allocator>::string(string<Allocator>&& other) noexcept
	: _a(other._a)
{
	if (other._start == other.u._buf) {
		_start = u._buf;
		u = other.u;
		_end = _start + other.size();
	}
	else {
		_start = other._start;
		_end = other._end;
		u._max = other.u._max;
		other._start = other.u._buf;
		other._end = other.u._buf;
	}
}


template <typename Allocator>
string<Allocator>& string<Allocator>::operator=(string<Allocator>&& other) noexcept
{
	if (_start != u._buf) //_a.free({_start, capacity()});
		_a.deallocate(_start, capacity());
	
	if (other._start == other.u._buf) {
		_start = u._buf;
		u = other.u;
		_end = _start + other.size();
	}
	else {
		_start = other._start;
		_end = other._end;
		u._max = other.u._max;
		other._start = other.u._buf;
		other._end = other.u._buf;
	}
	return *this;
}


template <typename Allocator>
string<Allocator>::string(const string<Allocator>& other)
	: string(other._a, other.size()+1)
{
	auto len = other.size();
	memcpy(_start, other._start, len+1);
	_end = _start + len;
}


template <typename Allocator>
string<Allocator>& string<Allocator>::operator=(const string<Allocator>& other)
{
	if (other.size() > capacity()) {
		string<Allocator> tmp(other);
		*this = std::move(tmp);
	}
	else {
		auto len = other.size();
		set_size(len);
		memcpy(_start, other_start, len);
	}
	
	return *this;
}


template <typename Allocator>
void string<Allocator>::push_back(char val)
{
	reserve(size()+2);
	*_end++ = val;
	*_end = 0;
}


template <typename Allocator>
string<Allocator>::~string() {
	if (_start == u._buf) return;
	//_a.free({_start, capacity()});
	_a.deallocate(_start, capacity());
}


template <typename Allocator>
void string<Allocator>::append(const char* s, size_t count) {
	reserve(size()+count+1);
	memcpy(_end, s, count);
	_end += count;
	*_end = 0;
}


template <typename Allocator>
void string<Allocator>::append(const char* s) {
	auto cap = capacity();
	auto n = cap - size();
	
	while (n && *s) {
		*_end++ = *s++;
		--n;
	}
	
	if (!n) {
		auto len = strlen(s) + 1;
		reserve(cap + len);
		memcpy(_end, s, len);
		_end += len - 1;
	}
}

	
} // namespace azp


