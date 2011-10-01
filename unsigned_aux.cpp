// unsigned_aux.cpp
// (C)2009 Kenneth Boyd, license: MIT.txt

#include "unsigned_aux.hpp"
#include <string.h>
#include <stdlib.h>

#ifndef FAST_ROUTE
#define FAST_ROUTE 2<=(UINT_MAX/UCHAR_MAX)
// #define FAST_ROUTE 0
#endif

//! \todo rethink this for hosting machines where sizeof(char)==sizeof(uintmax_t)
void _unsigned_copy(unsigned char* x, uintmax_t src, unsigned int i)
{
	do	{
		--i;
		x[i] = ((src & ((uintmax_t)(UCHAR_MAX)<<(i*CHAR_BIT)))>>(i*CHAR_BIT));
		}
	while(0<i);
}

void _mask_to(unsigned char* x, size_t x_len, size_t bitcount)
{
	const size_t target_bytes = bitcount/CHAR_BIT;
	const size_t target_bits = bitcount%CHAR_BIT;
	if (target_bytes>=x_len) return;
	if (0==target_bits)
		memset(x+target_bytes,0,x_len-target_bytes);
	else{
		if (target_bytes+1U<x_len)
			memset(x+target_bytes+1U,0,x_len-target_bytes-1U);
		x[target_bytes] &= (UCHAR_MAX>>(CHAR_BIT-target_bits));
		}
}

void _unsigned_sum(unsigned char* lhs, size_t lhs_len, const unsigned char* rhs)
{
	size_t i = 0;
#if FAST_ROUTE
	unsigned int tmp = 0;
	do	{
		tmp += lhs[i];
		tmp += rhs[i];
		lhs[i] = (tmp & UCHAR_MAX);
		tmp >>= CHAR_BIT;
		}
	while(lhs_len > ++i);
#else
	bool carry = false;
	do	{
		if (carry && UCHAR_MAX>lhs[i])
			{
			lhs[i] += 1;
			carry = false;
			}

		if (carry)
			lhs[i] = rhs[i];
		else if (UCHAR_MAX-lhs[i]>=rhs[i])
			lhs[i] += rhs[i];
		else{
			lhs[i] = rhs[i]-(UCHAR_MAX-lhs[i])
			lhs[i] -= 1;
			carry = true;
			}
		}
	while(lhs_len > ++i);
#endif
}

void _unsigned_sum(unsigned char* lhs, size_t lhs_len, uintmax_t rhs)
{
	size_t i = 0;
#if FAST_ROUTE
	unsigned int tmp = 0;
	do	{
		tmp += lhs[i];
		tmp += (rhs & UCHAR_MAX);
		lhs[i] = (tmp & UCHAR_MAX);
		tmp >>= CHAR_BIT;
		rhs >>= CHAR_BIT;
		}
	while(lhs_len > ++i && (rhs || tmp));
#else
	bool carry = false;
	do	{
		if (carry && UCHAR_MAX>lhs[i])
			{
			lhs[i] += 1;
			carry = false;
			}

		const unsigned char rhs_image = (rhs & UCHAR_MAX);
		rhs >>= CHAR_BIT;
		if (carry)
			lhs[i] = rhs_image;
		else if (UCHAR_MAX-lhs[i]>=rhs_image)
			lhs[i] += rhs_image;
		else{
			lhs[i] = rhs_image-(UCHAR_MAX-lhs[i])
			lhs[i] -= 1;
			carry = true;
			}
		}
	while(lhs_len > ++i && rhs);
#endif
}

void _unsigned_diff(unsigned char* lhs, size_t lhs_len, const unsigned char* rhs)
{
	size_t i = 0;
	bool carry = false;
	do	{
		if (carry)
			{
			lhs[i] -= 1;
			carry = (UCHAR_MAX == lhs[i]);
			};

		carry = carry ||  lhs[i]<rhs[i];
		lhs[i] -= rhs[i];
		}
	while(lhs_len > ++i);
}

void _unsigned_diff(unsigned char* lhs, size_t lhs_len, uintmax_t rhs)
{
	size_t i = 0;
	bool carry = false;
	do	{
		const unsigned char rhs_image = rhs;
		rhs >>= CHAR_BIT;
		if (carry)
			{
			lhs[i] -= 1;
			carry = (UCHAR_MAX == lhs[i]);
			};

		carry = carry || lhs[i]<rhs_image;
		lhs[i] -= rhs_image;
		}
	while(lhs_len > ++i && (rhs || carry));
}

unsigned int _int_log2(const unsigned char* const x, size_t x_len)
{
	while(0<x_len)
		{
		size_t i = CHAR_BIT;
		--x_len;
		do	if (x[x_len] & (1U<< --i)) return x_len*CHAR_BIT+i;
		while(0<i);
		};
	return 0;
}

void _bitwise_compl(unsigned char* x, size_t x_len)
{
	while(0<x_len)
		{
		--x_len;
		x[x_len] = ~x[x_len];
		};
}

void _bitwise_and(unsigned char* lhs, size_t lhs_len, const unsigned char* rhs)
{
	while(0<lhs_len)
		{
		--lhs_len;
		lhs[lhs_len] &= rhs[lhs_len];
		};
}

void _bitwise_xor(unsigned char* lhs, size_t lhs_len, const unsigned char* rhs)
{
	while(0<lhs_len)
		{
		--lhs_len;
		lhs[lhs_len] ^= rhs[lhs_len];
		};
}

void _bitwise_or(unsigned char* lhs, size_t lhs_len, const unsigned char* rhs)
{
	while(0<lhs_len)
		{
		--lhs_len;
		lhs[lhs_len] |= rhs[lhs_len];
		};
}

void _unsigned_mult(unsigned char* buf, const size_t buf_len, const unsigned char* lhs, size_t lhs_len, const unsigned char* rhs, size_t rhs_len)
{
	memset(buf,0,buf_len);
	// trim off leading zeros
	while(2<=rhs_len && 0==rhs[rhs_len-1]) --rhs_len;
	while(2<=lhs_len && 0==lhs[lhs_len-1]) --lhs_len;
	if (1==lhs_len && 0==lhs[0]) return;	// multiply by 0 is 0
	if (1==rhs_len && 0==rhs[0]) return;
#if FAST_ROUTE
	size_t k = 0;
	unsigned int tmp = 0;
	unsigned int tmp2 = 0;
	do	{
		if (lhs_len+rhs_len-2U>=k)
			{
			size_t i = k+1;
			do	{
				if (lhs_len<= --i || rhs_len<=k-i) continue;
				tmp += (unsigned int)(lhs[i])*(unsigned int)(rhs[k-i]);	// exploits: UCHAR_MAX*UCHAR_MAX-1 = (UCHAR_MAX+1)*(UCHAR_MAX-1)
				tmp2 += (tmp >> CHAR_BIT);
				tmp &= UCHAR_MAX;
				}
			while(0<i);
			}
		buf[k] = tmp;
		tmp = tmp2;
		tmp2 = (tmp >> CHAR_BIT);
		tmp &= UCHAR_MAX;
		}
	while(buf_len> ++k);
#else
#error _unsigned_mult(unsigned char* buf, const size_t buf_len, const unsigned char* lhs, const size_t lhs_len, const unsigned char* rhs, const size_t rhs_len) not implemented
#endif
}

void
_unsigned_right_shift(unsigned char* x, size_t x_len, uintmax_t bit_right_shift)
{
	if (0==bit_right_shift) return;
	const uintmax_t whole_bytes = bit_right_shift/CHAR_BIT;
	if (x_len<=whole_bytes)
		{
		memset(x,0,x_len);
		return;
		}

	const unsigned int left_over_bits = bit_right_shift%CHAR_BIT;
	const size_t content_span = x_len-(size_t)(whole_bytes);
	if (0==left_over_bits)
		{
		memmove(x,x+whole_bytes,content_span);
		memset(x+content_span,0,whole_bytes);
		return;
		};

	const size_t content_span_sub1 = content_span-1;
	size_t i = 0;
	if (0<whole_bytes)
		{
		while(content_span_sub1>i)
			{
			x[i] = (x[i+whole_bytes]>>left_over_bits);
			x[i] += ((x[i+whole_bytes+1]%(1U<<left_over_bits))<<(CHAR_BIT-left_over_bits));
			++i;
			};
		x[content_span_sub1] = (x[content_span_sub1+whole_bytes]>>left_over_bits);
		memset(x+content_span,0,whole_bytes);
		}
	else{
		while(content_span_sub1>i)
			{
			x[i] >>= left_over_bits;
			x[i] += ((x[i+1]%(1U<<left_over_bits))<<(CHAR_BIT-left_over_bits));
			++i;
			};
		x[content_span_sub1] = (x[content_span_sub1+whole_bytes]>>left_over_bits);
		}
}

void
_unsigned_left_shift(unsigned char* x, size_t x_len, uintmax_t bit_left_shift)
{
	if (0==bit_left_shift) return;
	const uintmax_t whole_bytes = bit_left_shift/CHAR_BIT;
	if (x_len<=whole_bytes)
		{
		memset(x,0,x_len);
		return;
		}

	const unsigned int left_over_bits = bit_left_shift%CHAR_BIT;
	const size_t content_span = x_len-(size_t)(whole_bytes);
	if (0==left_over_bits)
		{
		memmove(x+whole_bytes,x,content_span);
		memset(x,0,content_span);
		return;
		};

	size_t i = content_span-1U;
	if (0<whole_bytes)
		{
		while(0<i)
			{
			x[i+whole_bytes] = ((x[i]%(1U<<(CHAR_BIT-left_over_bits)))<<left_over_bits);
			x[i+whole_bytes] += (x[i-1]>>(CHAR_BIT-left_over_bits));
			--i;
			};
		x[whole_bytes] = ((x[0]%(1U<<(CHAR_BIT-left_over_bits)))<<left_over_bits);
		memset(x,0,whole_bytes);
		}
	else{
		while(0<i)
			{
			(x[i] %= (1U<<(CHAR_BIT-left_over_bits)))<<=left_over_bits;
			x[i] += (x[i-1]>>(CHAR_BIT-left_over_bits));
			--i;
			};
		(x[0] %= (1U<<(CHAR_BIT-left_over_bits)))<<=left_over_bits;
		}
}

int
_unsigned_cmp(const unsigned char* lhs, size_t lhs_len, const unsigned char* rhs)
{	// reverse memcmp
	do	{
		--lhs_len;
		if (lhs[lhs_len]<rhs[lhs_len]) return -1;
		if (lhs[lhs_len]>rhs[lhs_len]) return 1;
		}
	while(0<lhs_len);
	return 0;
}

int _unsigned_cmp(const unsigned char* lhs, size_t lhs_len, uintmax_t rhs)
{	// reverse memcmp
	do	{
		--lhs_len;
		const unsigned char rhs_image = ((rhs & (uintmax_t)(UCHAR_MAX)<<(lhs_len*CHAR_BIT))>>(lhs_len*CHAR_BIT));
		if (lhs[lhs_len]<rhs_image) return -1;
		if (lhs[lhs_len]>rhs_image) return 1;
		}
	while(0<lhs_len);
	return 0;
}

uintmax_t _to_uint(const unsigned char* x, size_t x_len)
{
	uintmax_t tmp = x[--x_len];
	while(0<x_len)
		{
		tmp <<= CHAR_BIT;
		tmp += x[--x_len];
		};
	return tmp;
}

void _remainder_quotient(const size_t buf_len,unsigned char* dividend_remainder,const unsigned char* divisor,unsigned char* quotient)
{
	memset(quotient,0,buf_len);
	SUCCEED_OR_DIE(1==_unsigned_cmp(divisor,buf_len,quotient));
	if (0==_unsigned_cmp(dividend_remainder,buf_len,quotient)) return;
	unsigned char* interim = reinterpret_cast<unsigned char*>(malloc(2*buf_len));
	SUCCEED_OR_DIE(NULL!=interim);
	while(0<=_unsigned_cmp(dividend_remainder,buf_len,divisor))
		{
		// interim: scaled_quotient
		// interim+N: scale
		memmove(interim,divisor,buf_len);
		memset(interim+buf_len,0,buf_len);
		interim[buf_len] = 1;
		int test = _unsigned_cmp(dividend_remainder,buf_len,interim);
		while(0<=test)
			{
			if (0==test)
				{
				_unsigned_sum(quotient,buf_len,interim+buf_len);
				memset(dividend_remainder,0,buf_len);
				free(interim);
				return;
				}
			if (interim[buf_len-1] & (1U<<(CHAR_BIT-1))) break;
			_unsigned_left_shift(interim,buf_len,1);
			assert(1==_unsigned_cmp(interim,buf_len,divisor));
			test = _unsigned_cmp(dividend_remainder,buf_len,interim);
			if (0>test)
				{
				_unsigned_right_shift(interim,buf_len,1);
				break;
				}
			_unsigned_left_shift(interim+buf_len,buf_len,1);
			}
		_unsigned_sum(quotient,buf_len,interim+buf_len);
		_unsigned_diff(dividend_remainder,buf_len,interim);
		}
	free(interim);
}

