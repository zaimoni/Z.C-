// polymorphic.hpp
// polymorphic type support
// (C)2009 Kenneth Boyd, license: MIT.txt

#ifndef ZAIMONI_STL_POLYMORPHIC_HPP
#define ZAIMONI_STL_POLYMORPHIC_HPP 1

#include <typeinfo>			// to properly support CopyInto (MingWin).  This requires RTTI.
#include "boost_core.hpp"

namespace zaimoni	{

// polymorphic objects should not use assignment unless they are "final"
// they should resort to the CopyInto member function instead (don't override these without a CopyInto)
// may be other things that should react, so just notate
// there is a boost::is_polymorphic
template<typename T>
struct is_polymorphic_base : public boost::false_type
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
struct is_polymorphic_final : public boost::false_type
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
struct is_polymorphic : public boost::type_traits::ice_or<is_polymorphic_base<T>::value,is_polymorphic_final<T>::value >
{
};

// objects with const or reference members do not have a valid assignment operator, but still can be copy-constructed.
// override this by deriving from boost::true_type
// * above caveat is not automatically true for subclasses
// * C++ STL hates this.
template<typename T>
struct has_invalid_assignment_but_copyconstructable : public boost::false_type
{
};

// class has MoveInto member function, and is *not* polymorphic-base
// intended semantics: dest has former value of src, src has no content (may or may not be valid other than for destruction)
template<typename T>
struct has_MoveInto : public boost::false_type
{
};

// don't override this one
template<typename T>
struct has_MoveInto<const T> : public boost::false_type
{
};

template<typename T>
struct has_MoveInto<volatile T> : public has_MoveInto<T>
{
};

// CopyInto family of functions
// all of these can throw std::bad_alloc
// polymorphic_base<T> types should rely on CopyInto member functions
template<typename T,typename U>
inline typename boost::enable_if<is_polymorphic_base<T>, void>::type
CopyInto(const T& src, U*& dest)
{	//! \todo should not be considered if T* is not convertible to U*
	src.CopyInto(dest);
}

template<typename T,typename U>
typename boost::enable_if<boost::type_traits::ice_and<!is_polymorphic_base<T>::value, has_invalid_assignment_but_copyconstructable<T>::value >, void>::type
CopyInto(const T& src, U*& dest)
{	//! \todo should not be considered if T* is not convertible to U*
	if (NULL!=dest)
		delete dest;
	dest = new T(src);
}

template<typename T,typename U>
typename boost::enable_if<boost::type_traits::ice_and<!is_polymorphic_base<T>::value, !has_invalid_assignment_but_copyconstructable<T>::value >, void>::type
CopyInto(const T& src, U*& dest)
{	//! \todo should not be considered if T* is not convertible to U*
	if (NULL==dest)
		dest = new T(src);
	else if (typeid(*dest)==typeid(src))
		{
		*static_cast<T*>(dest) = src;
		}
	else{
		delete dest;
		dest = new T(src);
		}
}

// following recurses to a class member function MoveInto
// intended semantics: dest has former value of src, src is only good for destruction
template<typename T,typename U>
void
MoveInto(T& src, U*& dest)
{	//! \todo should not be considered if T* is not convertible to U*
	if (NULL!=dest && typeid(src)!=typeid(*dest))
		{
		delete dest;
		dest = NULL;
		}
	src.MoveInto(reinterpret_cast<T*&>(dest));
}

}	// end namespace zaimoni

#endif
