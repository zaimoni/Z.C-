#ifndef CANONICAL_CACHE_HPP
#define CANONICAL_CACHE_HPP 1

#include <memory>
#include <vector>

namespace zaimoni {

template<class T>
class canonical_cache {
	std::vector<std::weak_ptr<T> > _cache;
public:
	canonical_cache() = default;
	canonical_cache(const canonical_cache& src) = delete;
	canonical_cache(canonical_cache&& src) = delete;
	canonical_cache& operator=(const canonical_cache& src) = delete;
	canonical_cache& operator=(canonical_cache&& src) = delete;
	~canonical_cache() = default;

	std::shared_ptr<T> track(const T& src) {
		auto ub = _cache.size();
		while (0 < ub) {
			auto test = _cache[--ub].lock();
			if (!test) {
				_cache.erase(_cache.begin() + ub);
				continue;
			}
			if (src == *test) return test;
		}
		std::shared_ptr<T> ret(new T(src));
		_cache.push_back(ret);
		return ret;
	}

	std::shared_ptr<T> track(T&& src) {
		auto ub = _cache.size();
		while (0 < ub) {
			auto test = _cache[--ub].lock();
			if (!test) {
				_cache.erase(_cache.begin() + ub);
				continue;
			}
			if (src == *test) return test;
		}
		std::shared_ptr<T> ret(new T(std::move(src)));
		_cache.push_back(ret);
		return ret;
	}

	static canonical_cache& get() {
		static canonical_cache ooao;
		return ooao;
	}
};

}	// namespace zaimoni

#endif
