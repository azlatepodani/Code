#include <atomic>
#include <mutex>


template <typename F, typename G>
struct CallbackManager {
	CallbackManager(F f, G g)
		: _f(std::move(f)), _g(std::move(g))
	{
		_valid.store(true, std::memory_order_release);
	}

	void call_f() {
		if (!_valid.load(std::memory_order_acquire)) return;
		_f("Foo()\n");
	}
	
	void call_g() {
		if (!_valid.load(std::memory_order_acquire)) return;
		_g("Bar()\n");
	}
	
	void invalidate() {
		_valid.store(false, std::memory_order_relaxed);
		// after this point no new calls will be made. Any inflight calls are still running
		printf("flag set\n");
	}
	
	F _f;
	G _g;
	
	std::atomic_bool _valid;
};


extern "C" __declspec(dllimport) void _stdcall Sleep(unsigned long);
auto p_sleep = [](unsigned long milis) { ::Sleep(milis); };


template <typename F, typename G>
struct CallbackManager2 {
	CallbackManager2(F f, G g)
		: _f(std::move(f)), _g(std::move(g)), _count(0)
	{
		_valid.store(true, std::memory_order_release);
	}

	void call_f() {
		if (!_valid.load(std::memory_order_acquire)) return;
		
		bump_count bc(*this);
		if (!_valid.load(std::memory_order_relaxed)) return;
		
		_f("Foo2()\n");
	}
	
	void call_g() {
		if (!_valid.load(std::memory_order_acquire)) return;
		
		bump_count bc(*this);
		if (!_valid.load(std::memory_order_relaxed)) return;
		
		_g("Bar2()\n");
	}
	
	void invalidate() {
		_valid.store(false, std::memory_order_relaxed);
		// after this point no new calls will be made. Wait for any inflight calls
		
		int loops = 0;
		while (_count.load(std::memory_order_relaxed))
		{
			loops++;
			_mm_pause();
			
			if (loops == 100) {
				loops = 0;
				p_sleep(1);	// the callbacks run for longer time. Back-off a bit.
			}
		}
		
		printf("_count==0\n");
	}
	
	F _f;
	G _g;
	
	std::atomic_bool _valid;
	std::atomic_long _count;	// may get a lot of contention
	
	struct bump_count {
		bump_count(CallbackManager2& cm)
			: _cm(cm)
		{
			_cm._count.fetch_add(1, std::memory_order_relaxed);
		}
		
		~bump_count() {
			_cm._count.fetch_sub(1, std::memory_order_relaxed);
		}
		
		CallbackManager2& _cm;
	};
};


template <typename F, typename G>
struct CallbackManager3 {
	CallbackManager3(F f, G g)
		: _f(std::move(f)), _g(std::move(g))
	{
		_count = 0;
		_valid = true;
	}

	void call_f() {
		std::unique_lock<std::mutex> guard(_lock);
		if (!_valid) return;
		
		_count++;
		guard.unlock();
		
		try {
			_f("Foo2()\n");
		}
		catch (...) {
			guard.lock();
			_count--;
			throw;
		}
		
		guard.lock();
		_count--;
	}
	
	void call_g() {
		std::unique_lock<std::mutex> guard(_lock);
		if (!_valid) return;
		
		_count++;
		guard.unlock();
		
		try {
			_g("Bar2()\n");
		}
		catch (...) {
			guard.lock();
			_count--;
			throw;
		}
		
		guard.lock();
		_count--;
	}
	
	void invalidate() {
		std::unique_lock<std::mutex> guard(_lock);
		_valid = false;
		// after this point no new calls will be made. Wait for any inflight calls
		
		int loops = 0;
		while (_count)
		{
			guard.unlock();
			loops++;
			
			if (loops == 100) {
				loops = 0;
				p_sleep(1);	// the callbacks run for longer time. Back-off a bit.
			}
			
			guard.lock();
		}
		
		printf("_count==0\n");
	}
	
	F _f;
	G _g;
	
	std::mutex _lock;
	bool _valid;
	long _count;
};


