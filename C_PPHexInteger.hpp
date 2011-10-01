// C_PPHexInteger.hpp
// (C)2009 Kenneth Boyd, license: MIT.txt

#ifndef C_PPHEXINTEGER_HPP
#define C_PPHEXINTEGER_HPP 1

#include "Zaimoni.STL/Logging.h"
#include "Zaimoni.STL/pure.C/format_util.h"
#include "C_PPNumCore.hpp"

struct C_PPHexInteger : public C_PPIntCore
{
	static bool is(const char* x,size_t token_len)
		{
		assert(NULL!=x);
		assert(0<token_len);
		C_PPHexInteger target;
		return is(x,token_len,target);
		};
	static bool is(const char* x,size_t token_len,C_PPHexInteger& target);

	uintmax_t bits_required() const;

	// inherits errno weirdness
	uintmax_t to_umax() {return z_atoumax(ptr,16);};	
	intmax_t to_imax() {return imax_from_umax(is_negative,z_atoumax(ptr,16));};	// signals that CRTP is appropriate
};

// usual: -1 <, 0 ==, 1 >
int cmp(const C_PPHexInteger& LHS, const C_PPHexInteger& RHS);
inline int cmp(const C_PPHexInteger& LHS, const char* const RHS, size_t RHS_len)
{
	C_PPHexInteger RHS2;
	SUCCEED_OR_DIE(C_PPHexInteger::is(RHS,RHS_len,RHS2));
	return cmp(LHS,RHS2);
}

inline int cmp(const char* const LHS, size_t LHS_len, const C_PPHexInteger& RHS)
{
	C_PPHexInteger LHS2;
	SUCCEED_OR_DIE(C_PPHexInteger::is(LHS,LHS_len,LHS2));
	return cmp(LHS2,RHS);
}

#endif
