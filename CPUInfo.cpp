// CPUInfo.cpp
// (C)2009,2010 Kenneth Boyd, license: MIT.txt

#include "CPUInfo.hpp"
#include <algorithm>

namespace virtual_machine {

#define C_sizeof_char() 1

	// names of macro parameters match names of constructor variables
	// target wchar_t is assumed to be an unsigned integral type
	// we first try to choose the smallest type that can represent a 32-bit UNICODE point
	// if that fails, we try to choose the smallest type that can represent plane 16
	// align with std_in_enum above
#define SELECT_TARGET_WCHAR_T(char_bit,sizeof_short,sizeof_int,sizeof_long,sizeof_long_long)	\
	((32U<=char_bit) ? 1 :	\
	(32<=char_bit*sizeof_short) ? 2 :	\
	(32<=char_bit*sizeof_int) ? 3 :	\
	(32<=char_bit*sizeof_long) ? 4 :	\
	(32<=char_bit*sizeof_long_long) ? 5 :	\
	(21U<=char_bit) ? 1+8 :	\
	(21<=char_bit*sizeof_short) ? 2+8 :	\
	(21<=char_bit*sizeof_int) ? 3+8 :	\
	(21<=char_bit*sizeof_long) ? 4+8 :	\
	(21<=char_bit*sizeof_long_long) ? 5+8 :	\
	0)

CPUInfo::CPUInfo(unsigned short _char_bit, unsigned short _sizeof_short, unsigned short _sizeof_int, unsigned short _sizeof_long, unsigned short _sizeof_long_long, signed_int_rep _signed_int_representation, bool _char_is_signed_char, std_int_enum _ptrdiff_type)
: char_bit(_char_bit),sizeof_short(_sizeof_short),sizeof_int(_sizeof_int),sizeof_long(_sizeof_long),sizeof_long_long(_sizeof_long_long),
  signed_int_representation(_signed_int_representation + 4 * _char_is_signed_char + 8 * SELECT_TARGET_WCHAR_T(char_bit, sizeof_short, sizeof_int, sizeof_long, sizeof_long_long) + 128 * _ptrdiff_type)
{
	unsigned_var_int tmp;

#define SET_MAXIMUM(A)	\
	tmp.set_bitcount(C_bit<std_int_long_long>()+1);	\
	tmp.clear();	\
	tmp.set((SUCCEED_OR_DIE(C_bit<std_int_long_long>()>=C_char_bit()*C_sizeof_##A()),C_char_bit()*C_sizeof_##A()));	\
	tmp -= 1;	\
	tmp.set_bitcount(C_bit<std_int_long_long>());	\
	tmp.MoveInto(unsigned_maxima[std_int_##A-1])

	SET_MAXIMUM(char);
	SET_MAXIMUM(short);
	SET_MAXIMUM(int);
	SET_MAXIMUM(long);
	SET_MAXIMUM(long_long);

#undef SET_MAXIMUM

	size_t i = 0;
	do	(signed_maxima[i] = unsigned_maxima[i]) >>= 1;
	while (std_int_long_long > ++i);
}

#undef SELECT_TARGET_WCHAR_T

bool CPUInfo::trap_int(const umaxint& src_int,std_int_enum machine_type) const
{
	switch(machine_type)
	{
	default: return false;
	case std_int_int:
	case std_int_long:
	case std_int_long_long:;
	}
	const unsigned int bitcount = C_bit(machine_type);
	const int target_bytecount = bitcount/CHAR_BIT;
	const unsigned int target_bitcount = bitcount%CHAR_BIT;
	assert(C_bit<std_int_long_long>()>=bitcount && 1<=bitcount);

	switch(C_signed_int_representation())
	{
	case ones_complement:	{	// bitwise all-ones may be trap (-0)
							if (0<target_bytecount && target_bytecount>std::count(src_int.data(),src_int.data()+target_bytecount,UCHAR_MAX)) return false;
							return 0==target_bitcount || (UCHAR_MAX>>(CHAR_BIT-target_bitcount))==((UCHAR_MAX>>(CHAR_BIT-target_bitcount)) & src_int.data()[target_bytecount]);
							}
	case twos_complement:		// sign bit only set may be trap -(2^N)
	case sign_and_magnitude:{	// sign bit only set may be trap (-0)
							if (0==target_bitcount)
								{
								if (1<target_bytecount && target_bytecount-1>std::count(src_int.data(),src_int.data()+(target_bytecount-1U),0)) return false;
								return (1U<<(CHAR_BIT-1))==src_int.data()[target_bytecount-1];
								}
							else{
								if (0<target_bytecount && target_bytecount>std::count(src_int.data(),src_int.data()+target_bytecount,0)) return false;
								return (1U<<(CHAR_BIT-1-target_bitcount))==((UCHAR_MAX>>(CHAR_BIT-target_bitcount)) & src_int.data()[target_bytecount]);
								}
							}
	}
	return false;
}

void CPUInfo::signed_additive_inverse(umaxint& src_int,std_int_enum machine_type) const
{
	assert(machine_type);
	const int signed_int_rep = C_signed_int_representation();
	if (sign_and_magnitude==signed_int_rep)
		{
		src_int.toggle(C_bit(machine_type)-1);
		return;
		}
	umaxint tmp(unsigned_max(machine_type));
	tmp -= src_int;
	if (twos_complement==signed_int_rep)
		{
		tmp += 1;
		tmp.mask_to(C_bit(machine_type));
		}
	tmp.MoveInto(src_int);
}

void CPUInfo::unsigned_additive_inverse(umaxint& src_int,std_int_enum machine_type) const
{
	assert(machine_type);
	assert(src_int<=unsigned_max(machine_type));
	umaxint tmp(0,src_int.size());
	tmp -= src_int;
	tmp.mask_to(C_bit(machine_type));
	tmp.MoveInto(src_int);
}

bool CPUInfo::is_zero(const unsigned char* x, size_t x_len, const promotion_info& targettype) const
{
	assert(x);
	assert(0<x_len);
	assert(x_len*CHAR_BIT>=targettype.bitcount);
	if (targettype.is_signed && ((1U<<((targettype.bitcount-1)%CHAR_BIT)) & x[(targettype.bitcount-1)/CHAR_BIT]))
		{	// negative
		switch(C_signed_int_representation())
		{
		default: return false;
		case sign_and_magnitude:
			x_len = (targettype.bitcount-1)/CHAR_BIT;
			if (1U<<((targettype.bitcount-1)%CHAR_BIT) != x[x_len]) return false;
			while(0<x_len)
				if (0!=x[--x_len]) return false;
			return true;
		case ones_complement:
			x_len = (targettype.bitcount-1)/CHAR_BIT;
			if (UCHAR_MAX>>((CHAR_BIT-1)-(targettype.bitcount-1)%CHAR_BIT) != x[x_len]) return false;
			while(0<x_len)
				if (UCHAR_MAX!=x[--x_len]) return false;
			return true;
		}
		}
	// positive
	do	if (0!=x[--x_len]) return false;
	while(0<x_len);
	return true;
}


bool CPUInfo::C_promote_integer(umaxint& x,const promotion_info& src_type, const promotion_info& dest_type) const
{
	if (src_type.is_signed && x.test(src_type.bitcount-1))
		{
		if (!dest_type.is_signed)
			// unsigned integer result: C99 6.3.1.3p2 dictates modulo conversion to unsigned
			C_cast_signed_to_unsigned(x,src_type.machine_type);
		else if (dest_type.bitcount>src_type.bitcount)
			sign_extend(x,src_type.machine_type,dest_type.machine_type);
		};
	return dest_type.is_signed && x.test(dest_type.bitcount-1);
}

}	// end namespace virtual_machine

#undef C_sizeof_char
