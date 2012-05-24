// z_float.cpp
// (C)2011 Kenneth Boyd, license: MIT.txt
// long double emulation implementation

#include "z_float.hpp"
#include "Zaimoni.STL/Logging.h"
#include "Zaimoni.STL/Pure.C/auto_int.h"

// NOTE: don't need z_float(float|double|long double) constuctors for Z.C++,
// but would for scientific computation.  It is worth stuffing host
// format detection into auto_int.h?

unsigned char z_float::_modes = 0;	// default-initialization
z_float::trapfunc* z_float::traps[5] = {NULL, NULL, NULL, NULL, NULL};	// default-initialization

// rounding modes
#define Z_FLOAT_TO_NEAREST 0
#define Z_FLOAT_TO_INF 1
#define Z_FLOAT_TO_NEG_INF 2
#define Z_FLOAT_TO_ZERO 3

// array indexes
#define Z_FLOAT_INVALID 0
#define Z_FLOAT_DIVZERO 1
#define Z_FLOAT_OVERFLOW 2
#define Z_FLOAT_UNDERFLOW 3
#define Z_FLOAT_INEXACT 4

// opcodes
#define Z_FLOAT_CODE_MULT 1

// NaN codes
#define Z_FLOAT_NAN_ZEROINF_MULT (1ULL<<1)

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

/*
map of significant digits for multiplication of two finite, not-denormal numerals
               1 32 bits:2^-32 32 bits:2^-64
1
32 bits:2^-32
32 bits:2^-64

so we need 4 64-bit integers: 2^-64, 2^-96, 2^-96, 2^-128
*/

// preprocessor test for good half-width type
#define HALFWIDTH_BITS ((INT_LOG2(UINTMAX_MAX)+1)/2)
#define HALFWIDTH_MASK (UINTMAX_MAX>>HALFWIDTH_BITS)
#if HALFWIDTH_MASK>=ULLONG_MAX 
#define HALFWIDTH_UINT unsigned long long
#elif HALFWIDTH_MASK>=ULONG_MAX 
#define HALFWIDTH_UINT unsigned long
#elif HALFWIDTH_MASK>=UINT_MAX 
#define HALFWIDTH_UINT unsigned int
#elif HALFWIDTH_MASK>=USHRT_MAX 
#define HALFWIDTH_UINT unsigned short
#elif HALFWIDTH_MASK>=UCHAR_MAX 
#define HALFWIDTH_UINT unsigned char
#else
#define HALFWIDTH_UINT uintmax_t
#endif

void z_float::mult_overflow(const z_float& rhs, uintmax_t extended_mantissa[4])
{
	if (traps[Z_FLOAT_OVERFLOW] && (traps[Z_FLOAT_OVERFLOW])(*this,rhs,*this,(1<<(Z_FLOAT_OVERFLOW+2))+_rounding_mode(),Z_FLOAT_CODE_MULT,extended_mantissa))
		return;
	// if we didn't overflow-trap, try to inexact-trap
	if (traps[Z_FLOAT_INEXACT] && (traps[Z_FLOAT_INEXACT])(*this,rhs,*this,(1<<(Z_FLOAT_INEXACT+2))+_rounding_mode(),Z_FLOAT_CODE_MULT,extended_mantissa))
		return;
	// record both overflow and inexact
	_modes &= (1<<(Z_FLOAT_OVERFLOW+2))+(1<<(Z_FLOAT_INEXACT+2));
	switch(4*is_negative+_rounding_mode())
	{
	default: assert(0 && "z_float *= signbit+rounding mode out of range");
	case Z_FLOAT_TO_NEAREST:
	case 4+Z_FLOAT_TO_NEAREST:
	case Z_FLOAT_TO_INF:
	case 4+Z_FLOAT_TO_NEG_INF:
		// infinity
		mantissa = 0;
		exponent = UINTMAX_MAX/2;
		return;			
	case 4+Z_FLOAT_TO_INF:
	case Z_FLOAT_TO_NEG_INF:
	case Z_FLOAT_TO_ZERO:
	case 4+Z_FLOAT_TO_ZERO:
		// most extreme finite
		mantissa = UINTMAX_MAX;
		exponent = UINTMAX_MAX/2-1;
		return;
	}
}

static void _multiply_intermediate(uintmax_t dest[4], HALFWIDTH_UINT sections[4], uintmax_t lhs, uintmax_t rhs)
{
	// comments for example 64-bit uintmax_t
	sections[0] = lhs>>HALFWIDTH_BITS;	// 2^-32
	sections[1] = lhs & HALFWIDTH_BITS;	// 2^-64
	sections[2] = rhs>>HALFWIDTH_BITS;	// 2^-32
	sections[3] = rhs & HALFWIDTH_BITS;	// 2^-64
	dest[0] = uintmax_t(sections[0])*uintmax_t(sections[2]);	// 2^-64
	dest[1] = uintmax_t(sections[1])*uintmax_t(sections[3]);	// 2^-128
	dest[2] = uintmax_t(sections[0])*uintmax_t(sections[3]);	// 2^-96
	dest[3] = uintmax_t(sections[1])*uintmax_t(sections[2]);	// 2^-96
	// we are modeling worst-case UINT64_MAX*UINT64_MAX with arguments cast to UINT128,
	// which is short of UINT128_MAX by 2*UINT64_MAX
	// so both dest[2] and dest[3] should be zero when we're done
	if (UINTMAX_MAX-dest[2]<dest[3])	// adding the two 2^-96 terms overflows
		dest[0] += (1ULL<<HALFWIDTH_BITS);	// just used up UINT32_MAX+1 of our 2*UINT32_MAX grace in dest[0]
	dest[2] += dest[3];	// dest[3] now expired
	dest[3] = (dest[2] & HALFWIDTH_MASK)<<HALFWIDTH_BITS;	// rescale lower half for 2^-128
	dest[2] >>= HALFWIDTH_BITS;	// rescale upper half for 2^-64
	if (UINTMAX_MAX-dest[1]<dest[3])
		dest[0] += 1ULL;	// used up another 1 grace in dest[0]
	dest[1]+=dest[3];	// dest[3] now expired
	dest[3] = 0;		// play nice later
	assert(UINTMAX_MAX-dest[0]>=dest[2]);
	dest[0] += dest[2];	// dest[2] now expired
	dest[2] = 0;
}

z_float& z_float::operator*=(const z_float& rhs)
{
	if (issnan(rhs))
		{	// invalid operation: trap if possible, otherwise downgrade to qNaN
		if (traps[Z_FLOAT_INVALID] && (traps[Z_FLOAT_INVALID])(*this,rhs,*this,(1<<(Z_FLOAT_INVALID+2))+_rounding_mode(),Z_FLOAT_CODE_MULT,NULL));
			return *this;
		_modes &= 1<<(Z_FLOAT_INVALID+2);
		}
	else if (issnan(*this))
		{
		if (traps[Z_FLOAT_INVALID] && (traps[Z_FLOAT_INVALID])(*this,rhs,*this,(1<<(Z_FLOAT_INVALID+2))+_rounding_mode(),Z_FLOAT_CODE_MULT,NULL));
			return *this;
		_modes &= 1<<(Z_FLOAT_INVALID+2);
		};

	// handle sign now
	if (rhs.is_negative) is_negative = !is_negative;
	if (isnan(rhs))
		{
		if (isnan(*this))
			mantissa |= rhs.mantissa;
		else
			*this = rhs;
		mantissa &= ~(1ULL);	// we no longer trap
		return *this;
		}
	if (isnan(*this))
		{
		mantissa &= ~(1ULL);	// we no longer trap
		return *this;
		}

	// zero*infinity and infinity*zero are invalid
	if (   (is_zero(*this) && isinf(rhs))
		|| (is_zero(rhs) && isinf(*this)))
		{	
		if (traps[Z_FLOAT_INVALID] && (traps[Z_FLOAT_INVALID])(*this,rhs,*this,(1<<(Z_FLOAT_INVALID+2))+_rounding_mode(),Z_FLOAT_CODE_MULT,NULL))
			return *this;
		_modes &= 1<<(Z_FLOAT_INVALID+2);
		exponent = UINTMAX_MAX/2U;
		mantissa = Z_FLOAT_NAN_ZEROINF_MULT;
		return *this;
		}

	// handle zero and infinity
	if (is_zero(rhs))
		{
		exponent = 0;
		mantissa = 0;
		return *this;
		};
	if (isinf(rhs))
		{
		exponent = UINTMAX_MAX/2U;
		mantissa = 0;
		return *this;
		};
	if (is_zero(*this) || isinf(*this)) return *this;
		
	// example, 64-bit intmax_t
	// INT63_MIN+INT63_MIN==INT64_MIN on two's complement machines;
	// if that traps, _exponent(*this)+_exponent(rhs) dies
	// so don't do that
	intmax_t raw_exponent = (1+_exponent(*this))+_exponent(rhs);
	// note: we have a total loss of precision denormalization if 
	// raw_exponent <= -(intmax_t)(UINTMAX_MAX/4)-64	
	if (-(intmax_t)(UINTMAX_MAX/4)-(INT_LOG2(UINTMAX_MAX)+1) >= raw_exponent)
		{	// denormalize to zero-ish immediately
		// if we have an underflow trap, trap now
		if (traps[Z_FLOAT_UNDERFLOW] && (traps[Z_FLOAT_UNDERFLOW])(*this,rhs,*this,(1<<(Z_FLOAT_UNDERFLOW+2))+_rounding_mode(),Z_FLOAT_CODE_MULT,NULL))
			return *this;
		switch(4*is_negative+_rounding_mode())
		{
		default: assert(0 && "z_float *= signbit+rounding mode out of range");
		case Z_FLOAT_TO_ZERO:
		case 4+Z_FLOAT_TO_ZERO:
		case 4+Z_FLOAT_TO_INF:
		case Z_FLOAT_TO_NEG_INF:
		case Z_FLOAT_TO_NEAREST:
		case 4+Z_FLOAT_TO_NEAREST:
			// to nearest
			mantissa = 0;
			exponent = 0;
			return *this;
		case Z_FLOAT_TO_INF:
		case 4+Z_FLOAT_TO_NEG_INF:
			mantissa = 1;
			exponent = 0;
			return *this;
		}
		}

	uintmax_t lhs_mantissa = this->mantissa;
	uintmax_t rhs_mantissa = rhs.mantissa;
	// adjust format to as-if-normalized
	if (0==this->exponent)
		{
		assert(0!=lhs_mantissa);
		while(UINTMAX_MAX/2>=lhs_mantissa)
			{
			lhs_mantissa<<=1;
			--raw_exponent;
			}
		lhs_mantissa<<=1;
		};
	if (0==rhs.exponent)
		{
		assert(0!=rhs_mantissa);
		while(UINTMAX_MAX/2>=rhs_mantissa)
			{
			rhs_mantissa<<=1;
			--raw_exponent;
			}
		lhs_mantissa<<=1;
		};
	if (-(intmax_t)(UINTMAX_MAX/4)-(INT_LOG2(UINTMAX_MAX)+1) >= raw_exponent)
		{	// denormalize to zero-ish immediately
		// if we have an underflow trap, trap now
		if (traps[Z_FLOAT_UNDERFLOW] && (traps[Z_FLOAT_UNDERFLOW])(*this,rhs,*this,(1<<(Z_FLOAT_UNDERFLOW+2))+_rounding_mode(),Z_FLOAT_CODE_MULT,NULL))
			return *this;
		switch(4*is_negative+_rounding_mode())
		{
		default: assert(0 && "z_float *= signbit+rounding mode out of range");
		case Z_FLOAT_TO_ZERO:
		case 4+Z_FLOAT_TO_ZERO:
		case 4+Z_FLOAT_TO_INF:
		case Z_FLOAT_TO_NEG_INF:
		case Z_FLOAT_TO_NEAREST:
		case 4+Z_FLOAT_TO_NEAREST:
			// to nearest
			mantissa = 0;
			exponent = 0;
			return *this;
		case Z_FLOAT_TO_INF:
		case 4+Z_FLOAT_TO_NEG_INF:
			mantissa = 1;
			exponent = 0;
			return *this;
		}
		}

	HALFWIDTH_UINT tmp[4];
	uintmax_t extended_mantissa[4];
	_multiply_intermediate(extended_mantissa,tmp,lhs_mantissa,rhs_mantissa);	

	// try to put that partial sum in dest[3]
	extended_mantissa[3] = rhs_mantissa;
	// example, 64-bit uintmax_t
	extended_mantissa[2] = (UINTMAX_MAX-extended_mantissa[3]<lhs_mantissa);	// 2^0
	extended_mantissa[3] += lhs_mantissa;	// 2^-64
	extended_mantissa[2] += (UINTMAX_MAX-extended_mantissa[0]<extended_mantissa[3]);
	extended_mantissa[0] += extended_mantissa[3];	// extended_mantissa[3] now expired
	extended_mantissa[3] = 0;
	// above says we could have wrapped twice, but this looks incorrect
	// as INT65_MAX*INT65_MAX falls short of INT130_MAX by 2*INT65_MAX
	assert(1>=extended_mantissa[2]);

	// check for overflow at this point, as adding 1 to INTMAX_MAX is undefined
	if ((intmax_t)(UINTMAX_MAX/4U)-(intmax_t)(extended_mantissa[2])<=raw_exponent)
		{	// we overflowed
		mult_overflow(rhs,extended_mantissa);
		return *this;
		}

	if (1<=extended_mantissa[2])
		{	// we wrapped.  store lost bit in extended_mantissa[2]
		extended_mantissa[2] = (extended_mantissa[1] & 1)<<(HALFWIDTH_BITS-1);
		extended_mantissa[1] >>= 1;
		extended_mantissa[1] += (extended_mantissa[0] & 1)<<(HALFWIDTH_BITS-1);
		extended_mantissa[0] >>= 1;
		++raw_exponent;
		};

	if (-((intmax_t)(UINTMAX_MAX/4U)+1)>=raw_exponent)
		{	// final result is denormalized
		extended_mantissa[2] >>= 1;
		extended_mantissa[2] += (extended_mantissa[1] & 1)<<(HALFWIDTH_BITS-1);
		extended_mantissa[1] >>= 1;
		extended_mantissa[1] += (extended_mantissa[0] & 1)<<(HALFWIDTH_BITS-1);
		extended_mantissa[0] >>= 1;
		extended_mantissa[0] += (UINTMAX_MAX/2U)+1U;

		// if we have an underflow trap, trap now
		if (traps[Z_FLOAT_UNDERFLOW] && (traps[Z_FLOAT_UNDERFLOW])(*this,rhs,*this,(1<<(Z_FLOAT_UNDERFLOW+2))+_rounding_mode(),Z_FLOAT_CODE_MULT,NULL))
			return *this;
		if (-(intmax_t)(UINTMAX_MAX/4)-(INT_LOG2(UINTMAX_MAX)+1) >= raw_exponent)
			{	// denormalize to zero-ish immediately
			switch(4*is_negative+_rounding_mode())
			{
			default: assert(0 && "z_float *= signbit+rounding mode out of range");
			case Z_FLOAT_TO_ZERO:
			case 4+Z_FLOAT_TO_ZERO:
			case 4+Z_FLOAT_TO_INF:
			case Z_FLOAT_TO_NEG_INF:
			case Z_FLOAT_TO_NEAREST:
			case 4+Z_FLOAT_TO_NEAREST:
				// to nearest
				mantissa = 0;
				exponent = 0;
				return *this;
			case Z_FLOAT_TO_INF:
			case 4+Z_FLOAT_TO_NEG_INF:
				mantissa = 1;
				exponent = 0;
				return *this;
			}
			}
		}
	
	// if we have catastrophically underflowed
	if (intmax_t critical_underflow = -((intmax_t)(UINTMAX_MAX/4U)+1)>raw_exponent ? -((intmax_t)(UINTMAX_MAX/4U)+1)-raw_exponent : 0)
		{
		const int scale_by = INT_LOG2(UINTMAX_MAX)>critical_underflow ? critical_underflow : INT_LOG2(UINTMAX_MAX);
		const int dual_scale_by = (INT_LOG2(UINTMAX_MAX)+1)-scale_by;
		extended_mantissa[2] >>= scale_by;
		extended_mantissa[2] += (extended_mantissa[1] & (UINTMAX_MAX>>dual_scale_by))<<dual_scale_by;
		extended_mantissa[1] >>= scale_by;
		extended_mantissa[1] += (extended_mantissa[0] & (UINTMAX_MAX>>dual_scale_by))<<dual_scale_by;
		extended_mantissa[0] >>= scale_by;
		raw_exponent += scale_by;
		};

	if (-(intmax_t)(UINTMAX_MAX/4)-1 >= raw_exponent)
		{	// denormalize to zero-ish immediately
		switch(4*is_negative+_rounding_mode())
		{
		default: assert(0 && "z_float *= signbit+rounding mode out of range");
		case Z_FLOAT_TO_ZERO:
		case 4+Z_FLOAT_TO_ZERO:
		case 4+Z_FLOAT_TO_INF:
		case Z_FLOAT_TO_NEG_INF:
		case Z_FLOAT_TO_NEAREST:
		case 4+Z_FLOAT_TO_NEAREST:
			// to nearest
			mantissa = 0;
			exponent = 0;
			return *this;
		case Z_FLOAT_TO_INF:
		case 4+Z_FLOAT_TO_NEG_INF:
			mantissa = 1;
			exponent = 0;
			return *this;
		}
		}
		
	bool inexact = extended_mantissa[1] || extended_mantissa[2];
	if (inexact)
		{
		if (traps[Z_FLOAT_INEXACT] && (traps[Z_FLOAT_INEXACT])(*this,rhs,*this,(1<<(Z_FLOAT_INEXACT+2))+_rounding_mode(),Z_FLOAT_CODE_MULT,NULL))
			return *this;
		// apply rounding now
		switch(4*is_negative+_rounding_mode())
		{
		default: assert(0 && "z_float *= signbit+rounding mode out of range");
		case Z_FLOAT_TO_NEAREST:
		case 4+Z_FLOAT_TO_NEAREST:
			// to nearest
			if (UINTMAX_MAX/2U>=extended_mantissa[1]) break;
			if (UINTMAX_MAX/2U+1==extended_mantissa[1] && !(extended_mantissa[0] & 1)) break;
			// larger absolute magnitude, but need different underflow handling
			if (!++extended_mantissa[0])
				{	// mantissa was UINTMAX_MAX beforehand
				if ((intmax_t)(UINTMAX_MAX/4U)<= ++raw_exponent)
					{	// overflow
					mult_overflow(rhs,extended_mantissa);
					return *this;
					}
				}
			break;
		case Z_FLOAT_TO_INF:
		case 4+Z_FLOAT_TO_NEG_INF:
			// larger absolute magnitude
			if (!++extended_mantissa[0])
				{	// mantissa was UINTMAX_MAX beforehand
				if ((intmax_t)(UINTMAX_MAX/4U)<= ++raw_exponent)
					{	// overflow
					mult_overflow(rhs,extended_mantissa);
					return *this;
					}
				}
			break;
		case Z_FLOAT_TO_ZERO:
		case 4+Z_FLOAT_TO_ZERO:
		case 4+Z_FLOAT_TO_INF:
		case Z_FLOAT_TO_NEG_INF:
			// smaller absolute magnitude
			break;				
		}
		}
	
	// point estimate now
	mantissa = extended_mantissa[0];
	exponent = raw_exponent+(intmax_t)(UINTMAX_MAX/4U);
	return *this;
}

#if 0
z_float& operator/=(const z_float& rhs)
{
}

z_float& operator+=(const z_float& rhs)
{
}
#endif

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

	z_float zero(false,0,0);
	z_float zero_2(0LL);
	z_float zero_3(0ULL);
	z_float zero_4;
	z_float neg_zero(true,0,0);

	assert(isfinite(zero));
	assert(!isinf(zero));
	assert(!isnan(zero));
	assert(!isnormal(zero));
	assert(is_zero(zero));
	assert(!isinf_or_nan(zero));
	assert(isdenormal_or_zero(zero));
	assert(false==signbit(zero));
	assert(0==_mantissa(zero));
	assert(zero==zero);
	assert(-(intmax_t)(UINTMAX_MAX/4U)==_exponent(zero));	

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

	assert(zero==zero_2);
	assert(zero_2==zero);
	
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

	assert(zero==zero_3);
	assert(zero_3==zero);
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

	assert(zero==zero_4);
	assert(zero_4==zero);
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

	assert(zero==neg_zero);
	assert(neg_zero==zero);
	assert(zero_2==neg_zero);
	assert(neg_zero==zero_2);
	assert(zero_3==neg_zero);
	assert(neg_zero==zero_3);
	assert(zero_4==neg_zero);
	assert(neg_zero==zero_4);
	
	STRING_LITERAL_TO_STDOUT("zero OK\n");

	z_float one(1LL);
	z_float one_2(false,UINTMAX_MAX/4U,0);
	z_float one_3(1ULL);
	z_float neg_one(-1LL);
	z_float neg_one_2(true,UINTMAX_MAX/4U,0);

	assert(isfinite(one));
	assert(!isinf(one));
	assert(!isnan(one));
	assert(isnormal(one));
	assert(!is_zero(one));
	assert(!isinf_or_nan(one));
	assert(!isdenormal_or_zero(one));
	assert(false==signbit(one));
	assert(0==_mantissa(one));
	assert(one==one);
	assert(0==_exponent(one));	

	assert(isfinite(neg_one));
	assert(!isinf(neg_one));
	assert(!isnan(neg_one));
	assert(isnormal(neg_one));
	assert(!is_zero(neg_one));
	assert(!isinf_or_nan(neg_one));
	assert(!isdenormal_or_zero(neg_one));
	assert(true==signbit(neg_one));
	assert(0==_mantissa(neg_one));
	assert(neg_one==neg_one);
	assert(0==_exponent(neg_one));	

	assert(one_2==one);	
	assert(one==one_2);	
	assert(one_2==one_3);	
	assert(one_3==one_2);	

	assert(neg_one_2==neg_one);	
	assert(neg_one==neg_one_2);	

	assert(one_2 == -neg_one_2);
	assert(neg_one_2 == -one_2);

	assert(one == copysign(one,one));
	assert(one == copysign(neg_one,one));
	assert(neg_one == copysign(one,neg_one));
	assert(neg_one == copysign(neg_one,neg_one));

	STRING_LITERAL_TO_STDOUT("one OK\n");
	
	z_float two(2LL);
	z_float two_2(false,UINTMAX_MAX/4U+1,0);
	z_float two_3(2ULL);
	z_float neg_two(-2LL);
	z_float neg_two_2(true,UINTMAX_MAX/4U+1,0);

	assert(isfinite(two));
	assert(!isinf(two));
	assert(!isnan(two));
	assert(isnormal(two));
	assert(!is_zero(two));
	assert(!isinf_or_nan(two));
	assert(!isdenormal_or_zero(two));
	assert(false==signbit(two));
	assert(0==_mantissa(two));
	assert(two==two);
	assert(1==_exponent(two));	

	assert(isfinite(neg_two));
	assert(!isinf(neg_two));
	assert(!isnan(neg_two));
	assert(isnormal(neg_two));
	assert(!is_zero(neg_two));
	assert(!isinf_or_nan(neg_two));
	assert(!isdenormal_or_zero(neg_two));
	assert(true==signbit(neg_two));
	assert(0==_mantissa(neg_two));
	assert(neg_two==neg_two);
	assert(1==_exponent(neg_two));	

	assert(two_2==two);	
	assert(two==two_2);	
	assert(two_2==two_3);	
	assert(two_3==two_2);	

	assert(neg_two_2==neg_two);	
	assert(neg_two==neg_two_2);	

	assert(two == -neg_two);
	assert(neg_two == -two);
	
	STRING_LITERAL_TO_STDOUT("two OK\n");

	STRING_LITERAL_TO_STDOUT("tests finished\n");
}

#endif
