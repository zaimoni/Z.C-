// z_float.hpp
// (C)2011 Kenneth Boyd, license: MIT.txt
// header for long double emulation

#ifndef Z_FLOAT_HPP
#define Z_FLOAT_HPP 1

#define __STDC_LIMIT_MACROS 1
#define __STDC_CONSTANT_MACROS 1
#include <limits.h>
#include <stdint.h>

// design choice: no padding
// we have assumed no padding bits within the uintmax_t representation
// follow Intel design decision: suppress leading 1 for normalized numerals
class z_float
{
private:
	uintmax_t mantissa;
	// bias for exponent is UINTMAX_MAX/4
	uintmax_t exponent:(sizeof(uintmax_t)*CHAR_BIT-1U);
	uintmax_t is_negative:1;
public:
	// for when we want to control the bit pattern rather than the value
	// we can't normalize this
	explicit z_float(bool _negative,uintmax_t exp,uintmax_t man) : mantissa(man),exponent(exp),is_negative(_negative) {};
	explicit z_float(intmax_t src);
	explicit z_float(uintmax_t src = 0);

	// IEEE-754 allows this to be a non-arithmetic operation
	z_float& operator=(const z_float& src);
	// IEEE-754 allows this to be a non-arithmetic operation
	z_float operator-() const;	
	
	// IEEE-754 operator==
	// NaN!=NaN
	// zero equals negative zero
	friend bool operator==(const z_float& lhs, const z_float& rhs);
	
	// cmath/math.h emulation
	// IEEE-754 allows this to be a non-arithmetic operation
	friend z_float copysign(const z_float& x, const z_float& y);
	// static ... fpclassify(const z_float& x);
	friend bool isfinite(const z_float& x) {return UINTMAX_MAX/2U>x.exponent;};
	friend bool isinf(const z_float& x) {return UINTMAX_MAX/2U<=x.exponent && 0==x.mantissa;};
	friend bool isnan(const z_float& x) {return UINTMAX_MAX/2U<=x.exponent && 0<x.mantissa;};
	friend bool isnormal(const z_float& x) {return UINTMAX_MAX/2U>x.exponent && 0<x.exponent;};
	friend bool signbit(const z_float& x) {return x.is_negative;};
	// not in cmath/math.h, but would be nice if it were
	friend bool is_zero(const z_float& x) {return 0==x.exponent && 0==x.mantissa;};
	friend bool isinf_or_nan(const z_float& x) {return UINTMAX_MAX/2U<=x.exponent;};
	friend bool isdenormal_or_zero(const z_float& x) {return 0==x.exponent;};
	// state inspection
	friend uintmax_t _mantissa(const z_float& x) {return x.mantissa;};
	friend intmax_t _exponent(const z_float& x) {return (intmax_t)(x.exponent)-(intmax_t)(UINTMAX_MAX/4U);};	
private:
	void init_from_uintmax_t(uintmax_t src);
};

inline bool operator!=(const z_float& lhs, const z_float& rhs) {return !(lhs==rhs);};

#endif
