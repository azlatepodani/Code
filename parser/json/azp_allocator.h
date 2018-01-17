#pragma once

namespace azp {


struct block_t {
	void* p;
	size_t size;
};

template <typename Primary, typename Fallback>
struct fallback_alloc_t {
	
	block_t alloc(size_t n) {
		auto b = _p.alloc(n);
		if (b.p) return b;
		return _f.alloc(n);
	}
	
	void free(block_t b) {
		if (_p.owns(b)) {
			_p.free(b);
		}
		else {
			_f.free(b);
		}
	}
	
	bool owns(block_t b) {
		return _p.owns(b) || _f.owns(b);
	}
	
	void free_all() {
		_p.free_all();
		_f.free_all();
	}
	
	Primary _p;
	Fallback _f;
};	

inline void * advance(void * p, size_t n) { return (char*)p + n; }
	
struct monotonic_alloc_t {
	monotonic_alloc_t(void* buffer, size_t size) : _start(buffer), _buffer(buffer), _size(size) { }
	
	block_t alloc(size_t n) {
		if (n <= _size) {
			block_t b{_buffer, n};
			_buffer = advance(_buffer, n);
			_size -= n;
		}
	}
	
	void free(block_t b) {
		// only the previously allocated buffer can be freed
		if (advance(b.p, b.size) == _buffer) {
			_buffer = b.p;
			_size -= b.size;
		}
	}
	
	bool owns(block_t b) {
		return (b.p >= _start) && ((char*)b.p + b.size <= _buffer);
	}
	
	void free_all() { }
	
	const void* _start;
	void* _buffer;
	size_t _size;
};


struct default_alloc_t {
	block_t alloc(size_t n) {
		return block_t{ ::malloc(n), n };
	}
	
	void free(block_t b) {
		::free(b.p);
	}
	
	bool owns(block_t b) {
		return true;
	}
};


template <typename Allocator, size_t size>
struct freelist_alloc_t {
	
	struct node_t { struct node_t* next; };	
	
	bool owns(block_t b) { return b.size == size || _parent.owns(b); }
	
	block_t alloc(size_t n) {
		if (n == size && _head) {
			block_t b = {_head, size};
			_head = _head.next;
			return b;
		}
		
		return _parent.alloc(n);
	}
	
	void free(block_t b) {
		if (b.size == size) {
			node_t* node = (node_t*)b.ptr;
			node->next = _head;
			_head = node;
		}
		else {
			_parent.free(b);
		}
	}
	
	void free_all() {
		while (_head) {
			void* ptr = _head;
			_head = _head->next;
			_parent.free(block_t{ptr, size});
		}
	}
	
	~freelist_alloc_t() {
		free_all();
	}
	
	Allocator _parent;
	node_t* _head = nullptr;
};

template <size_t size, typename Small, typename Large>
struct segregator_t {
};


} // namespace azp