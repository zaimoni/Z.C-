// z_float.cpp
// (C)2011,2012 Kenneth Boyd, license: MIT.txt
// long double emulation implementation

#include "z_float.hpp"
#include "Zaimoni.STL/Logging.h"
#include "Zaimoni.STL/Pure.C/auto_int.h"

#include <utility>

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
#define Z_FLOAT_CODE_DIV 2
#define Z_FLOAT_CODE_ADD 2

// NaN codes
#define Z_FLOAT_NAN_ZEROINF_MULT (1ULL<<1)
#define Z_FLOAT_NAN_INF_NEGINF_ADD (1ULL<<2)
#define Z_FLOAT_NAN_ZERO_ZERO_DIV (1ULL<<3)
#define Z_FLOAT_NAN_INF_INF_DIV (1ULL<<4)

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

// Cf IEEE-754 7.3
// \pre *this is already the correct sign
z_float& z_float::IEEE_overflow(const unsigned round_mode,const z_float& lhs, const z_float& rhs,const int opcode)
{
	assert(3>=round_mode);
	if (traps[Z_FLOAT_OVERFLOW] && (traps[Z_FLOAT_OVERFLOW])(lhs,rhs,*this,(1<<(Z_FLOAT_OVERFLOW+2))+round_mode,opcode,NULL))
		return *this;
	// if we didn't overflow-trap, try to inexact-trap
	if (traps[Z_FLOAT_INEXACT] && (traps[Z_FLOAT_INEXACT])(lhs,rhs,*this,(1<<(Z_FLOAT_INEXACT+2))+round_mode,opcode,NULL))
		return *this;

	// record both overflow and inexact
	_modes |= (1<<(Z_FLOAT_OVERFLOW+2))+(1<<(Z_FLOAT_INEXACT+2));
	switch(4*is_negative+round_mode)
	{
	default: _fatal_code("z_float::IEEE_overflow: signbit+rounding mode out of range",3);
	case Z_FLOAT_TO_NEAREST:
	case 4+Z_FLOAT_TO_NEAREST:
	case Z_FLOAT_TO_INF:
	case 4+Z_FLOAT_TO_NEG_INF:
		// infinity
		mantissa = 0;
		exponent = UINTMAX_MAX/2;
		return *this;			
	case 4+Z_FLOAT_TO_INF:
	case Z_FLOAT_TO_NEG_INF:
	case Z_FLOAT_TO_ZERO:
	case 4+Z_FLOAT_TO_ZERO:
		// most extreme finite
		mantissa = UINTMAX_MAX;
		exponent = UINTMAX_MAX/2-1;
		return *this;
	}
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

// \pre *this is already the correct sign
z_float& z_float::IEEE_underflow_to_zero(const unsigned round_mode,const z_float& lhs, const z_float& rhs,const int opcode)
{
	assert(3>=round_mode);
	if (traps[Z_FLOAT_UNDERFLOW] && (traps[Z_FLOAT_UNDERFLOW])(lhs,rhs,*this,(1<<(Z_FLOAT_UNDERFLOW+2))+round_mode,opcode,NULL))
		return *this;
	if (traps[Z_FLOAT_INEXACT] && (traps[Z_FLOAT_INEXACT])(lhs,rhs,*this,(1<<(Z_FLOAT_INEXACT+2))+round_mode,opcode,NULL))
		return *this;
	// record both overflow and inexact
	_modes |= (1<<(Z_FLOAT_UNDERFLOW+2))+(1<<(Z_FLOAT_INEXACT+2));
	switch(4*is_negative+round_mode)
	{
	default: _fatal_code("z_float::IEEE_underflow_to_zero: signbit+rounding mode out of range",3);
	case Z_FLOAT_TO_ZERO:
	case 4+Z_FLOAT_TO_ZERO:
	case 4+Z_FLOAT_TO_INF:
	case Z_FLOAT_TO_NEG_INF:
	case Z_FLOAT_TO_NEAREST:
	case 4+Z_FLOAT_TO_NEAREST:
		// to nearest
		mantissa = 0;
		break;
	case Z_FLOAT_TO_INF:
	case 4+Z_FLOAT_TO_NEG_INF:
		mantissa = 1;
//		break;
	}
	exponent = 0;
	return *this;
}

// Cf IEEE-754 4.1, 4.2
// \pre *this is already the correct sign
// \pre the remainder being rounded away is the same sign
z_float& z_float::IEEE_round_from_infinity(const unsigned round_mode,const z_float& lhs, const z_float& rhs,const int opcode,const int cmp_half_epsilon)
{
	assert(3>=round_mode);
	assert((intmax_t)(UINTMAX_MAX/2U)>exponent);
	switch(4*is_negative+round_mode)
	{
	default: _fatal_code("z_float::IEEE_round_from_infinity: signbit+rounding mode out of range",3);
	case Z_FLOAT_TO_NEAREST:
	case 4+Z_FLOAT_TO_NEAREST:
		// to nearest
		if (0>cmp_half_epsilon) break;
		if (0==cmp_half_epsilon && !(mantissa & 1)) break;
		// intentional fallthrough
	case Z_FLOAT_TO_INF:
	case 4+Z_FLOAT_TO_NEG_INF:
		// larger absolute magnitude
		if (!add_bit(0))
			return IEEE_overflow(round_mode,lhs,rhs,opcode);
		break;
	case Z_FLOAT_TO_ZERO:
	case 4+Z_FLOAT_TO_ZERO:
	case 4+Z_FLOAT_TO_INF:
	case Z_FLOAT_TO_NEG_INF:
		// smaller absolute magnitude
		break;				
	}
	if (traps[Z_FLOAT_INEXACT] && (traps[Z_FLOAT_INEXACT])(lhs,rhs,*this,(1<<(Z_FLOAT_INEXACT+2))+round_mode,opcode,NULL))
		return *this;
	// record inexact
	_modes |= (1<<(Z_FLOAT_INEXACT+2));
	return *this;
}

// Cf IEEE-754 4.1, 4.2
// \pre *this is already the correct sign
// \pre the remainder being rounded away is the opposite sign
z_float& z_float::IEEE_round_from_zero(const unsigned round_mode,const z_float& lhs, const z_float& rhs,const int opcode,const int cmp_half_epsilon)
{
	assert(3>=round_mode);
	assert((intmax_t)(UINTMAX_MAX/2U)>exponent);
	switch(4*is_negative+round_mode)
	{
	default: _fatal_code("z_float::IEEE_round_from_infinity: signbit+rounding mode out of range",3);
	case Z_FLOAT_TO_NEAREST:
	case 4+Z_FLOAT_TO_NEAREST:
		// to nearest
		if (0>cmp_half_epsilon) break;
		if (0==cmp_half_epsilon && !(mantissa & 1)) break;
		// intentional fallthrough
	case Z_FLOAT_TO_ZERO:
	case 4+Z_FLOAT_TO_ZERO:
		// larger absolute magnitude
		subtract_bit(0);
		break;
	case Z_FLOAT_TO_INF:
	case 4+Z_FLOAT_TO_NEG_INF:
	case 4+Z_FLOAT_TO_INF:
	case Z_FLOAT_TO_NEG_INF:
		// smaller absolute magnitude
		break;				
	}
	if (traps[Z_FLOAT_INEXACT] && (traps[Z_FLOAT_INEXACT])(lhs,rhs,*this,(1<<(Z_FLOAT_INEXACT+2))+round_mode,opcode,NULL))
		return *this;
	// record inexact
	_modes |= (1<<(Z_FLOAT_INEXACT+2));
	return *this;
}

z_float& z_float::operator*=(const z_float& rhs)
{
	if (issnan(rhs))
		{	// invalid operation: trap if possible, otherwise downgrade to qNaN
		if (traps[Z_FLOAT_INVALID] && (traps[Z_FLOAT_INVALID])(*this,rhs,*this,(1<<(Z_FLOAT_INVALID+2))+_rounding_mode(),Z_FLOAT_CODE_MULT,NULL));
			return *this;
		_modes |= 1<<(Z_FLOAT_INVALID+2);
		mantissa &= ~(1ULL);	// we no longer trap
		}
	else if (issnan(*this))
		{
		if (traps[Z_FLOAT_INVALID] && (traps[Z_FLOAT_INVALID])(*this,rhs,*this,(1<<(Z_FLOAT_INVALID+2))+_rounding_mode(),Z_FLOAT_CODE_MULT,NULL));
			return *this;
		_modes |= 1<<(Z_FLOAT_INVALID+2);
		mantissa &= ~(1ULL);	// we no longer trap
		};

	// handle sign now
	if (rhs.is_negative) is_negative = !is_negative;

	if (isnan(rhs))
		{
		if (isnan(*this))
			mantissa |= rhs.mantissa;
		else
			*this = rhs;
		return *this;
		}
	if (isnan(*this)) return *this;

	// zero*infinity and infinity*zero are invalid
	if (   (is_zero(*this) && isinf(rhs))
		|| (is_zero(rhs) && isinf(*this)))
		{	
		if (traps[Z_FLOAT_INVALID] && (traps[Z_FLOAT_INVALID])(*this,rhs,*this,(1<<(Z_FLOAT_INVALID+2))+_rounding_mode(),Z_FLOAT_CODE_MULT,NULL))
			return *this;
		_modes |= 1<<(Z_FLOAT_INVALID+2);
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
		// denormalize to zero-ish immediately
		return IEEE_underflow_to_zero(_rounding_mode(),z_float(*this),rhs,Z_FLOAT_CODE_MULT);

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
		rhs_mantissa<<=1;
		};
	if (-(intmax_t)(UINTMAX_MAX/4)-(INT_LOG2(UINTMAX_MAX)+1) >= raw_exponent)
		// denormalize to zero-ish immediately
		return IEEE_underflow_to_zero(_rounding_mode(),z_float(*this),rhs,Z_FLOAT_CODE_MULT);

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
		// we overflowed
		return IEEE_overflow(_rounding_mode(),z_float(*this),rhs,Z_FLOAT_CODE_MULT);

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

		if (-(intmax_t)(UINTMAX_MAX/4)-(INT_LOG2(UINTMAX_MAX)+1) >= raw_exponent)
			// denormalize to zero-ish immediately
			return IEEE_underflow_to_zero(_rounding_mode(),z_float(*this),rhs,Z_FLOAT_CODE_MULT);

		// if we have an underflow trap, trap now
		if (traps[Z_FLOAT_UNDERFLOW] && (traps[Z_FLOAT_UNDERFLOW])(*this,rhs,*this,(1<<(Z_FLOAT_UNDERFLOW+2))+_rounding_mode(),Z_FLOAT_CODE_MULT,NULL))
			return *this;
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
		// denormalize to zero-ish immediately
		return IEEE_underflow_to_zero(_rounding_mode(),z_float(*this),rhs,Z_FLOAT_CODE_MULT);
		
	if (extended_mantissa[1] || extended_mantissa[2])
		{
		z_float tmp(this->is_negative,raw_exponent+(intmax_t)(UINTMAX_MAX/4U),extended_mantissa[0]);
		return *this = tmp.IEEE_round_from_infinity(_rounding_mode(),*this,rhs,Z_FLOAT_CODE_MULT,zaimoni::cmp(extended_mantissa[1],UINTMAX_MAX/2U+1));
		}
	
	// point estimate now
	mantissa = extended_mantissa[0];
	exponent = raw_exponent+(intmax_t)(UINTMAX_MAX/4U);
	return *this;
}

z_float& z_float::operator/=(const z_float& rhs)
{
	if (issnan(rhs))
		{	// invalid operation: trap if possible, otherwise downgrade to qNaN
		if (traps[Z_FLOAT_INVALID] && (traps[Z_FLOAT_INVALID])(*this,rhs,*this,(1<<(Z_FLOAT_INVALID+2))+_rounding_mode(),Z_FLOAT_CODE_DIV,NULL));
			return *this;
		_modes |= 1<<(Z_FLOAT_INVALID+2);
		mantissa &= ~(1ULL);	// we no longer trap
		}
	else if (issnan(*this))
		{
		if (traps[Z_FLOAT_INVALID] && (traps[Z_FLOAT_INVALID])(*this,rhs,*this,(1<<(Z_FLOAT_INVALID+2))+_rounding_mode(),Z_FLOAT_CODE_DIV,NULL));
			return *this;
		_modes |= 1<<(Z_FLOAT_INVALID+2);
		mantissa &= ~(1ULL);	// we no longer trap
		};

	// handle sign now
	if (rhs.is_negative) is_negative = !is_negative;

	if (isnan(rhs))
		{
		if (isnan(*this))
			mantissa |= rhs.mantissa;
		else
			*this = rhs;
		return *this;
		}
	if (isnan(*this)) return *this;

	// 0/0 and infinity/infinity are invalid
	// 0/x is a correctly signed zero
	// x/infinity is a correctly signed zero
	// x/0 is a DIV_BY_ZERO exception with a non-trap result of a correctly signed infinity
	// infinity/x is a correctly signed infinity
	if (isinf(*this))
		{
		if (isinf(rhs))
			{	//	+-infinity/+-infinity
			if (traps[Z_FLOAT_INVALID] && (traps[Z_FLOAT_INVALID])(*this,rhs,*this,(1<<(Z_FLOAT_INVALID+2))+_rounding_mode(),Z_FLOAT_CODE_DIV,NULL))
				return *this;
			_modes |= 1<<(Z_FLOAT_INVALID+2);
			exponent = UINTMAX_MAX/2U;
			mantissa = Z_FLOAT_NAN_INF_INF_DIV;
			return *this;
			}
		else if (is_zero(rhs))
			{	// +-infinity/0
			if (traps[Z_FLOAT_DIVZERO] && (traps[Z_FLOAT_DIVZERO])(*this,rhs,*this,(1<<(Z_FLOAT_DIVZERO+2))+_rounding_mode(),Z_FLOAT_CODE_DIV,NULL))
				return *this;
			_modes |= 1<<(Z_FLOAT_DIVZERO+2);
			exponent = UINTMAX_MAX/2U;
			mantissa = 0;
			return *this;
			};
		// +-infinity/x
		return *this;
		}
	else if (is_zero(*this))
		{
		if (is_zero(rhs))
			{	//	0/0
			if (traps[Z_FLOAT_INVALID] && (traps[Z_FLOAT_INVALID])(*this,rhs,*this,(1<<(Z_FLOAT_INVALID+2))+_rounding_mode(),Z_FLOAT_CODE_DIV,NULL))
				return *this;
			_modes |= 1<<(Z_FLOAT_INVALID+2);
			exponent = UINTMAX_MAX/2U;
			mantissa = Z_FLOAT_NAN_ZERO_ZERO_DIV;
			return *this;
			}
		// 0/x or 0/infinity
		return *this;
		}
	else if (is_zero(rhs))
		{	// x/0
		if (traps[Z_FLOAT_DIVZERO] && (traps[Z_FLOAT_DIVZERO])(*this,rhs,*this,(1<<(Z_FLOAT_DIVZERO+2))+_rounding_mode(),Z_FLOAT_CODE_DIV,NULL))
			return *this;
		_modes |= 1<<(Z_FLOAT_DIVZERO+2);
		exponent = UINTMAX_MAX/2U;
		mantissa = 0;
		return *this;
		}

	// overflow check
	const intmax_t tmp = _exponent(rhs)-(mantissa<rhs.mantissa);
	if (0>tmp && -tmp<(intmax_t)(UINTMAX_MAX/2U)-(intmax_t)exponent)
		// we went unbounded
		return IEEE_overflow(_rounding_mode(),z_float(*this),rhs,Z_FLOAT_CODE_DIV);

	// catastrophic underflow check
	const intmax_t denormal_severity = (0<tmp && tmp>=(intmax_t)exponent) ? tmp-(intmax_t)exponent : -1;
	if (64<=denormal_severity)
		return IEEE_underflow_to_zero(_rounding_mode(),z_float(*this),rhs,Z_FLOAT_CODE_DIV);

	// division by exact power of two
	if (0==rhs.mantissa)
		{
		if (0==tmp) return *this;	// division by 1 is exact
		if (0>tmp)
			{
			exponent += -tmp;
			return *this;
			};
		if (tmp<(intmax_t)exponent)
			{
			exponent -= tmp;
			return *this;
			}
		// we denormalized
		assert(64>denormal_severity && 0<=denormal_severity);
		exponent = 0;
		mantissa >>= 1;
		mantissa |= UINTMAX_MAX/2+1;
		mantissa >>= denormal_severity;
		return *this;
		};
	_fatal_code("z_float::operator/= not fully implemented yet",3);
}

void z_float::subtract_implicit_leading_bit()
{
	assert(!isinf_or_nan(*this));
	assert(!isdenormal_or_zero(*this));
	if (0==mantissa)
		{	// now zero
		exponent = 0;
		return;
		}
	// \todo use compiler intrinsics here if available
	if (1==exponent)
		{	// denormalized
		exponent = 0;
		return;
		}
	while(UINTMAX_MAX/2>=mantissa)
		{
		mantissa <<= 1;
		if (1== --exponent)
			{	// denormalized
			exponent = 0;
			return;
			};
		}
	// normal form
	mantissa <<= 1;
	--exponent;
}

// \ret true if and only if action was taken
bool z_float::add_bit(const unsigned n)
{
	assert(ZAIMONI_SIZEOF_LLONG*CHAR_BIT>n);
	assert(!isinf_or_nan(*this));
	assert(!is_zero(*this));
	uintmax_t tmp = (1ULL<<n);
	uintmax_t tmp2 = UINTMAX_MAX-mantissa;
	if (tmp2>=tmp)
		{
		mantissa += tmp;
		return true;
		}
	uintmax_t need_air = tmp-tmp2;
	if (UINTMAX_MAX/2-1>exponent && (1 & need_air))
		{	// rollover is still exact if we don't overflow
		++exponent;
		mantissa = (need_air-1)/2;
		return true;
		}
	return false;
}

// \ret true if and only if action was taken
// this can fail on denormals
bool z_float::subtract_bit(const unsigned n)
{
	assert(ZAIMONI_SIZEOF_LLONG*CHAR_BIT>n);
	assert(!isinf_or_nan(*this));
	assert(!is_zero(*this));
	const uintmax_t tmp = (1ULL<<n);
	if (tmp<=mantissa)
		{	// it fits
		mantissa -= tmp;
		return true;
		}
	if (0<exponent)
		{	// use the implicit leading bit to make it work
		mantissa -= tmp;
		subtract_implicit_leading_bit();
		return true;;
		}
	return false;
}

// \ret true if and only if action was taken
bool z_float::scaled_cancel(z_float& rhs,const unsigned delta)
{
	assert(1<=delta && ZAIMONI_SIZEOF_LLONG*CHAR_BIT>delta);
	assert(!isinf_or_nan(*this));
	assert(!isinf_or_nan(rhs));
	assert(!is_zero(*this));
	assert(!is_zero(rhs));
	assert(1<=exponent);
	if (0==mantissa)
		{
		if (rhs.subtract_bit(delta-1))
			{
			--exponent;
			mantissa = UINTMAX_MAX;
			return true;
			}
		return false;
		};
	bool changed = false;
	if (uintmax_t masked_rhs = ZAIMONI_SIZEOF_LLONG*CHAR_BIT>delta ? rhs.mantissa & (UINTMAX_MAX<<delta) : 0)
		{
		uintmax_t masked_lhs = masked_rhs>>delta;
		changed = true;
		if (mantissa>=masked_lhs)
			{
			mantissa -= masked_lhs;
			rhs.mantissa -= masked_rhs;
			}
		else{
			rhs.mantissa -= (mantissa<<delta);
			mantissa = 0;
			}
		}
	if (0<mantissa)
		{
		if (rhs.subtract_bit(delta))
			{
			--mantissa;
			return true;
			}
		return changed;
		};
	if (rhs.subtract_bit(delta-1))
		{
		--exponent;
		mantissa = UINTMAX_MAX;
		return true;
		}
	return changed;
}

// this does handle denormals
void z_float::_rearrange_sum(z_float& rhs)
{
	assert(!isinf_or_nan(*this));
	assert(!isinf_or_nan(rhs));
	assert(!is_zero(*this));
	assert(!is_zero(rhs));
	// lhs should have larger absolute value
	if (exponent<rhs.exponent || (exponent==rhs.exponent && mantissa<rhs.mantissa))
		std::swap(*this,rhs);

	uintmax_t delta = exponent-rhs.exponent;
	while(ZAIMONI_SIZEOF_LLONG*CHAR_BIT+1>=delta)
		{
		if (is_negative!=rhs.is_negative)
			{
			if (0==rhs.exponent)
				{	// rhs is denormalized
				switch(delta)
				{
				case ZAIMONI_SIZEOF_LLONG*CHAR_BIT+1: return;
				case 0:
					mantissa -= rhs.mantissa;
					rhs.mantissa = 0;
					return;
				case 1:
					// denormals don't have an implicit leading 1, but do have an effective exponent one higher
					if (rhs.mantissa>mantissa) exponent = 0;	// lhs denormalizes
					mantissa -= rhs.mantissa;
					rhs.mantissa = 0;
					return;								
				default:;
					scaled_cancel(rhs,delta-1);
					return;
				}
				return;
				}
			switch(delta)
			{
			case 0:
				// implicit leading bit cancels
				subtract_implicit_leading_bit();
				rhs.subtract_implicit_leading_bit();
				if (is_zero(rhs)) return;
				break;
			case ZAIMONI_SIZEOF_LLONG*CHAR_BIT+1:
				// implicit leading 1 is just off the edge
				if (0<mantissa) return;
				--exponent;
				mantissa = UINTMAX_MAX;
				rhs.subtract_implicit_leading_bit();
				if (is_zero(rhs)) return;
				break;
			default: // implicit leading 1 is affecting somewhere in the middle of the explicit lhs mantissa
				scaled_cancel(rhs,delta);
				if (is_zero(rhs)) return;
				break;
			}
			}
		else{	// is_negative==rhs.is_negative
			if (ZAIMONI_SIZEOF_LLONG*CHAR_BIT+1==delta) return;
			if (0==rhs.exponent)
				// rhs is denormal
				switch(delta)
				{
				case ZAIMONI_SIZEOF_LLONG*CHAR_BIT:
					if (rhs.subtract_bit(ZAIMONI_SIZEOF_LLONG*CHAR_BIT-1))
						add_bit(0);
					return;
				case 0:
					if (UINTMAX_MAX-mantissa<rhs.mantissa) exponent = 1;
					mantissa += rhs.mantissa;
					rhs.mantissa = 0;
					return;
				case 1:	// denormals have the same exponent as this, they just don't have a leading 1
					{
					const uintmax_t tmp = UINTMAX_MAX-mantissa;
					if (tmp>=rhs.mantissa)
						{
						mantissa += rhs.mantissa;
						rhs.mantissa = 0;
						return;
						}
					rhs.mantissa -= tmp+1;
					}
					++exponent;
					mantissa = rhs.mantissa/2;
					rhs.mantissa &= 1;
					return;
				default:
					{
					uintmax_t rhs_mask = rhs.mantissa & (UINTMAX_MAX<<(delta-1));
					uintmax_t rhs_mask_as_lhs = rhs_mask>>(delta-1);
					uintmax_t tmp = UINTMAX_MAX-mantissa;

					if (tmp>=rhs_mask_as_lhs)
						{
						mantissa += rhs_mask_as_lhs;
						rhs.mantissa -= rhs_mask;
						if (rhs.subtract_bit(delta-1))
							add_bit(0);
						return;
						}
					rhs.mantissa -= ((tmp+1)<<(delta-1));
					++exponent;
					// ZAIMONI_SIZEOF_LLONG*CHAR_BIT>delta 
					mantissa = (rhs.mantissa>>delta);
					rhs.mantissa &= (UINTMAX_MAX>>(ZAIMONI_SIZEOF_LLONG*CHAR_BIT-delta));
					if (rhs.subtract_bit(delta))
						add_bit(0);
					return;
					}
				}
			else
				switch(delta)
				{
				case 0:
					if (UINTMAX_MAX/2-1>exponent)
						{	// 	x+x is multiply by 2
						// we have mantissa>=rhs.mantissa
						if (mantissa==rhs.mantissa)
							{
							++exponent;
							rhs.exponent = 0;
							rhs.mantissa = 0;
							return;
							};
						// mantissa>rhs.mantissa here
						uintmax_t tmp = mantissa-rhs.mantissa;
						if (!(tmp & 1))
							{
							tmp >>= 1;
							++exponent;
							mantissa -=tmp;
							rhs.exponent = 0;
							rhs.mantissa = 0;
							return;				
							}
						tmp >>= 1;
						++exponent;
						mantissa -= tmp;
						if (ZAIMONI_SIZEOF_LLONG*CHAR_BIT+1<=rhs.exponent)
							{
							rhs.exponent -= ZAIMONI_SIZEOF_LLONG*CHAR_BIT;
							rhs.mantissa = 0;
							return;
							}
						rhs.mantissa = (1ULL<<(rhs.exponent-1));
						rhs.exponent = 0;
						return;
						}
					// UINTMAX_MAX/2-1==exponent here
					{	// saturate LHS mantissa
					const uintmax_t tmp = UINTMAX_MAX-mantissa;
					if (tmp<=rhs.mantissa)
						{
						mantissa = UINTMAX_MAX;
						rhs.mantissa -= tmp;
						return;	// defer overflow
						};
					// tmp>rhs.mantissa here
					mantissa += rhs.mantissa;
					rhs.mantissa = 0;
					++mantissa;
					--rhs.exponent;
					rhs.mantissa = UINTMAX_MAX-1;
					break;
					}
				case ZAIMONI_SIZEOF_LLONG*CHAR_BIT:
					// implicit leading 1 is just off the edge
					if (UINTMAX_MAX/2-1==exponent && UINTMAX_MAX==mantissa) return;	// defer overflow
					if (0== ++mantissa) ++exponent;
					rhs.subtract_implicit_leading_bit();
					if (is_zero(rhs)) return;
					break;
				default:
					{
					// try to add in the leading bit exactly
					if (add_bit(delta))
						{
						rhs.subtract_implicit_leading_bit();
						if (is_zero(rhs)) return;
						break;
						}
					// try to add in half of the leading bit exactly, if rhs can handle the other half
					if (UINTMAX_MAX/2>=rhs.mantissa && add_bit(delta-1))
						{
						rhs.mantissa += UINTMAX_MAX/2+1;
						rhs.subtract_implicit_leading_bit();
						break;
						}
					// if we can't overflow, then the problem is parity
					if (UINTMAX_MAX/2-1>exponent)
						{
						subtract_bit(0);	// automatically succeeds as exponent is at least 2
						rhs.add_bit(ZAIMONI_SIZEOF_LLONG*CHAR_BIT-delta);
						break;
						};

					// we have UINTMAX_MAX/2-1==exponent
					if (UINTMAX_MAX==mantissa) return;	// defer overflow

					// the implicit leading bit isn't going away trivially
					{
					uintmax_t rhs_mask = rhs.mantissa & (UINTMAX_MAX<<delta);
					uintmax_t rhs_mask_as_lhs = rhs_mask>>delta;
					uintmax_t tmp = UINTMAX_MAX-mantissa;

					// max out mantissa
					if (tmp<=rhs_mask_as_lhs)
						{
						mantissa = UINTMAX_MAX;
						rhs.mantissa -= (tmp<<delta);
						return;	// defer overflow
						}
					mantissa += rhs_mask_as_lhs;
					++mantissa;
					rhs.mantissa -= rhs_mask;
					rhs.subtract_bit(ZAIMONI_SIZEOF_LLONG*CHAR_BIT-delta);
					}
					break;
					}
				}
			}
		delta = exponent-rhs.exponent;		
		}
}

int z_float::cmp_half_epsilon_of(const z_float& rhs) const
{
	assert(!isinf_or_nan(*this));
	assert(!isinf_or_nan(rhs));
	assert(!is_zero(*this));
	assert(!is_zero(rhs));
	if (rhs.exponent<=exponent) return 1;
	const uintmax_t delta = rhs.exponent-exponent;
	if (ZAIMONI_SIZEOF_LLONG*CHAR_BIT+1<delta) return -1;
	if (!isdenormal_or_zero(rhs))
		{
		if (ZAIMONI_SIZEOF_LLONG*CHAR_BIT+1==delta && 0==mantissa) return 0;
		return 1;
		}
	if (ZAIMONI_SIZEOF_LLONG*CHAR_BIT<delta) return -1;
	if (ZAIMONI_SIZEOF_LLONG*CHAR_BIT==delta && 0==mantissa) return 0;
	return zaimoni::cmp(mantissa,1ULL<<(ZAIMONI_SIZEOF_LLONG*CHAR_BIT-1-delta));
}

z_float& z_float::operator+=(z_float rhs)
{
	if (issnan(rhs))
		{	// invalid operation: trap if possible, otherwise downgrade to qNaN
		if (traps[Z_FLOAT_INVALID] && (traps[Z_FLOAT_INVALID])(*this,rhs,*this,(1<<(Z_FLOAT_INVALID+2))+_rounding_mode(),Z_FLOAT_CODE_ADD,NULL));
			return *this;
		_modes |= 1<<(Z_FLOAT_INVALID+2);
		mantissa &= ~(1ULL);	// we no longer trap
		}
	else if (issnan(*this))
		{
		if (traps[Z_FLOAT_INVALID] && (traps[Z_FLOAT_INVALID])(*this,rhs,*this,(1<<(Z_FLOAT_INVALID+2))+_rounding_mode(),Z_FLOAT_CODE_ADD,NULL));
			return *this;
		_modes |= 1<<(Z_FLOAT_INVALID+2);
		mantissa &= ~(1ULL);	// we no longer trap
		};

	if (isnan(rhs))
		{
		if (isnan(*this))
			mantissa |= rhs.mantissa;
		else
			*this = rhs;
		return *this;
		}
	if (isnan(*this)) return *this;

	// infinity+(-infinity) and (-infinity)+infinity are invalid
	// infinity+x is infinity
	// (-infinity)+x is -infinity
	if (isinf(*this))
		{
		if (isinf(rhs))
			{	// +-infinity+(+-infinity)
			if (is_negative==rhs.is_negative) return *this;
			if (traps[Z_FLOAT_INVALID] && (traps[Z_FLOAT_INVALID])(*this,rhs,*this,(1<<(Z_FLOAT_INVALID+2))+_rounding_mode(),Z_FLOAT_CODE_ADD,NULL))
				return *this;
			_modes |= 1<<(Z_FLOAT_INVALID+2);
			exponent = UINTMAX_MAX/2U;
			mantissa = Z_FLOAT_NAN_INF_NEGINF_ADD;
			return *this;
			}
		// +-infinity+x
		return *this;
		}
	else if (isinf(rhs))
		// x+(+/-infinity)
		return *this = rhs;

	// Cf. IEEE-754 6.3
	// 0+-0 is 0
	// -0+-0 is -0
	// x+-0 is x
	if (is_zero(rhs)) return *this;
	// +-0+x is x
	if (is_zero(*this))
		return *this = rhs;

	// exact arithmetic check
	_rearrange_sum(rhs);
	if (is_zero(rhs)) return *this;

	if (is_negative==rhs.is_negative)
		// same sign
		return IEEE_round_from_infinity(_rounding_mode(),z_float(*this),rhs,Z_FLOAT_CODE_ADD,rhs.cmp_half_epsilon_of(*this));
	return IEEE_round_from_zero(_rounding_mode(),z_float(*this),rhs,Z_FLOAT_CODE_ADD,rhs.cmp_half_epsilon_of(*this));
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
