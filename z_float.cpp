// z_float.cpp
// (C)2011 Kenneth Boyd, license: MIT.txt
// long double emulation implementation

#include "z_float.hpp"
#include "Zaimoni.STL/Logging.h"

z_float::z_float(intmax_t src)
:	mantissa(0),exponent(0),is_negative(0>src)
{
	if (0!=src) init_from_uintmax_t((0<src) ? src : -src);
}

z_float::z_float(uintmax_t src)
:	mantissa(0),exponent(0),is_negative(0)
{
	if (0!=src) init_from_uintmax_t(src);
}

void z_float::init_from_uintmax_t(uintmax_t src)
{
	assert(0==exponent);
	uintmax_t tmp = src;
	while(2<=tmp) (++exponent,tmp >>= 1);
	mantissa = ((src<<(63-exponent))-(UINTMAX_MAX/2+1))<<1;
	exponent += UINTMAX_MAX/4U;
}

z_float& z_float::operator=(const z_float& src)
{
	mantissa = src.mantissa;
	exponent = src.exponent;
	is_negative = src.is_negative;	
	// check whether memmove works
	return *this;
}

z_float z_float::operator-() const
{
	z_float tmp(*this);
	tmp.is_negative = !tmp.is_negative;
	return tmp;
}

z_float copysign(const z_float& x, const z_float& y)
{
	z_float tmp(x);
	tmp.is_negative = y.is_negative;
	return tmp;
}

bool operator==(const z_float& lhs, const z_float& rhs)
{
	if (isnan(lhs)) return false;
	if (isnan(rhs)) return false;
	if (is_zero(lhs) && is_zero(rhs)) return true;
	return lhs.mantissa==rhs.mantissa && lhs.exponent==rhs.exponent && lhs.is_negative==rhs.is_negative;
}

#ifdef TEST_APP
// example build line
// g++ -oz_float.exe -I/Mingwin.aux/Headers.lib -DTEST_APP z_float.cpp -lz_log_adapter -lz_stdio_log -lz_format_util
// only need -lz_stdio_log if INFORM-based debugging not used
// g++ -oz_float.exe -I/Mingwin.aux/Headers.lib -DTEST_APP z_float.cpp -lz_stdio_log

#include <stdlib.h>
#include <stdio.h>

// console-mode application
#define STRING_LITERAL_TO_STDOUT(A) fwrite(A,sizeof(A)-1,1,stdout)
#define C_STRING_TO_STDOUT(A) fwrite(A,strlen(A),1,stdout)
#define STL_PTR_STRING_TO_STDOUT(A) fwrite((A)->data(),(A)->size(),1,stdout)

#define EXAMINE(A) INFORM((A)._mantissa());INFORM((A)._exponent());INFORM(signbit(A))

int main(int argc, char* argv[])
{	// parse options
	STRING_LITERAL_TO_STDOUT("starting main\n");

	z_float NaN(false,UINTMAX_MAX/2,1);
	assert(!isfinite(NaN));
	assert(!isinf(NaN));
	assert(isnan(NaN));
	assert(!isnormal(NaN));
	assert(!is_zero(NaN));
	assert(isinf_or_nan(NaN));
	assert(!isdenormal_or_zero(NaN));
	assert(false==signbit(NaN));
	assert(1==_mantissa(NaN));
	assert((UINTMAX_MAX/2U-UINTMAX_MAX/4U)==_exponent(NaN));	
	assert(NaN!=NaN);
	STRING_LITERAL_TO_STDOUT("NaN OK\n");

	z_float pos_inf(false,UINTMAX_MAX/2,0);
	assert(!isfinite(pos_inf));
	assert(isinf(pos_inf));
	assert(!isnan(pos_inf));
	assert(!isnormal(pos_inf));
	assert(!is_zero(pos_inf));
	assert(isinf_or_nan(pos_inf));
	assert(!isdenormal_or_zero(pos_inf));
	assert(false==signbit(pos_inf));
	assert(0==_mantissa(pos_inf));
	assert((UINTMAX_MAX/2U-UINTMAX_MAX/4U)==_exponent(pos_inf));	
	assert(pos_inf==pos_inf);
	STRING_LITERAL_TO_STDOUT("pos_inf OK\n");

	z_float neg_inf(true,UINTMAX_MAX/2,0);
	assert(!isfinite(neg_inf));
	assert(isinf(neg_inf));
	assert(!isnan(neg_inf));
	assert(!isnormal(neg_inf));
	assert(!is_zero(neg_inf));
	assert(isinf_or_nan(neg_inf));
	assert(!isdenormal_or_zero(neg_inf));
	assert(true==signbit(neg_inf));
	assert(0==_mantissa(neg_inf));
	assert((UINTMAX_MAX/2U-UINTMAX_MAX/4U)==_exponent(neg_inf));	
	assert(neg_inf==neg_inf);
	STRING_LITERAL_TO_STDOUT("neg_inf OK\n");

	z_float zero_1(false,0,0);
	z_float zero_2(0LL);
	z_float zero_3(0ULL);
	z_float zero_4;
	z_float neg_zero(true,0,0);

	assert(isfinite(zero_1));
	assert(!isinf(zero_1));
	assert(!isnan(zero_1));
	assert(!isnormal(zero_1));
	assert(is_zero(zero_1));
	assert(!isinf_or_nan(zero_1));
	assert(isdenormal_or_zero(zero_1));
	assert(false==signbit(zero_1));
	assert(0==_mantissa(zero_1));
	assert(zero_1==zero_1);
	assert(-(intmax_t)(UINTMAX_MAX/4U)==_exponent(zero_1));	

	assert(isfinite(zero_2));
	assert(!isinf(zero_2));
	assert(!isnan(zero_2));
	assert(!isnormal(zero_2));
	assert(is_zero(zero_2));
	assert(!isinf_or_nan(zero_2));
	assert(isdenormal_or_zero(zero_2));
	assert(false==signbit(zero_2));
	assert(0==_mantissa(zero_2));
	assert(zero_2==zero_2);
	assert(-(intmax_t)(UINTMAX_MAX/4U)==_exponent(zero_2));	

	assert(zero_1==zero_2);
	assert(zero_2==zero_1);
	
	assert(isfinite(zero_3));
	assert(!isinf(zero_3));
	assert(!isnan(zero_3));
	assert(!isnormal(zero_3));
	assert(is_zero(zero_3));
	assert(!isinf_or_nan(zero_3));
	assert(isdenormal_or_zero(zero_3));
	assert(false==signbit(zero_3));
	assert(0==_mantissa(zero_3));
	assert(zero_3==zero_3);
	assert(-(intmax_t)(UINTMAX_MAX/4U)==_exponent(zero_3));	

	assert(zero_1==zero_3);
	assert(zero_3==zero_1);
	assert(zero_2==zero_3);
	assert(zero_3==zero_2);
	
	assert(isfinite(zero_4));
	assert(!isinf(zero_4));
	assert(!isnan(zero_4));
	assert(!isnormal(zero_4));
	assert(is_zero(zero_4));
	assert(!isinf_or_nan(zero_4));
	assert(isdenormal_or_zero(zero_4));
	assert(false==signbit(zero_4));
	assert(0==_mantissa(zero_4));
	assert(zero_4==zero_4);
	assert(-(intmax_t)(UINTMAX_MAX/4U)==_exponent(zero_4));	

	assert(zero_1==zero_4);
	assert(zero_4==zero_1);
	assert(zero_2==zero_4);
	assert(zero_4==zero_2);
	assert(zero_3==zero_4);
	assert(zero_4==zero_3);
	
	assert(isfinite(neg_zero));
	assert(!isinf(neg_zero));
	assert(!isnan(neg_zero));
	assert(!isnormal(neg_zero));
	assert(is_zero(neg_zero));
	assert(!isinf_or_nan(neg_zero));
	assert(isdenormal_or_zero(neg_zero));
	assert(true==signbit(neg_zero));
	assert(0==_mantissa(neg_zero));
	assert(neg_zero==neg_zero);
	assert(-(intmax_t)(UINTMAX_MAX/4U)==_exponent(neg_zero));	

	assert(zero_1==neg_zero);
	assert(neg_zero==zero_1);
	assert(zero_2==neg_zero);
	assert(neg_zero==zero_2);
	assert(zero_3==neg_zero);
	assert(neg_zero==zero_3);
	assert(zero_4==neg_zero);
	assert(neg_zero==zero_4);
	
	STRING_LITERAL_TO_STDOUT("zero OK\n");

	z_float one_1(false,UINTMAX_MAX/4U,0);
	z_float one_2(1LL);
	z_float one_3(1ULL);
	z_float neg_one_1(true,UINTMAX_MAX/4U,0);
	z_float neg_one_2(-1LL);

	assert(isfinite(one_2));
	assert(!isinf(one_2));
	assert(!isnan(one_2));
	assert(isnormal(one_2));
	assert(!is_zero(one_2));
	assert(!isinf_or_nan(one_2));
	assert(!isdenormal_or_zero(one_2));
	assert(false==signbit(one_2));
	assert(0==_mantissa(one_2));
	assert(one_2==one_2);
	assert(0==_exponent(one_2));	

	assert(isfinite(neg_one_2));
	assert(!isinf(neg_one_2));
	assert(!isnan(neg_one_2));
	assert(isnormal(neg_one_2));
	assert(!is_zero(neg_one_2));
	assert(!isinf_or_nan(neg_one_2));
	assert(!isdenormal_or_zero(neg_one_2));
	assert(true==signbit(neg_one_2));
	assert(0==_mantissa(neg_one_2));
	assert(neg_one_2==neg_one_2);
	assert(0==_exponent(neg_one_2));	

	assert(one_1==one_2);	
	assert(one_2==one_1);	
	assert(one_1==one_3);	
	assert(one_3==one_1);	

	assert(neg_one_1==neg_one_2);	
	assert(neg_one_2==neg_one_1);	

	assert(one_1 == -neg_one_1);
	assert(neg_one_1 == -one_1);

	assert(one_1 == copysign(one_1,one_1));
	assert(one_1 == copysign(neg_one_1,one_1));
	assert(neg_one_1 == copysign(one_1,neg_one_1));
	assert(neg_one_1 == copysign(neg_one_1,neg_one_1));
	
	STRING_LITERAL_TO_STDOUT("one OK\n");
	STRING_LITERAL_TO_STDOUT("tests finished\n");
}

#endif
