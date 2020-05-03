// polymorphic.hpp
// polymorphic type support
// (C)2009,2015,2018 Kenneth Boyd, license: MIT.txt

#ifndef ZAIMONI_STL_POLYMORPHIC_HPP
#define ZAIMONI_STL_POLYMORPHIC_HPP 1

#include <typeinfo>			// to properly support CopyInto (MingWin).  This requires RTTI.
#include <type_traits>

namespace zaimoni	{

	// https://devblogs.microsoft.com/oldnewthing/20200413-00/?p=103669 [Raymond Chen/"The Old New Thing"]
	template<typename T, typename...>
	using unconditional_t = T;

	template<typename T, T v, typename...>
	inline constexpr T unconditional_v = v;

// polymorphic objects should not use assignment unless they are "final"
// they should resort to the CopyInto member function instead (don't override these without a CopyInto)
// may be other things that should react, so just notate
// there is a boost::is_polymorphic
template<typename T>
struct is_polymorphic_base : public std::false_type
{
};

template<typename T>
struct is_polymorphic_base<const T> : public is_polymorphic_base<T>
{
};

template<typename T>
struct is_polymorphic_base<volatile T> : public is_polymorphic_base<T>
{
};

template<typename T>
struct is_polymorphic_final : public std::false_type
{
};

template<typename T>
struct is_polymorphic_final<const T> : public is_polymorphic_final<T>
{
};

template<typename T>
struct is_polymorphic_final<volatile T> : public is_polymorphic_final<T>
{
};

// yes, there is a boost::is_polymorphic.
template<typename T>
struct is_polymorphic : public std::integral_constant<bool, is_polymorphic_base<T>::value || is_polymorphic_final<T>::value>
{
};

template<typename T>
struct param : public std::conditional<sizeof(T)<=sizeof(unsigned long long) && std::is_trivially_copy_assignable<T>::value, T , const T&>
{
};

// objects with const or reference members do not have a valid assignment operator, but still can be copy-constructed.
// override this by deriving from boost::true_type
// * above caveat is not automatically true for subclasses
// * C++ STL hates this.
template<typename T>
struct has_invalid_assignment_but_copyconstructable : public std::false_type
{
};

// class has MoveInto member function, and is *not* polymorphic-base
// intended semantics: dest has former value of src, src has no content (may or may not be valid other than for destruction)
template<typename T>
struct has_MoveInto : public std::false_type
{
};

// don't override this one
template<typename T>
struct has_MoveInto<const T> : public std::false_type
{
};

template<typename T>
struct has_MoveInto<volatile T> : public has_MoveInto<T>
{
};

// polymorphic_base<T> types should rely on CopyInto member functions
template<typename T, typename U>
std::enable_if_t<std::is_base_of_v<U, T>, void>
CopyInto(const T& src, U*& dest)
{
	if constexpr (std::is_copy_constructible_v<T>) {
		if (!dest) {
			dest = new T(src);
			return;
		}
	};
	if constexpr (std::is_copy_assignable_v<T>) {
		if constexpr (std::is_same_v<T, U>) {
			*static_cast<T*>(dest) = src;
			return;
		} else if (typeid(*dest) == typeid(src) && dest) {
			if constexpr (std::is_copy_constructible_v<T>) {
				*static_cast<T*>(dest) = src;
				return;
			} else if (dest) {
				*static_cast<T*>(dest) = src;
				return;
			}
		}
	}
	if constexpr (std::is_copy_constructible_v<T>) {
		dest = new T(src);
		return;
	} else if constexpr (is_polymorphic_base<U>::value) {
		src.CopyInto(dest);
	} else {
		static_assert(unconditional_v<bool, false, T>, "unclear how to copy type");
	}
}

// following recurses to a class member function MoveInto
// intended semantics: dest has former value of src, src is only good for destruction
template<typename T,typename U>
void
MoveInto(T& src, U*& dest)
{	//! \todo should not be considered if T* is not convertible to U*
	if (dest && typeid(src)!=typeid(*dest))
		{
		delete dest;
		dest = 0;
		}
	src.MoveInto(reinterpret_cast<T*&>(dest));
}

}	// end namespace zaimoni

#endif
