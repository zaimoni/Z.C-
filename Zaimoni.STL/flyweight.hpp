// flyweight.hpp
// loosely inspired by boost::flyweight
// (C)2012 Kenneth Boyd, license: MIT.txt

#ifndef ZAIMONI_STL_FLYWEIGHT_HPP
#define ZAIMONI_STL_FLYWEIGHT_HPP 1

#include <stdlib.h>

#ifdef ZAIMONI_FORCE_ISO
#include <assert.h>
#else
#include "MetaRAM2.hpp"
#endif

namespace zaimoni {
namespace destruct_type {

enum {
	none = 0,
	array_delete_op,
	delete_op,
	free
};

}

namespace detail {

struct refcount_POD
{
	size_t count;
	void* x;

	void reference() {
		// if we saturate then we leak but that is better than dereferencing-invalid
		if (size_t(-1)>count) ++count;
	}
};

template<bool free_on_destruct>
class refcount : public refcount_POD
{
private:
	// break copy-construction and assignment
	refcount(const refcount& src) = delete;
	refcount(refcount&& src) = delete;
	void operator=(const refcount& src) = delete;
	void operator=(refcount&& src) = delete;
public:
	refcount(void* src) noexcept
	{
		count = 1;
		x = src;
	};

	~refcount() {}
};

template<>
inline refcount<true>::~refcount()
{
	free(x);
	x = NULL;	// to mitigate double-destruction issues
}

}	// end namespace detail

template<class T,int destruct_mode=zaimoni::destruct_type::free>
class flyweight
{
private:
	typedef zaimoni::detail::refcount<(zaimoni::destruct_type::free==destruct_mode)> ptr_type;
	ptr_type* ptr;
public:
	flyweight() : ptr(0)  {};
	explicit flyweight(T* src) : ptr(src ? new ptr_type((void*)(src)) : 0)  {};
	flyweight(const flyweight& src) : ptr(src.ptr) {
		if (ptr) ptr->reference();
	};
	~flyweight() {
		dereference_and_overwrite(NULL);
	};

	flyweight& operator=(const flyweight& src) {
		if (ptr==src.ptr) return *this;
		assert((!ptr || !ptr->x || (src.ptr && src.ptr->x && ptr->x != src.ptr->x)) && "double-destruction expected later");
		src.ptr->reference();
		dereference_and_overwrite(src.ptr);
		return *this;
	};
#if 0
	flyweight& operator=(T* src) {
		// we tolerate this even though it indicates problems
		if (ptr && ptr->x==src) return *this;
		dereference_and_overwrite(src ? new ptr_type((void*)(src)) : 0);
		return *this;
	};
#endif

#ifndef ZAIMONI_FORCE_ISO
	bool condense(flyweight& src) {
		if (ptr==src.ptr) return true;
		assert((!ptr || !ptr->x || (src.ptr && src.ptr->x && ptr->x == src.ptr->x)) && "double-destruction expected later");
		if (   zaimoni::SafeArraySize(reinterpret_cast<T*>(ptr->x))==zaimoni::SafeArraySize(reinterpret_cast<T*>(src.ptr->x))
			&& !memcmp(ptr->x,src.ptr->x,_msize(ptr->x)))
			{
			if (ptr->count>=src.ptr->count)
				{
				src.dereference_and_overwrite(ptr);
				}
			else{
				deference_and_ovewrite(src.ptr);
				}
			return true;
			}
		return false;
	}
#endif

	// support functions
	bool empty() const {return !ptr;};
	void clear() {dereference_and_overwrite(NULL);};

	// we do not want to expose the underlying ptr->x by reference
	// C APIs want copy
	T& operator*() {
		assert(ptr && ptr->x && "undefined behavior: NULL deference");
		assert(1==ptr->count && "non-const deference of flyweight shared object is disallowed");
		return *(reinterpret_cast<T*>(ptr->x));
	};
	const T& operator*() const {
		assert(ptr && ptr->x && "undefined behavior: NULL deference");
		return *(reinterpret_cast<T*>(ptr->x));
	};
	T& operator[](size_t i) {
		assert(ptr && ptr->x && "undefined behavior: NULL deference");
		assert(1==ptr->count && "non-const deference of flyweight shared object is disallowed");
#ifndef ZAIMONI_FORCE_ISO
		assert(ArraySize(ptr->x)>i);
#endif
		return reinterpret_cast<T*>(ptr->x)[i];
	};
	const T& operator[](size_t i) const {
		assert(ptr && ptr->x && "undefined behavior: NULL deference");
#ifndef ZAIMONI_FORCE_ISO
		assert(ArraySize(ptr->x)>i);
#endif
		return reinterpret_cast<T*>(ptr->x)[i];
	};
	T* c_array() {
		assert(ptr && "undefined behavior: NULL deference");
		return reinterpret_cast<T*>(ptr->x);
	}	
	const T* data() const {
		assert(ptr && "undefined behavior: NULL deference");
		return reinterpret_cast<const T*>(ptr->x);
	}	

private:
	void dereference_and_overwrite(ptr_type* src) {
		// if we saturate then we leak but that is better than dereferencing-invalid
		if (ptr && size_t(-1)>ptr->count && 0== --(ptr->count))
			{
			if (zaimoni::destruct_type::array_delete_op==destruct_mode) delete[] static_cast<T*>(ptr->x); 
			else if (zaimoni::destruct_type::delete_op==destruct_mode) delete static_cast<T*>(ptr->x);
			delete ptr;
			}
		ptr = src;				
	}
};

} // end namespace zaimoni

#endif
