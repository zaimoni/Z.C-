// CPreproc_autogen_pp.cpp
// (C)2009,2010 Kenneth Boyd, license: MIT.txt
// class CPreprocessor support for autogenerating headers for arbitrary machine targets.

#include "CPreproc_pp.hpp"
#include "CPUInfo.hpp"
#include "errors.hpp"
#include "Zaimoni.STL/POD.hpp"
#include "Zaimoni.STL/LexParse/Token.hpp"
#include "Zaimoni.STL/pure.C/format_util.h"

using virtual_machine::umaxint;

// XXX adjust VM_MAX_BIT_PLATFORM to work inside of CPreprocessor XXX
#undef VM_MAX_BIT_PLATFORM
#define VM_MAX_BIT_PLATFORM target_machine.C_bit<virtual_machine::std_int_long_long>()

//! \todo POSIX support as feasible
static const char* const limits_h_reserved[]
	=	{	"CHAR_BIT",
			"SCHAR_MIN",
			"SCHAR_MAX",
			"UCHAR_MAX",
			"CHAR_MIN",
			"CHAR_MAX",
			"MB_LEN_MAX",
			"SHRT_MIN",
			"SHRT_MAX",
			"USHRT_MAX",
			"INT_MIN",
			"INT_MAX",
			"UINT_MAX",
			"LONG_MIN",
			"LONG_MAX",
			"ULONG_MAX",
			"LLONG_MIN",
			"LLONG_MAX",
			"ULLONG_MAX"
		};

static const char* const limits_h_POSIX_reserved[]
	=	{	"WORD_BIT",
			"LONG_BIT"
		};

static const zaimoni::POD_pair<const char*,size_t> limits_h_core[]
	=	{	DICT_STRUCT("#ifndef __LIMITS_H__"),
			DICT_STRUCT("#define __LIMITS_H__ 1"),
			DICT_STRUCT("#pragma ZCC lock __LIMITS_H__"),
			DICT_STRUCT("#define CHAR_BIT"),
			DICT_STRUCT("#define SCHAR_MIN -"),
			DICT_STRUCT("#define SCHAR_MAX"),
			DICT_STRUCT("#define UCHAR_MAX"),
			DICT_STRUCT("#define CHAR_MIN"),
			DICT_STRUCT("#define CHAR_MAX"),
			DICT_STRUCT("#define MB_LEN_MAX"),
			DICT_STRUCT("#define SHRT_MIN -"),
			DICT_STRUCT("#define SHRT_MAX"),
			DICT_STRUCT("#define USHRT_MAX"),
			DICT_STRUCT("#define INT_MIN -"),
			DICT_STRUCT("#define INT_MAX"),
			DICT_STRUCT("#define UINT_MAX"),
			DICT_STRUCT("#define LONG_MIN -"),
			DICT_STRUCT("#define LONG_MAX"),
			DICT_STRUCT("#define ULONG_MAX"),
			DICT_STRUCT("#define LLONG_MIN "),
			DICT_STRUCT("#define LLONG_MAX"),
			DICT_STRUCT("#define ULLONG_MAX"),
// LONG_BIT and WORD_BIT do not require POSIX library features to make sense, and do not appear version-sensitive
// _XOPEN_SOURCE implies _POSIX_C_SOURCE
// _POSIX_SOURCE is the POSIX 1 equivalent of _POSIX_C_SOURCE in POSIX 2/3.  ZCC supports this in spite of POSIX 3.
			DICT_STRUCT("#if defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE)"),
			DICT_STRUCT("#define WORD_BIT"),
			DICT_STRUCT("#define LONG_BIT"),
			DICT_STRUCT("#endif"),
			DICT_STRUCT("#endif")
		};

#define LIMITS_CHAR_BIT_LINE 3
#define LIMITS_SCHAR_MIN_LINE 4
#define LIMITS_SCHAR_MAX_LINE 5
#define LIMITS_UCHAR_MAX_LINE 6
#define LIMITS_CHAR_MIN_LINE 7
#define LIMITS_CHAR_MAX_LINE 8
#define LIMITS_MB_LEN_MAX_LINE 9
#define LIMITS_SHRT_MIN_LINE 10
#define LIMITS_SHRT_MAX_LINE 11
#define LIMITS_USHRT_MAX_LINE 12
#define LIMITS_INT_MIN_LINE 13
#define LIMITS_INT_MAX_LINE 14
#define LIMITS_UINT_MAX_LINE 15
#define LIMITS_LONG_MIN_LINE 16
#define LIMITS_LONG_MAX_LINE 17
#define LIMITS_ULONG_MAX_LINE 18
#define LIMITS_LLONG_MIN_LINE 19
#define LIMITS_LLONG_MAX_LINE 20
#define LIMITS_ULLONG_MAX_LINE 21
#define LIMITS_WORD_BIT_LINE 23
#define LIMITS_LONG_BIT_LINE 24

#define LIMITS_INJECT_LOCK (LIMITS_LONG_BIT_LINE+2)
#define LIMITS_INJECT_REALITY LIMITS_CHAR_BIT_LINE
#define LIMITS_POSIX_INJECT_LOCK (LIMITS_LONG_BIT_LINE+1)
#define LIMITS_POSIX_INJECT_REALITY LIMITS_WORD_BIT_LINE

static const char* const stddef_h_reserved[]
	=	{	"wchar_t",	// C only
			"ptrdiff_t",	// common to C, C++
			"size_t",
			"NULL",
			"offsetof",
			"std"			// C++ only
		};

//! \todo option --deathstation, with supporting predefine : do not provide convenient, legal but not required features
// NOTE: wchar_t is a reserved keyword in C++, do not typedef it!
static const zaimoni::POD_pair<const char*,size_t> stddef_h_core[]
	=	{	DICT_STRUCT("#ifndef __STDDEF_H__"),
			DICT_STRUCT("#define __STDDEF_H__ 1"),
			DICT_STRUCT("#pragma ZCC lock __STDDEF_H__"),
			DICT_STRUCT("typedef "),
			DICT_STRUCT("typedef "),
			DICT_STRUCT("#ifndef __cplusplus "),
			DICT_STRUCT("#pragma ZCC lock wchar_t"),
			DICT_STRUCT("typedef "),
			DICT_STRUCT("#endif"),
			DICT_STRUCT("#define NULL "),
//			DICT_STRUCT("#define offsetof"),	// do not provide offsetof as we don't parse structs yet
			DICT_STRUCT("#pragma ZCC lock ptrdiff_t size_t NULL offsetof"),	// lock offsetof because it's standard
			DICT_STRUCT("#ifdef __cplusplus"),
			DICT_STRUCT("#pragma ZCC lock std"),
			DICT_STRUCT("namespace std {"),
			DICT_STRUCT("typedef "),
			DICT_STRUCT("typedef "),
			DICT_STRUCT("}"),
			DICT_STRUCT("#endif"),
			DICT_STRUCT("#endif"),
		};

#define STDDEF_PTRDIFF_T_LINE 3
#define STDDEF_SIZE_T_LINE 4
#define STDDEF_WCHAR_T_LINE 7
#define STDDEF_NULL_LINE 9
#define STDDEF_CPP_PTRDIFF_T_LINE 14
#define STDDEF_CPP_SIZE_T_LINE 15

#define STDDEF_INJECT_REALITY STDDEF_PTRDIFF_T_LINE
#define STDDEF_INJECT_C_REALITY (STDDEF_WCHAR_T_LINE-1)
#define STDDEF_INJECT_CPP_REALITY (STDDEF_CPP_PTRDIFF_T_LINE-2)

static const zaimoni::POD_pair<const char*,size_t> stdint_h_core[]
	=	{	DICT_STRUCT("#ifndef __STDINT_H__"),
			DICT_STRUCT("#define __STDINT_H__ 1"),
			DICT_STRUCT("#pragma ZCC lock __STDINT_H__"),
			// exact-width types
			DICT_STRUCT("typedef unsigned char uint"),
			DICT_STRUCT("typedef unsigned short uint"),
			DICT_STRUCT("typedef unsigned int uint"),
			DICT_STRUCT("typedef unsigned long uint"),
			DICT_STRUCT("typedef unsigned long long uint"),
			DICT_STRUCT("typedef signed char int"),	// these die on non-two's complement machines
			DICT_STRUCT("typedef short int"),
			DICT_STRUCT("typedef int int"),
			DICT_STRUCT("typedef long int"),
			DICT_STRUCT("typedef long long int"),
			// their widths
			DICT_STRUCT("#define INT"),
			DICT_STRUCT("#define INT"),
			DICT_STRUCT("#define UINT"),
			DICT_STRUCT("#define INT"),
			DICT_STRUCT("#define INT"),
			DICT_STRUCT("#define UINT"),
			DICT_STRUCT("#define INT"),
			DICT_STRUCT("#define INT"),
			DICT_STRUCT("#define UINT"),
			DICT_STRUCT("#define INT"),
			DICT_STRUCT("#define INT"),
			DICT_STRUCT("#define UINT"),
			DICT_STRUCT("#define INT"),
			DICT_STRUCT("#define INT"),
			DICT_STRUCT("#define UINT"),
			// uintptr_t, intptr_t
			DICT_STRUCT("typedef "),
			DICT_STRUCT("typedef "),
			// their limits
			DICT_STRUCT("#define INTPTR_MIN "),
			DICT_STRUCT("#define INTPTR_MAX"),
			DICT_STRUCT("#define UINTPTR_MAX"),
			DICT_STRUCT("#pragma ZCC lock INTPTR_MIN INTPTR_MAX UINTPTR_MAX"),
			// intmax_t, uintmax_t
			DICT_STRUCT("typedef long long intmax_t;"),
			DICT_STRUCT("typedef unsigned long long uintmax_t;"),
			// their limits
			DICT_STRUCT("#define INTMAX_MIN "),
			DICT_STRUCT("#define INTMAX_MAX"),
			DICT_STRUCT("#define UINTMAX_MAX"),
			// adapter macros
			DICT_STRUCT("#define INTMAX_C(A) A##LL"),
			DICT_STRUCT("#define UINTMAX_C(A) A##ULL"),
			DICT_STRUCT("#pragma ZCC lock INTMAX_MIN INTMAX_MAX UINTMAX_MAX INTMAX_C UINTMAX_C"),
			//! \todo: enforce dec/octal/hex constant restriction on INTMAX_C/UINTMAX_C
			// perhaps new pragma
			// #pragma ZCC param_enforce "C99 7.18.4p2" INTMAX_C 0 decimal_literal octal_literal hexadecimal_literal
			// #pragma ZCC param_enforce "C99 7.18.4p2" UINTMAX_C 0 decimal_literal octal_literal hexadecimal_literal
			// eventually also allow: char_literal str_literal narrow_char_literal narrow_str_literal wide_char_literal wide_str_literal
			// limits of stddef.h, etc. types; do not provide sig_atomic_t or wint_t macros as we don't have those
			DICT_STRUCT("#define PTRDIFF_MIN "),
			DICT_STRUCT("#define PTRDIFF_MAX"),
			DICT_STRUCT("#define SIZE_MAX"),
			DICT_STRUCT("#define WCHAR_MIN 0"),
			DICT_STRUCT("#define WCHAR_MAX"),
			// least, fast types: autospawn them all
			// C++ also needs typedefs in namespace std
			DICT_STRUCT("#ifdef __cplusplus"),
			DICT_STRUCT("namespace std {"),
			// exact-width types
			DICT_STRUCT("typedef unsigned char uint"),
			DICT_STRUCT("typedef unsigned short uint"),
			DICT_STRUCT("typedef unsigned int uint"),
			DICT_STRUCT("typedef unsigned long uint"),
			DICT_STRUCT("typedef unsigned long long uint"),
			DICT_STRUCT("typedef signed char int"),	// these die on non-two's complement machines
			DICT_STRUCT("typedef short int"),
			DICT_STRUCT("typedef int int"),
			DICT_STRUCT("typedef long int"),
			DICT_STRUCT("typedef long long int"),
			// uintptr_t, intptr_t
			DICT_STRUCT("typedef "),
			DICT_STRUCT("typedef "),
			// intmax_t, uintmax_t
			DICT_STRUCT("typedef long long intmax_t;"),
			DICT_STRUCT("typedef unsigned long long uintmax_t;"),
			// least, fast types: autospawn them all
			DICT_STRUCT("}"),
			DICT_STRUCT("#endif"),
			DICT_STRUCT("#endif"),
		};

#define STDINT_EXACT_SCHAR_OFFSET 5
#define STDINT_EXACT_UCHAR_OFFSET 0
#define STDINT_EXACT_SHRT_OFFSET 6
#define STDINT_EXACT_USHRT_OFFSET 1
#define STDINT_EXACT_INT_OFFSET 7
#define STDINT_EXACT_UINT_OFFSET 2
#define STDINT_EXACT_LONG_OFFSET 8
#define STDINT_EXACT_ULONG_OFFSET 3
#define STDINT_EXACT_LLONG_OFFSET 9
#define STDINT_EXACT_ULLONG_OFFSET 4

#define STDINT_SMIN_OFFSET 0
#define STDINT_SMAX_OFFSET 1
#define STDINT_UMAX_OFFSET 2

#define STDINT_EXACT_LINEORIGIN 3
#define STDINT_EXACT_CHAR_LIMITS_LINEORIGIN 13
#define STDINT_EXACT_SHRT_LIMITS_LINEORIGIN 16
#define STDINT_EXACT_INT_LIMITS_LINEORIGIN 19
#define STDINT_EXACT_LONG_LIMITS_LINEORIGIN 22
#define STDINT_EXACT_LLONG_LIMITS_LINEORIGIN 25
#define STDINT_INTPTR_LINE 28
#define STDINT_UINTPTR_LINE 29
#define STDINT_INTPTR_LIMITS_LINEORIGIN 30
#define STDINT_INTMAX_LIMITS_LINEORIGIN 36
#define STDINT_PTRDIFF_T_LIMITS_LINEORIGIN 42
#define STDINT_SIZE_T_MAX_LINE 44
#define STDINT_WCHAR_T_LIMITS_LINEORIGIN 45

#define STDINT_CPP_EXACT_LINEORIGIN 49
#define STDINT_CPP_INTPTR_LINE 59
#define STDINT_CPP_UINTPTR_LINE 60

#define STDINT_LEAST_FAST_INJECT_LINE 47
#define STDINT_CPP_LEAST_FAST_INJECT_LINE 61

// inject preprocessor block of preexisting definitions
//! \throw std::bad_alloc
static void
disallow_prior_definitions(zaimoni::autovalarray_ptr<zaimoni::Token<char>* >& TokenList,size_t i,const char* const * identifiers,size_t identifiers_len)
{
	assert(TokenList.size()>i);
	assert(identifiers);
	assert(0<identifiers_len);
	assert(SIZE_MAX/4>=identifiers_len);
/*
#ifdef __bool_true_false_are_defined
#error Undefined Behavior: reserved identifier '__bool_true_false_are_defined' defined as macro
#undef __bool_true_false_are_defined
#endif
*/
	TokenList.insertNSlotsAt(4*identifiers_len,i);
	while(0<identifiers_len)
		{
		assert(*identifiers && **identifiers);
		zaimoni::Token<char>** const tmp = TokenList.c_array();
		tmp[i+3] = new zaimoni::Token<char>("#endif",0,sizeof("#endif")-1,0);

		const size_t identifier_len = strlen(*identifiers);
		zaimoni::autovalarray_ptr_throws<char> tmp2(ZAIMONI_LEN_WITH_NULL(sizeof("#ifdef ")-1+identifier_len));
		strcpy(tmp2,"#ifdef ");
		strcpy(tmp2+sizeof("#ifdef ")-1,*identifiers);
#ifndef ZAIMONI_FORCE_ISO
		tmp[i] = new zaimoni::Token<char>(tmp2,NULL);
#else
		tmp[i] = new zaimoni::Token<char>(tmp2,ZAIMONI_LEN_WITH_NULL(sizeof("#ifdef ")-1+identifier_len),NULL);
#endif

		tmp2.resize(ZAIMONI_LEN_WITH_NULL(sizeof("#undef ")-1+identifier_len));
		strcpy(tmp2,"#undef ");
		strcpy(tmp2+sizeof("#undef ")-1,*identifiers);
#ifndef ZAIMONI_FORCE_ISO
		tmp[i+2] = new zaimoni::Token<char>(tmp2,NULL);
#else
		tmp[i+2] = new zaimoni::Token<char>(tmp2,ZAIMONI_LEN_WITH_NULL(sizeof("#undef ")-1+identifier_len),NULL);
#endif

		tmp2.resize(ZAIMONI_LEN_WITH_NULL(sizeof("#error Undefined Behavior: reserved identifier '")-1+identifier_len+sizeof("' defined as macro")-1));
		strcpy(tmp2,"#error Undefined Behavior: reserved identifier '");
		strcpy(tmp2+sizeof("#error Undefined Behavior: reserved identifier '")-1,*identifiers);
		strcpy(tmp2+sizeof("#error Undefined Behavior: reserved identifier '")-1+identifier_len,"' defined as macro");
#ifndef ZAIMONI_FORCE_ISO
		tmp[i+1] = new zaimoni::Token<char>(tmp2,NULL);
#else
		tmp[i+1] = new zaimoni::Token<char>(tmp2,ZAIMONI_LEN_WITH_NULL(sizeof("#error Undefined Behavior: reserved identifier '")-1+identifier_len+sizeof("' defined as macro")-1),NULL);
#endif

		i += 4;
		--identifiers_len;
		++identifiers;
		}
}

// inject preprocessor lockdown for a reserved identifier
//! \throw std::bad_alloc
static void
lockdown_reserved_identifiers(zaimoni::autovalarray_ptr<zaimoni::Token<char>* >& TokenList,const size_t i,const char* const * identifiers,size_t identifiers_len)
{
	assert(TokenList.size()>i);
	assert(identifiers);
	assert(0<identifiers_len);
	size_t target_len = sizeof("#pragma ZCC lock")-1;
	size_t j = 0;
	do	{
		assert(*identifiers && **identifiers);
		target_len += strlen(identifiers[j])+1;
		}
	while(identifiers_len> ++j);
	zaimoni::autovalarray_ptr_throws<char> tmp(ZAIMONI_LEN_WITH_NULL(target_len));
	{
	char* tmp2 = tmp.c_array();
	strcpy(tmp2,"#pragma ZCC lock");
	tmp2 += sizeof("#pragma ZCC lock")-1;
	while(0<identifiers_len)
		{
		*tmp2++ = ' ';
		strcpy(tmp2,*identifiers);
		tmp2 += strlen(*identifiers);
		++identifiers;
		--identifiers_len;
		};
	}
#ifndef ZAIMONI_FORCE_ISO
	zaimoni::Token<char>* relay = new zaimoni::Token<char>(tmp,NULL);
#else
	zaimoni::Token<char>* relay = new zaimoni::Token<char>(tmp,ZAIMONI_LEN_WITH_NULL(target_len),NULL);
#endif
	if (!TokenList.InsertSlotAt(i,relay))
		{
		delete relay;
		throw std::bad_alloc();
		};
}

static void final_init_tokenlist(zaimoni::Token<char>* const * x, size_t x_len, const char* const header_name)
{
	assert(x);
	assert(0<x_len);
	assert(header_name && *header_name);
	while(0<x_len)
		{
		--x_len;
		assert(x[x_len]);
		x[x_len]->logical_line.first = x_len+1;
		x[x_len]->logical_line.second = 0;
		x[x_len]->original_line = x[x_len]->logical_line;
		x[x_len]->src_filename = header_name;
		};
}

/*! 
 * Improvises the C99 limits.h header from target information.  Can throw std::bad_alloc.
 * 
 * \param TokenList : where to improvise to
 * \param header_name : what header we were requesting.
 */
//! \throw std::bad_alloc
void
CPreprocessor::create_limits_header(zaimoni::autovalarray_ptr<zaimoni::Token<char>* >& TokenList,const char* const header_name) const
{
	assert(header_name && *header_name);
	assert(TokenList.empty());
	// 2 for: leading space, trailing null-termination
	// (VM_MAX_BIT_PLATFORM/3) for: digits (using octal rather than decimal count because that's easy to do at compile-time)
	zaimoni::autovalarray_ptr_throws<char> buf(2+(VM_MAX_BIT_PLATFORM/3));
	buf[0] = ' ';
	zaimoni::autovalarray_ptr<zaimoni::Token<char>* > TmpTokenList(STATIC_SIZE(limits_h_core));
	zaimoni::Token<char>** tmp = TmpTokenList.c_array();
	
	size_t i = STATIC_SIZE(limits_h_core);
	do	{
		--i;
		tmp[i] = new zaimoni::Token<char>(limits_h_core[i].first,0,limits_h_core[i].second,0);
		}
	while(0<i);
	// initialize the limits from target_machine
	// no stdlib, so no real multibyte character support
	// \todo make this react to a compiler option
	tmp[LIMITS_MB_LEN_MAX_LINE]->append(" 1");
	// charbits
	tmp[LIMITS_CHAR_BIT_LINE]->append(z_umaxtoa(target_machine.C_char_bit(),buf+1,10)-1);

	// set up the negative signs
	tmp[LIMITS_CHAR_MIN_LINE]->append((target_machine.char_is_signed_char()) ? " -" : " 0");	// char is unsigned: 0

	// C99 5.2.4.2.1p1: types for UCHAR_MAX, USHRT_MAX are target-dependent
	// unsigned character limits
	tmp[LIMITS_UCHAR_MAX_LINE]->append(z_ucharint_toa(target_machine.unsigned_max<virtual_machine::std_int_char>(),buf+1,10)-1);
	if (1==target_machine.C_sizeof_int()) tmp[LIMITS_UCHAR_MAX_LINE]->append('U');
	if (!target_machine.char_is_signed_char())
		{
		tmp[LIMITS_CHAR_MAX_LINE]->append(buf);
		if (1==target_machine.C_sizeof_int()) tmp[LIMITS_CHAR_MAX_LINE]->append('U');
		}
	// signed character limits
	umaxint s_max(target_machine.signed_max<virtual_machine::std_int_char>());
	tmp[LIMITS_SCHAR_MAX_LINE]->append(z_ucharint_toa(s_max,buf+1,10)-1);
	if (target_machine.char_is_signed_char()) tmp[LIMITS_CHAR_MAX_LINE]->append(buf);
	const bool twos_complement_non_trapping = virtual_machine::twos_complement==target_machine.C_signed_int_representation() && !bool_options[boolopt::int_traps];
	tmp[LIMITS_SCHAR_MIN_LINE]->append(twos_complement_non_trapping ? z_ucharint_toa(s_max += 1,buf+1,10) : buf+1);
	if (target_machine.char_is_signed_char()) tmp[LIMITS_CHAR_MIN_LINE]->append(buf+1);

	// unsigned short limits
	tmp[LIMITS_USHRT_MAX_LINE]->append(z_ucharint_toa(target_machine.unsigned_max<virtual_machine::std_int_short>(),buf+1,10)-1);
	if (target_machine.C_sizeof_short()>=target_machine.C_sizeof_int()) tmp[LIMITS_USHRT_MAX_LINE]->append('U');
	// signed short limits
	s_max = target_machine.signed_max<virtual_machine::std_int_short>();
	tmp[LIMITS_SHRT_MAX_LINE]->append(z_ucharint_toa(s_max,buf+1,10)-1);
	tmp[LIMITS_SHRT_MIN_LINE]->append(twos_complement_non_trapping ? z_ucharint_toa(s_max += 1,buf+1,10) : buf+1);

	// unsigned int limits
	tmp[LIMITS_UINT_MAX_LINE]->append(z_ucharint_toa(target_machine.unsigned_max<virtual_machine::std_int_int>(),buf+1,10)-1);
	tmp[LIMITS_UINT_MAX_LINE]->append('U');	// C99 5.2.4.2.1 p1 : requires unsigned int
	// signed int limits
	s_max = target_machine.signed_max<virtual_machine::std_int_int>();
	tmp[LIMITS_INT_MAX_LINE]->append(z_ucharint_toa(s_max,buf+1,10)-1);
	tmp[LIMITS_INT_MIN_LINE]->append(twos_complement_non_trapping ? z_ucharint_toa(s_max += 1,buf+1,10) : buf+1);

	// unsigned long limits
	tmp[LIMITS_ULONG_MAX_LINE]->append(z_ucharint_toa(target_machine.unsigned_max<virtual_machine::std_int_long>(),buf+1,10)-1,"UL");
	// signed long limits
	s_max = target_machine.signed_max<virtual_machine::std_int_long>();
	tmp[LIMITS_LONG_MAX_LINE]->append(z_ucharint_toa(s_max,buf+1,10)-1);
	tmp[LIMITS_LONG_MAX_LINE]->append('L');
	tmp[LIMITS_LONG_MIN_LINE]->append(twos_complement_non_trapping ? z_ucharint_toa(s_max += 1,buf+1,10) : buf+1);
	tmp[LIMITS_LONG_MIN_LINE]->append('L');

	// unsigned long long limits
	tmp[LIMITS_ULLONG_MAX_LINE]->append(z_ucharint_toa(target_machine.unsigned_max<virtual_machine::std_int_long_long>(),buf+1,10)-1,"ULL");
	// signed long long limits
	s_max = target_machine.signed_max<virtual_machine::std_int_long_long>();
	tmp[LIMITS_LLONG_MAX_LINE]->append(z_ucharint_toa(s_max,buf+1,10)-1,"LL");
	if (twos_complement_non_trapping)
		{
		tmp[LIMITS_LLONG_MIN_LINE]->append("(-1-");
		tmp[LIMITS_LLONG_MIN_LINE]->append(buf+1,"LL)");
		}
	else{
		tmp[LIMITS_LLONG_MIN_LINE]->append('-');
		tmp[LIMITS_LLONG_MIN_LINE]->append(buf+1,"LL");
		}

	// handle POSIX; should be no question of representability for reasonable machines
	tmp[LIMITS_WORD_BIT_LINE]->append(z_umaxtoa(target_machine.C_bit<virtual_machine::std_int_int>(),buf+1,10)-1);
	tmp[LIMITS_LONG_BIT_LINE]->append(z_umaxtoa(target_machine.C_bit<virtual_machine::std_int_long>(),buf+1,10)-1);

	lockdown_reserved_identifiers(TmpTokenList,LIMITS_INJECT_LOCK,limits_h_reserved+18,STATIC_SIZE(limits_h_reserved)-18);
	lockdown_reserved_identifiers(TmpTokenList,LIMITS_INJECT_LOCK,limits_h_reserved+12,6);
	lockdown_reserved_identifiers(TmpTokenList,LIMITS_INJECT_LOCK,limits_h_reserved+6,6);
	lockdown_reserved_identifiers(TmpTokenList,LIMITS_INJECT_LOCK,limits_h_reserved,6);
	lockdown_reserved_identifiers(TmpTokenList,LIMITS_POSIX_INJECT_LOCK,limits_h_POSIX_reserved,STATIC_SIZE(limits_h_POSIX_reserved));
	disallow_prior_definitions(TmpTokenList,LIMITS_POSIX_INJECT_REALITY,limits_h_POSIX_reserved,STATIC_SIZE(limits_h_POSIX_reserved));
	disallow_prior_definitions(TmpTokenList,LIMITS_INJECT_REALITY,limits_h_reserved,STATIC_SIZE(limits_h_reserved));
	zaimoni::swap(TokenList,TmpTokenList);
	
	final_init_tokenlist(TokenList.c_array(),TokenList.size(),header_name);
}

//! \bug balancing feature envy vs minimal interface
static const char* signed_type_from_machine(const virtual_machine::std_int_enum x)
{
	switch(x)
	{
	case virtual_machine::std_int_char:	return "signed char";
	case virtual_machine::std_int_short:	return "short";
	case virtual_machine::std_int_int:	return "int";
	case virtual_machine::std_int_long:	return "long";
	case virtual_machine::std_int_long_long:	return "long long";
	};
	return NULL;
}

//! \bug balancing feature envy vs minimal interface
static const char* unsigned_type_from_machine(const virtual_machine::std_int_enum x)
{
	switch(x)
	{
	case virtual_machine::std_int_char:	return "unsigned char";
	case virtual_machine::std_int_short:	return "unsigned short";
	case virtual_machine::std_int_int:	return "unsigned int";
	case virtual_machine::std_int_long:	return "unsigned long";
	case virtual_machine::std_int_long_long:	return "unsigned long long";
	};
	return NULL;
}

//! \bug balancing feature envy vs minimal interface
static const char* signed_suffix_from_machine(const virtual_machine::std_int_enum x)
{
	switch(x)
	{
	case virtual_machine::std_int_char:	return NULL;
	case virtual_machine::std_int_short:	return NULL;
	case virtual_machine::std_int_int:	return NULL;
	case virtual_machine::std_int_long:	return "L";
	case virtual_machine::std_int_long_long:	return "LL";
	};
	return NULL;
}

//! \bug balancing feature envy vs minimal interface
// GCC compatibility: C99 7.18.4.1p1 (for stdint.h) requires that the INT_C
// and UINT_C constants be of an integral constant type corresponding
// to the type.  While strictly speaking there is no such type below int,
// the integer promotions do elevate types below int to int upon use in 
// any arithmetic operator (it is a machine-specific optimization 
// to not do so).  This has no effect on INT_C, but UINT_C is paradoxical.  
// C99 5.2.4.2.1p1 (for limits.h) is clearer.
static const char* unsigned_suffix_from_machine(const virtual_machine::std_int_enum x,const virtual_machine::CPUInfo& target_machine)
{
	switch(x)
	{
	case virtual_machine::std_int_char:	return (1<target_machine.C_sizeof_int()) ? NULL : "U";
	case virtual_machine::std_int_short:	return (target_machine.C_sizeof_short()<target_machine.C_sizeof_int()) ? NULL : "U";
	case virtual_machine::std_int_int:	return "U";
	case virtual_machine::std_int_long:	return "UL";
	case virtual_machine::std_int_long_long:	return "ULL";
	};
	return NULL;
}

//! \bug balancing feature envy vs minimal interface
static const char* NULL_constant_from_machine(const virtual_machine::std_int_enum x)
{
	switch(x)
	{
	case virtual_machine::std_int_char:	return "'\0'";
	case virtual_machine::std_int_short:	return "0";
	case virtual_machine::std_int_int:	return "0";
	case virtual_machine::std_int_long:	return "0L";
	case virtual_machine::std_int_long_long:	return "0LL";
	};
	return NULL;
}

/*! 
 * Improvises the C99 stddef.h header from target information.  Can throw std::bad_alloc.
 * 
 * \param TokenList : where to improvise to
 * \param header_name : what header we were requesting.
 */
//! \throw std::bad_alloc
void
CPreprocessor::create_stddef_header(zaimoni::autovalarray_ptr<zaimoni::Token<char>* >& TokenList,const char* const header_name) const
{
	assert(header_name && *header_name);
	assert(TokenList.empty());
	zaimoni::autovalarray_ptr<zaimoni::Token<char>* > TmpTokenList(STATIC_SIZE(stddef_h_core));
	zaimoni::Token<char>** tmp = TmpTokenList.c_array();

	size_t i = STATIC_SIZE(stddef_h_core);
	do	{
		--i;
		tmp[i] = new zaimoni::Token<char>(stddef_h_core[i].first,0,stddef_h_core[i].second,0);
		}
	while(0<i);

	// C99 17.7p2 : typedefs; C++ versions in namespace std
	const char* const ptrdiff_str = signed_type_from_machine(target_machine.ptrdiff_t_type());
	assert(ptrdiff_str);
	tmp[STDDEF_PTRDIFF_T_LINE]->append(ptrdiff_str," ptrdiff_t;");
	tmp[STDDEF_CPP_PTRDIFF_T_LINE]->append(ptrdiff_str," ptrdiff_t;");

	const char* const size_t_str = unsigned_type_from_machine(target_machine.size_t_type());
	assert(size_t_str);
	tmp[STDDEF_SIZE_T_LINE]->append(size_t_str," size_t;");
	tmp[STDDEF_CPP_SIZE_T_LINE]->append(size_t_str," size_t;");

	const char* const wchar_t_str = unsigned_type_from_machine(target_machine.UNICODE_wchar_t());
	assert(wchar_t_str);
	tmp[STDDEF_WCHAR_T_LINE]->append(wchar_t_str," wchar_t;");

	// C99 17.7p3 : macros
	// we assume that ptrdiff_t is the correct size (really should have an explicit void* size)
	tmp[STDDEF_NULL_LINE]->append(NULL_constant_from_machine(target_machine.ptrdiff_t_type()));

	BOOST_STATIC_ASSERT(6==STATIC_SIZE(stddef_h_reserved));
	disallow_prior_definitions(TmpTokenList,STDDEF_INJECT_CPP_REALITY,stddef_h_reserved+5,1);
	disallow_prior_definitions(TmpTokenList,STDDEF_INJECT_C_REALITY,stddef_h_reserved,1);
	disallow_prior_definitions(TmpTokenList,STDDEF_INJECT_REALITY,stddef_h_reserved+1,4);
	zaimoni::swap(TokenList,TmpTokenList);

	final_init_tokenlist(TokenList.c_array(),TokenList.size(),header_name);
}

static void new_token_at(zaimoni::autovalarray_ptr<zaimoni::Token<char>* >& dest,size_t i,const char* const src)
{
	assert(src && *src);
	zaimoni::Token<char>* tmp = new zaimoni::Token<char>(src,0,strlen(src),0);
	if (!dest.InsertSlotAt(i,tmp))
		{
		delete tmp;
		throw std::bad_alloc();
		};
}

template<size_t offset,size_t buf_size>
static void memset_strcpy(char* dest,const char* src)
{
	BOOST_STATIC_ASSERT(offset<buf_size);
	assert(dest);
	assert(src);
	assert(buf_size-offset>strlen(src));
	memset(dest += offset,0,buf_size-offset);
	strcpy(dest,src);
}

template<size_t offset>
static void memset_strcpy(char* dest,const char* src,size_t buf_size)
{
	assert(offset<buf_size);
	assert(dest);
	assert(src);
	assert(buf_size-offset>strlen(src));
	memset(dest += offset,0,buf_size-offset);
	strcpy(dest,src);
}

/*! 
 * Improvises the C99 stdint.h header from target information.  Can throw std::bad_alloc.
 * This is not the most memory-efficient implementation possible.
 * We currently do *not* handle padding bits; sorry.
 * 
 * \param TokenList : where to improvise to
 * \param header_name : what header we were requesting.
 */
//! \throw std::bad_alloc
void
CPreprocessor::create_stdint_header(zaimoni::autovalarray_ptr<zaimoni::Token<char>* >& TokenList,const char* const header_name) const
{
	assert(header_name && *header_name);
	assert(TokenList.empty());
	// 2 for: leading space, trailing null-termination
	// (VM_MAX_BIT_PLATFORM/3) for: digits (using octal rather than decimal count because that's easy to do at compile-time)
	zaimoni::autovalarray_ptr_throws<char> buf(2+(VM_MAX_BIT_PLATFORM/3));
	zaimoni::autovalarray_ptr<zaimoni::Token<char>* > TmpTokenList(STATIC_SIZE(stdint_h_core));
	zaimoni::Token<char>** tmp = TmpTokenList.c_array();

//	TokenList.resize(STATIC_SIZE(stdint_h_core));
//	zaimoni::Token<char>** tmp = TokenList.c_array();
	size_t i = STATIC_SIZE(stdint_h_core);
	do	{
		--i;
		tmp[i] = new zaimoni::Token<char>(stdint_h_core[i].first,0,stdint_h_core[i].second,0);
		}
	while(0<i);

	// set up some result strings
	zaimoni::autovalarray_ptr_throws<char> signed_max_metabuf(virtual_machine::std_int_enum_max*(2+(VM_MAX_BIT_PLATFORM/3)+4));
	char* signed_max_buf[virtual_machine::std_int_enum_max] = {signed_max_metabuf, signed_max_metabuf+(2+(VM_MAX_BIT_PLATFORM/3)+2), signed_max_metabuf+2*(2+(VM_MAX_BIT_PLATFORM/3)+2), signed_max_metabuf+3*(2+(VM_MAX_BIT_PLATFORM/3)+2), signed_max_metabuf+4*(2+(VM_MAX_BIT_PLATFORM/3)+2)};
	*signed_max_buf[0] = ' ';
	*signed_max_buf[1] = ' ';
	*signed_max_buf[2] = ' ';
	*signed_max_buf[3] = ' ';
	*signed_max_buf[4] = ' ';
	z_ucharint_toa(target_machine.signed_max<virtual_machine::std_int_char>(),signed_max_buf[virtual_machine::std_int_char-1]+1,10);
	z_ucharint_toa(target_machine.signed_max<virtual_machine::std_int_short>(),signed_max_buf[virtual_machine::std_int_short-1]+1,10);
	z_ucharint_toa(target_machine.signed_max<virtual_machine::std_int_int>(),signed_max_buf[virtual_machine::std_int_int-1]+1,10);
	z_ucharint_toa(target_machine.signed_max<virtual_machine::std_int_long>(),signed_max_buf[virtual_machine::std_int_long-1]+1,10);
	z_ucharint_toa(target_machine.signed_max<virtual_machine::std_int_long_long>(),signed_max_buf[virtual_machine::std_int_long_long-1]+1,10);
	strcat(signed_max_buf[virtual_machine::std_int_long-1],"L");
	strcat(signed_max_buf[virtual_machine::std_int_long_long-1],"LL");

	zaimoni::autovalarray_ptr_throws<char> unsigned_max_metabuf(virtual_machine::std_int_enum_max*(2+(VM_MAX_BIT_PLATFORM/3)+3));
	char* unsigned_max_buf[virtual_machine::std_int_enum_max] = {unsigned_max_metabuf, unsigned_max_metabuf+(2+(VM_MAX_BIT_PLATFORM/3)+2), unsigned_max_metabuf+2*(2+(VM_MAX_BIT_PLATFORM/3)+2), unsigned_max_metabuf+3*(2+(VM_MAX_BIT_PLATFORM/3)+2), unsigned_max_metabuf+4*(2+(VM_MAX_BIT_PLATFORM/3)+2)};
	*unsigned_max_buf[0] = ' ';
	*unsigned_max_buf[1] = ' ';
	*unsigned_max_buf[2] = ' ';
	*unsigned_max_buf[3] = ' ';
	*unsigned_max_buf[4] = ' ';
	z_ucharint_toa(target_machine.unsigned_max<virtual_machine::std_int_char>(),unsigned_max_buf[virtual_machine::std_int_char-1]+1,10);
	z_ucharint_toa(target_machine.unsigned_max<virtual_machine::std_int_short>(),unsigned_max_buf[virtual_machine::std_int_short-1]+1,10);
	z_ucharint_toa(target_machine.unsigned_max<virtual_machine::std_int_int>(),unsigned_max_buf[virtual_machine::std_int_int-1]+1,10);
	z_ucharint_toa(target_machine.unsigned_max<virtual_machine::std_int_long>(),unsigned_max_buf[virtual_machine::std_int_long-1]+1,10);
	z_ucharint_toa(target_machine.unsigned_max<virtual_machine::std_int_long_long>(),unsigned_max_buf[virtual_machine::std_int_long_long-1]+1,10);
	strcat(unsigned_max_buf[virtual_machine::std_int_char-1],"U");
	strcat(unsigned_max_buf[virtual_machine::std_int_short-1],"U");
	strcat(unsigned_max_buf[virtual_machine::std_int_int-1],"U");
	strcat(unsigned_max_buf[virtual_machine::std_int_long-1],"UL");
	strcat(unsigned_max_buf[virtual_machine::std_int_long_long-1],"ULL");

	const bool target_is_twos_complement = virtual_machine::twos_complement==target_machine.C_signed_int_representation();
	zaimoni::autovalarray_ptr_throws<char> signed_min_metabuf(virtual_machine::std_int_enum_max*(2+(VM_MAX_BIT_PLATFORM/3)+4));
	char* signed_min_buf[virtual_machine::std_int_enum_max] = {signed_min_metabuf, signed_min_metabuf+(2+(VM_MAX_BIT_PLATFORM/3)+2), signed_min_metabuf+2*(2+(VM_MAX_BIT_PLATFORM/3)+2), signed_min_metabuf+3*(2+(VM_MAX_BIT_PLATFORM/3)+2), signed_min_metabuf+4*(2+(VM_MAX_BIT_PLATFORM/3)+2)};
	umaxint tmp_VM;
	if (target_is_twos_complement && !bool_options[boolopt::int_traps])
		{
		*signed_min_buf[0] = '-';
		*signed_min_buf[1] = '-';
		*signed_min_buf[2] = '-';
		*signed_min_buf[3] = '-';
		*signed_min_buf[4] = '-';
		tmp_VM = target_machine.signed_max<virtual_machine::std_int_char>();
		tmp_VM += 1;
		z_ucharint_toa(tmp_VM,signed_min_buf[virtual_machine::std_int_char-1]+1,10);
		tmp_VM = target_machine.signed_max<virtual_machine::std_int_short>();
		tmp_VM += 1;
		z_ucharint_toa(tmp_VM,signed_min_buf[virtual_machine::std_int_short-1]+1,10);
		tmp_VM = target_machine.signed_max<virtual_machine::std_int_int>();
		tmp_VM += 1;
		z_ucharint_toa(tmp_VM,signed_min_buf[virtual_machine::std_int_int-1]+1,10);
		tmp_VM = target_machine.signed_max<virtual_machine::std_int_long>();
		tmp_VM += 1;
		z_ucharint_toa(tmp_VM,signed_min_buf[virtual_machine::std_int_long-1]+1,10);
		tmp_VM = target_machine.signed_max<virtual_machine::std_int_long_long>();
		strcpy(signed_min_buf[virtual_machine::std_int_long_long-1],"(-1-");
		z_ucharint_toa(tmp_VM,signed_min_buf[virtual_machine::std_int_long_long-1]+4,10);
		strcat(signed_min_buf[virtual_machine::std_int_long-1],"L");
		strcat(signed_min_buf[virtual_machine::std_int_long_long-1],"LL)");
		}
	else{
		assert(signed_min_metabuf.size()==signed_max_metabuf.size());
		memmove(signed_min_metabuf,signed_max_metabuf,signed_max_metabuf.size());
		*signed_min_buf[0] = '-';
		*signed_min_buf[1] = '-';
		*signed_min_buf[2] = '-';
		*signed_min_buf[3] = '-';
		*signed_min_buf[4] = '-';
		}

	// we assume ptrdiff_t is closely related to intptr_t and uintptr_t (doesn't work too well on 16-bit DOS)
	const virtual_machine::std_int_enum ptrtype = target_machine.ptrdiff_t_type();
	const char* const ptr_signed_type = signed_type_from_machine(ptrtype);
	tmp[STDINT_INTPTR_LINE]->append(ptr_signed_type," intptr_t;");
	tmp[STDINT_CPP_INTPTR_LINE]->append(ptr_signed_type," intptr_t;");
	const char* const ptr_unsigned_type = unsigned_type_from_machine(ptrtype);
	tmp[STDINT_UINTPTR_LINE]->append(ptr_unsigned_type," uintptr_t;");
	tmp[STDINT_CPP_UINTPTR_LINE]->append(ptr_unsigned_type," uintptr_t;");

	// intptr_t limits
	tmp[STDINT_INTPTR_LIMITS_LINEORIGIN+STDINT_SMAX_OFFSET]->append(signed_max_buf[ptrtype-1]);
	tmp[STDINT_PTRDIFF_T_LIMITS_LINEORIGIN+STDINT_SMAX_OFFSET]->append(signed_max_buf[ptrtype-1]);
	tmp[STDINT_INTPTR_LIMITS_LINEORIGIN+STDINT_SMIN_OFFSET]->append(signed_min_buf[ptrtype-1]);
	tmp[STDINT_PTRDIFF_T_LIMITS_LINEORIGIN+STDINT_SMIN_OFFSET]->append(signed_min_buf[ptrtype-1]);

	// assume size_t is same as uintptr_t
	tmp[STDINT_SIZE_T_MAX_LINE]->append(unsigned_max_buf[ptrtype-1]);

	// uintptr_t limits
	tmp[STDINT_INTPTR_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET]->append(unsigned_max_buf[ptrtype-1]);

	// intmax_t limits
	tmp[STDINT_INTMAX_LIMITS_LINEORIGIN+STDINT_SMAX_OFFSET]->append(signed_max_buf[virtual_machine::std_int_long_long-1]);
	tmp[STDINT_INTMAX_LIMITS_LINEORIGIN+STDINT_SMIN_OFFSET]->append(signed_min_buf[virtual_machine::std_int_long_long-1]);

	// uintmax_t limits
	tmp[STDINT_INTMAX_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET]->append(unsigned_max_buf[virtual_machine::std_int_long_long-1]);

	// wchar_t limits
	tmp[STDINT_WCHAR_T_LIMITS_LINEORIGIN+STDINT_SMAX_OFFSET]->append(unsigned_max_buf[target_machine.UNICODE_wchar_t()-1]);

	// two's complement controls whether the exact-width int types even exist
	const unsigned short type_bits[virtual_machine::std_int_enum_max] = {target_machine.C_bit(virtual_machine::std_int_char),target_machine.C_bit(virtual_machine::std_int_short),target_machine.C_bit(virtual_machine::std_int_int),target_machine.C_bit(virtual_machine::std_int_long),target_machine.C_bit(virtual_machine::std_int_long_long)};
	const bool suppress[virtual_machine::std_int_enum_max-1] = {type_bits[virtual_machine::std_int_char-1]==type_bits[virtual_machine::std_int_short-1],type_bits[virtual_machine::std_int_short-1]==type_bits[virtual_machine::std_int_int-1],type_bits[virtual_machine::std_int_int-1]==type_bits[virtual_machine::std_int_long-1],type_bits[virtual_machine::std_int_long-1]==type_bits[virtual_machine::std_int_long_long-1]};

	// uint___t and UINT___MAX will exist no matter what; almost everything else has suppresssion conditions
	// int
	tmp[STDINT_EXACT_LINEORIGIN+STDINT_EXACT_UINT_OFFSET]->append(z_umaxtoa(type_bits[virtual_machine::std_int_int-1],buf+1,10),"_t;");
	tmp[STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_UINT_OFFSET]->append(buf+1,"_t;");
	tmp[STDINT_EXACT_INT_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET]->append(buf+1,"_MAX");
	if (target_is_twos_complement)
		{
		tmp[STDINT_EXACT_LINEORIGIN+STDINT_EXACT_INT_OFFSET]->append(buf+1,"_t;");
		tmp[STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_INT_OFFSET]->append(buf+1,"_t;");
		tmp[STDINT_EXACT_INT_LIMITS_LINEORIGIN+STDINT_SMIN_OFFSET]->append(buf+1,"_MIN ");
		tmp[STDINT_EXACT_INT_LIMITS_LINEORIGIN+STDINT_SMAX_OFFSET]->append(buf+1,"_MAX");
		};
	tmp[STDINT_EXACT_INT_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET]->append(unsigned_max_buf[virtual_machine::std_int_int-1]);
	if (target_is_twos_complement)
		{
		tmp[STDINT_EXACT_INT_LIMITS_LINEORIGIN+STDINT_SMAX_OFFSET]->append(signed_max_buf[virtual_machine::std_int_int-1]);
		tmp[STDINT_EXACT_INT_LIMITS_LINEORIGIN+STDINT_SMIN_OFFSET]->append(signed_min_buf[virtual_machine::std_int_int-1]);
		};

	// char-based types
	if (!suppress[virtual_machine::std_int_char-1])
		{
		tmp[STDINT_EXACT_LINEORIGIN+STDINT_EXACT_UCHAR_OFFSET]->append(z_umaxtoa(type_bits[virtual_machine::std_int_char-1],buf+1,10),"_t;");
		tmp[STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_UCHAR_OFFSET]->append(buf+1,"_t;");
		tmp[STDINT_EXACT_CHAR_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET]->append(buf+1,"_MAX");
		if (target_is_twos_complement)
			{
			tmp[STDINT_EXACT_LINEORIGIN+STDINT_EXACT_SCHAR_OFFSET]->append(buf+1,"_t;");
			tmp[STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_SCHAR_OFFSET]->append(buf+1,"_t;");
			tmp[STDINT_EXACT_CHAR_LIMITS_LINEORIGIN+STDINT_SMIN_OFFSET]->append(buf+1,"_MIN ");
			tmp[STDINT_EXACT_CHAR_LIMITS_LINEORIGIN+STDINT_SMAX_OFFSET]->append(buf+1,"_MAX");
			};
		tmp[STDINT_EXACT_CHAR_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET]->append(unsigned_max_buf[virtual_machine::std_int_char-1]);
		if (target_is_twos_complement)
			{
			tmp[STDINT_EXACT_CHAR_LIMITS_LINEORIGIN+STDINT_SMAX_OFFSET]->append(signed_max_buf[virtual_machine::std_int_char-1]);
			tmp[STDINT_EXACT_CHAR_LIMITS_LINEORIGIN+STDINT_SMIN_OFFSET]->append(signed_min_buf[virtual_machine::std_int_char-1]);
			};
		}

	// short-based types
	if (!suppress[virtual_machine::std_int_short-1])
		{
		tmp[STDINT_EXACT_LINEORIGIN+STDINT_EXACT_USHRT_OFFSET]->append(z_umaxtoa(type_bits[virtual_machine::std_int_short-1],buf+1,10),"_t;");
		tmp[STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_USHRT_OFFSET]->append(buf+1,"_t;");
		tmp[STDINT_EXACT_SHRT_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET]->append(buf+1,"_MAX");
		if (target_is_twos_complement)
			{
			tmp[STDINT_EXACT_LINEORIGIN+STDINT_EXACT_SHRT_OFFSET]->append(buf+1,"_t;");
			tmp[STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_SHRT_OFFSET]->append(buf+1,"_t;");
			tmp[STDINT_EXACT_SHRT_LIMITS_LINEORIGIN+STDINT_SMIN_OFFSET]->append(buf+1,"_MIN ");
			tmp[STDINT_EXACT_SHRT_LIMITS_LINEORIGIN+STDINT_SMAX_OFFSET]->append(buf+1,"_MAX");
			};
		tmp[STDINT_EXACT_SHRT_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET]->append(unsigned_max_buf[virtual_machine::std_int_short-1]);
		if (target_is_twos_complement)
			{
			tmp[STDINT_EXACT_SHRT_LIMITS_LINEORIGIN+STDINT_SMAX_OFFSET]->append(signed_max_buf[virtual_machine::std_int_short-1]);
			tmp[STDINT_EXACT_SHRT_LIMITS_LINEORIGIN+STDINT_SMIN_OFFSET]->append(signed_min_buf[virtual_machine::std_int_short-1]);
			};
		}

	// long-based types
	if (!suppress[virtual_machine::std_int_long-2])
		{
		tmp[STDINT_EXACT_LINEORIGIN+STDINT_EXACT_ULONG_OFFSET]->append(z_umaxtoa(type_bits[virtual_machine::std_int_long-1],buf+1,10),"_t;");
		tmp[STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_ULONG_OFFSET]->append(buf+1,"_t;");
		tmp[STDINT_EXACT_LONG_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET]->append(buf+1,"_MAX");
		if (target_is_twos_complement)
			{
			tmp[STDINT_EXACT_LINEORIGIN+STDINT_EXACT_LONG_OFFSET]->append(buf+1,"_t;");
			tmp[STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_LONG_OFFSET]->append(buf+1,"_t;");
			tmp[STDINT_EXACT_LONG_LIMITS_LINEORIGIN+STDINT_SMIN_OFFSET]->append(buf+1,"_MIN ");
			tmp[STDINT_EXACT_LONG_LIMITS_LINEORIGIN+STDINT_SMAX_OFFSET]->append(buf+1,"_MAX");
			};
		tmp[STDINT_EXACT_LONG_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET]->append(unsigned_max_buf[virtual_machine::std_int_long-1]);
		if (target_is_twos_complement)
			{
			tmp[STDINT_EXACT_LONG_LIMITS_LINEORIGIN+STDINT_SMAX_OFFSET]->append(signed_max_buf[virtual_machine::std_int_long-1]);
			tmp[STDINT_EXACT_LONG_LIMITS_LINEORIGIN+STDINT_SMIN_OFFSET]->append(signed_min_buf[virtual_machine::std_int_long-1]);
			};
		}

	// long-long-based types
	if (!suppress[virtual_machine::std_int_long_long-2])
		{
		tmp[STDINT_EXACT_LINEORIGIN+STDINT_EXACT_ULLONG_OFFSET]->append(z_umaxtoa(type_bits[virtual_machine::std_int_long_long-1],buf+1,10),"_t;");
		tmp[STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_ULLONG_OFFSET]->append(buf+1,"_t;");
		tmp[STDINT_EXACT_LLONG_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET]->append(buf+1,"_MAX");
		if (target_is_twos_complement)
			{
			tmp[STDINT_EXACT_LINEORIGIN+STDINT_EXACT_LLONG_OFFSET]->append(buf+1,"_t;");
			tmp[STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_LLONG_OFFSET]->append(buf+1,"_t;");
			tmp[STDINT_EXACT_LLONG_LIMITS_LINEORIGIN+STDINT_SMIN_OFFSET]->append(buf+1,"_MIN ");
			tmp[STDINT_EXACT_LLONG_LIMITS_LINEORIGIN+STDINT_SMAX_OFFSET]->append(buf+1,"_MAX");
			};
		tmp[STDINT_EXACT_LLONG_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET]->append(unsigned_max_buf[virtual_machine::std_int_long_long-1]);
		if (target_is_twos_complement)
			{
			tmp[STDINT_EXACT_LLONG_LIMITS_LINEORIGIN+STDINT_SMAX_OFFSET]->append(signed_max_buf[virtual_machine::std_int_long_long-1]);
			tmp[STDINT_EXACT_LLONG_LIMITS_LINEORIGIN+STDINT_SMIN_OFFSET]->append(signed_min_buf[virtual_machine::std_int_long_long-1]);
			};
		}

	// cleanup
	size_t inject_C_index = STDINT_LEAST_FAST_INJECT_LINE;
	size_t inject_CPP_index = STDINT_CPP_LEAST_FAST_INJECT_LINE;
	// C++ typedef cleanup
	{
	size_t stack[9];
	size_t stack_size = 0;
	if (!target_is_twos_complement || suppress[virtual_machine::std_int_long_long-2])
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_LLONG_OFFSET;
		--inject_CPP_index;
		}
	if (!target_is_twos_complement || suppress[virtual_machine::std_int_long-2])
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_LONG_OFFSET;
		--inject_CPP_index;
		}
	if (!target_is_twos_complement)
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_INT_OFFSET;
		--inject_CPP_index;
		}
	if (!target_is_twos_complement || suppress[virtual_machine::std_int_short-1])
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_SHRT_OFFSET;
		--inject_CPP_index;
		}
	if (!target_is_twos_complement || suppress[virtual_machine::std_int_char-1])
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_SCHAR_OFFSET;
		--inject_CPP_index;
		}
	if (suppress[virtual_machine::std_int_long_long-2])
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_ULLONG_OFFSET;
		--inject_CPP_index;
		}
	if (suppress[virtual_machine::std_int_long-2])
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_ULONG_OFFSET;
		--inject_CPP_index;
		}
	if (suppress[virtual_machine::std_int_short-1])
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_USHRT_OFFSET;
		--inject_CPP_index;
		}
	if (suppress[virtual_machine::std_int_char-1])
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_CPP_EXACT_LINEORIGIN+STDINT_EXACT_UCHAR_OFFSET;
		--inject_CPP_index;
		}	
	if (0<stack_size) TmpTokenList.DeleteNSlots(stack,stack_size);
	}

	// limits macros cleanup
	char lock_buf[sizeof("#pragma ZCC lock INT_LEAST_MIN INT_LEAST_MAX UINT_LEAST_MAX INT_FAST_MIN INT_FAST_MAX UINT_FAST_MAX INT_C UINT_C")+8*2] = "#pragma ZCC lock ";
	if (suppress[virtual_machine::std_int_long_long-2])
		{
		TmpTokenList.DeleteNSlotsAt(3,STDINT_EXACT_LLONG_LIMITS_LINEORIGIN);
		inject_CPP_index -= 3;
		inject_C_index -= 3;
		}
	else{
		memset_strcpy<sizeof("#pragma ZCC lock ")-1,sizeof(lock_buf)>(lock_buf,"UINT");
		strcat(lock_buf,z_umaxtoa(type_bits[virtual_machine::std_int_long_long-1],buf+1,10));
		if (target_is_twos_complement)
			{
			strcat(lock_buf,"_MAX INT");
			strcat(lock_buf,buf+1);
			strcat(lock_buf,"_MIN INT");
			strcat(lock_buf,buf+1);
			};
		strcat(lock_buf,"_MAX");
		new_token_at(TmpTokenList,STDINT_EXACT_LLONG_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET+1,lock_buf);
		++inject_CPP_index;
		++inject_C_index;
		if (!target_is_twos_complement)
			{
			TmpTokenList.DeleteNSlotsAt(2,STDINT_EXACT_LLONG_LIMITS_LINEORIGIN);
			inject_CPP_index -= 2;
			inject_C_index -= 2;
			};
		};
	if (suppress[virtual_machine::std_int_long-2])
		{
		TmpTokenList.DeleteNSlotsAt(3,STDINT_EXACT_LONG_LIMITS_LINEORIGIN);
		inject_CPP_index -= 3;
		inject_C_index -= 3;
		}
	else{
		memset_strcpy<sizeof("#pragma ZCC lock ")-1,sizeof(lock_buf)>(lock_buf,"UINT");
		strcat(lock_buf,z_umaxtoa(type_bits[virtual_machine::std_int_long-1],buf+1,10));
		if (target_is_twos_complement)
			{
			strcat(lock_buf,"_MAX INT");
			strcat(lock_buf,buf+1);
			strcat(lock_buf,"_MIN INT");
			strcat(lock_buf,buf+1);
			};
		strcat(lock_buf,"_MAX");
		new_token_at(TmpTokenList,STDINT_EXACT_LONG_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET+1,lock_buf);
		++inject_CPP_index;
		++inject_C_index;
		if (!target_is_twos_complement)
			{
			TmpTokenList.DeleteNSlotsAt(2,STDINT_EXACT_LONG_LIMITS_LINEORIGIN);
			inject_CPP_index -= 2;
			inject_C_index -= 2;
			};
		};

	memset_strcpy<sizeof("#pragma ZCC lock ")-1,sizeof(lock_buf)>(lock_buf,"UINT");
	strcat(lock_buf,z_umaxtoa(type_bits[virtual_machine::std_int_int-1],buf+1,10));
	if (target_is_twos_complement)
		{
		strcat(lock_buf,"_MAX INT");
		strcat(lock_buf,buf+1);
		strcat(lock_buf,"_MIN INT");
		strcat(lock_buf,buf+1);
		};
	strcat(lock_buf,"_MAX");
	new_token_at(TmpTokenList,STDINT_EXACT_INT_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET+1,lock_buf);
	++inject_CPP_index;
	++inject_C_index;
	if (!target_is_twos_complement)
		{
		TmpTokenList.DeleteNSlotsAt(2,STDINT_EXACT_INT_LIMITS_LINEORIGIN);
		inject_CPP_index -= 2;
		inject_C_index -= 2;
		};
	if (suppress[virtual_machine::std_int_short-1])
		{
		TmpTokenList.DeleteNSlotsAt(3,STDINT_EXACT_SHRT_LIMITS_LINEORIGIN);
		inject_CPP_index -= 3;
		inject_C_index -= 3;
		}
	else{
		memset_strcpy<sizeof("#pragma ZCC lock ")-1,sizeof(lock_buf)>(lock_buf,"UINT");
		strcat(lock_buf,z_umaxtoa(type_bits[virtual_machine::std_int_short-1],buf+1,10));
		if (target_is_twos_complement)
			{
			strcat(lock_buf,"_MAX INT");
			strcat(lock_buf,buf+1);
			strcat(lock_buf,"_MIN INT");
			strcat(lock_buf,buf+1);
			};
		strcat(lock_buf,"_MAX");
		new_token_at(TmpTokenList,STDINT_EXACT_SHRT_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET+1,lock_buf);
		++inject_CPP_index;
		++inject_C_index;
		if (!target_is_twos_complement)
			{
			TmpTokenList.DeleteNSlotsAt(2,STDINT_EXACT_SHRT_LIMITS_LINEORIGIN);
			inject_CPP_index -= 2;
			inject_C_index -= 2;
			}
		}
	if (suppress[virtual_machine::std_int_char-1])
		{
		TmpTokenList.DeleteNSlotsAt(3,STDINT_EXACT_CHAR_LIMITS_LINEORIGIN);
		inject_CPP_index -= 3;
		inject_C_index -= 3;
		}
	else{
		memset_strcpy<sizeof("#pragma ZCC lock ")-1,sizeof(lock_buf)>(lock_buf,"UINT");
		strcat(lock_buf,z_umaxtoa(type_bits[virtual_machine::std_int_char-1],buf+1,10));
		if (target_is_twos_complement)
			{
			strcat(lock_buf,"_MAX INT");
			strcat(lock_buf,buf+1);
			strcat(lock_buf,"_MIN INT");
			strcat(lock_buf,buf+1);
			};
		strcat(lock_buf,"_MAX");
		new_token_at(TmpTokenList,STDINT_EXACT_CHAR_LIMITS_LINEORIGIN+STDINT_UMAX_OFFSET+1,lock_buf);
		++inject_CPP_index;
		++inject_C_index;
		if (!target_is_twos_complement)
			{
			TmpTokenList.DeleteNSlotsAt(2,STDINT_EXACT_CHAR_LIMITS_LINEORIGIN);
			inject_CPP_index -= 2;
			inject_C_index -= 2;
			}
		}

	// C typedef cleanup
	{
	size_t stack[9];
	size_t stack_size = 0;

	if (!target_is_twos_complement || suppress[virtual_machine::std_int_long_long-2])
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_EXACT_LINEORIGIN+STDINT_EXACT_LLONG_OFFSET;
		--inject_CPP_index;
		--inject_C_index;
		}
	if (!target_is_twos_complement || suppress[virtual_machine::std_int_long-2])
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_EXACT_LINEORIGIN+STDINT_EXACT_LONG_OFFSET;
		--inject_CPP_index;
		--inject_C_index;
		}
	if (!target_is_twos_complement)
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_EXACT_LINEORIGIN+STDINT_EXACT_INT_OFFSET;
		--inject_CPP_index;
		--inject_C_index;
		}
	if (!target_is_twos_complement || suppress[virtual_machine::std_int_short-1])
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_EXACT_LINEORIGIN+STDINT_EXACT_SHRT_OFFSET;
		--inject_CPP_index;
		--inject_C_index;
		}
	if (!target_is_twos_complement || suppress[virtual_machine::std_int_char-1])
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_EXACT_LINEORIGIN+STDINT_EXACT_SCHAR_OFFSET;
		--inject_CPP_index;
		--inject_C_index;
		}
	if (suppress[virtual_machine::std_int_long_long-2])
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_EXACT_LINEORIGIN+STDINT_EXACT_ULLONG_OFFSET;
		--inject_CPP_index;
		--inject_C_index;
		}
	if (suppress[virtual_machine::std_int_long-2])
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_EXACT_LINEORIGIN+STDINT_EXACT_ULONG_OFFSET;
		--inject_CPP_index;
		--inject_C_index;
		}
	if (suppress[virtual_machine::std_int_short-1])
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_EXACT_LINEORIGIN+STDINT_EXACT_USHRT_OFFSET;
		--inject_CPP_index;
		--inject_C_index;
		}
	if (suppress[virtual_machine::std_int_char-1])
		{
		assert(STATIC_SIZE(stack)>stack_size);
		stack[stack_size++] = STDINT_EXACT_LINEORIGIN+STDINT_EXACT_UCHAR_OFFSET;
		--inject_CPP_index;
		--inject_C_index;
		}
	if (0<stack_size) TmpTokenList.DeleteNSlots(stack,stack_size);	
	}

	// prepare to inject least/fast types and their adapter macros
	// span...start with *8_t and go up to long long bits
	// * C99 requires *8_t types
	// * we don't want to provide types that don't exist
	// * absolute minimum CHAR_BIT is 7, otherwise basic source isn't representable.
	// C++ typedefs in namespace std
	char typedef_buf[sizeof("typedef unsigned long long uint_least_t;")+2] = "typedef ";	// should be dependent on base 10 logarithm of VM_MAX_BIT_PLATFORM: fix auto_int.h
	const unsigned short bitspan_types = type_bits[virtual_machine::std_int_long_long-1]-7;
	assert(USHRT_MAX/13>=bitspan_types);
	i = 4*bitspan_types;
	TmpTokenList.insertNSlotsAt(i,inject_CPP_index);
	tmp = TmpTokenList.c_array()+inject_CPP_index;
	do	{
		const int target_bits = --i/4+8;
		assert(target_bits<=target_machine.C_bit(virtual_machine::std_int_long_long));
		const virtual_machine::std_int_enum fast_type 	= target_bits<=type_bits[virtual_machine::std_int_int-1] ? virtual_machine::std_int_int
														: target_bits<=type_bits[virtual_machine::std_int_long-1] ? virtual_machine::std_int_long
														: virtual_machine::std_int_long_long;
		const virtual_machine::std_int_enum least_type 	= (!suppress[virtual_machine::std_int_char-1] && target_bits<=type_bits[virtual_machine::std_int_char-1]) ? virtual_machine::std_int_char
														: (!suppress[virtual_machine::std_int_short-1] && target_bits<=type_bits[virtual_machine::std_int_short-1]) ? virtual_machine::std_int_short
														: target_bits<=type_bits[virtual_machine::std_int_int-1] ? virtual_machine::std_int_int
														: target_bits<=type_bits[virtual_machine::std_int_long-1] ? virtual_machine::std_int_long
														: virtual_machine::std_int_long_long;
		// uint_least
		memset_strcpy<sizeof("typedef ")-1,sizeof(typedef_buf)>(typedef_buf,unsigned_type_from_machine(least_type));
		strcat(typedef_buf," uint_least");
		strcat(typedef_buf,z_umaxtoa(target_bits,buf+1,10));
		strcat(typedef_buf,"_t;");
		tmp[i--] = new zaimoni::Token<char>(typedef_buf,0,strlen(typedef_buf),0);
		// int_least
		memset_strcpy<sizeof("typedef ")-1,sizeof(typedef_buf)>(typedef_buf,signed_type_from_machine(least_type));
		strcat(typedef_buf," int_least");
		strcat(typedef_buf,buf+1);
		strcat(typedef_buf,"_t;");
		tmp[i--] = new zaimoni::Token<char>(typedef_buf,0,strlen(typedef_buf),0);
		// uint_fast
		memset_strcpy<sizeof("typedef ")-1,sizeof(typedef_buf)>(typedef_buf,unsigned_type_from_machine(fast_type));
		strcat(typedef_buf," uint_fast");
		strcat(typedef_buf,buf+1);
		strcat(typedef_buf,"_t;");
		tmp[i--] = new zaimoni::Token<char>(typedef_buf,0,strlen(typedef_buf),0);
		// int_fast
		memset_strcpy<sizeof("typedef ")-1,sizeof(typedef_buf)>(typedef_buf,signed_type_from_machine(fast_type));
		strcat(typedef_buf," int_fast");
		strcat(typedef_buf,buf+1);
		strcat(typedef_buf,"_t;");
		tmp[i] = new zaimoni::Token<char>(typedef_buf,0,strlen(typedef_buf),0);
		}
	while(0<i);

	zaimoni::autovalarray_ptr_throws<char> define_buf(sizeof("#define UINT_LEAST_MAX")+2+VM_MAX_BIT_PLATFORM/3+5);
	strcpy(define_buf,"#define ");
	i = 13*bitspan_types;
	TmpTokenList.insertNSlotsAt(i,inject_C_index);
	tmp = TmpTokenList.c_array()+inject_C_index;
	do	{
		const int target_bits = --i/13+8;
		assert(target_bits<=target_machine.C_bit(virtual_machine::std_int_long_long));
		const virtual_machine::std_int_enum fast_type 	= target_bits<=type_bits[virtual_machine::std_int_int-1] ? virtual_machine::std_int_int
														: target_bits<=type_bits[virtual_machine::std_int_long-1] ? virtual_machine::std_int_long
														: virtual_machine::std_int_long_long;
		const virtual_machine::std_int_enum least_type 	= (!suppress[virtual_machine::std_int_char-1] && target_bits<=type_bits[virtual_machine::std_int_char-1]) ? virtual_machine::std_int_char
														: (!suppress[virtual_machine::std_int_short-1] && target_bits<=type_bits[virtual_machine::std_int_short-1]) ? virtual_machine::std_int_short
														: target_bits<=type_bits[virtual_machine::std_int_int-1] ? virtual_machine::std_int_int
														: target_bits<=type_bits[virtual_machine::std_int_long-1] ? virtual_machine::std_int_long
														: virtual_machine::std_int_long_long;
		// #pragma ZCC lock INT_LEAST_MIN INT_LEAST_MAX UINT_LEAST_MAX INT_FAST_MIN INT_FAST_MAX UINT_FAST_MAX INT_C UINT_C
		memset_strcpy<sizeof("#pragma ZCC lock ")-1,sizeof(lock_buf)>(lock_buf,"INT_LEAST");
		strcat(lock_buf,z_umaxtoa(target_bits,buf+1,10));
		strcat(lock_buf,"_MIN INT_LEAST");
		strcat(lock_buf,buf+1);
		strcat(lock_buf,"_MAX UINT_LEAST");
		strcat(lock_buf,buf+1);
		strcat(lock_buf,"_MAX INT_FAST");
		strcat(lock_buf,buf+1);
		strcat(lock_buf,"_MIN INT_FAST");
		strcat(lock_buf,buf+1);
		strcat(lock_buf,"_MAX UINT_FAST");
		strcat(lock_buf,buf+1);
		strcat(lock_buf,"_MAX INT");
		strcat(lock_buf,buf+1);
		strcat(lock_buf,"_C UINT");
		strcat(lock_buf,buf+1);
		strcat(lock_buf,"_C");
		tmp[i--] = new zaimoni::Token<char>(lock_buf,0,strlen(lock_buf),0);
		// UINT_C
		memset_strcpy<sizeof("#define ")-1>(define_buf,"UINT",define_buf.size());
		strcat(define_buf,buf+1);
		strcat(define_buf,"_C(A) A");
		const char* int_suffix = unsigned_suffix_from_machine(least_type,target_machine);
		if (NULL!=int_suffix)
			{
			strcat(define_buf,"##");
			strcat(define_buf,int_suffix);
			};
		tmp[i--] = new zaimoni::Token<char>(define_buf,0,strlen(define_buf),0);
		// INT_C
		memset_strcpy<sizeof("#define ")-1>(define_buf,"INT",define_buf.size());
		strcat(define_buf,buf+1);
		strcat(define_buf,"_C(A) A");
		int_suffix = signed_suffix_from_machine(least_type);
		if (NULL!=int_suffix)
			{
			strcat(define_buf,"##");
			strcat(define_buf,int_suffix);
			};
		tmp[i--] = new zaimoni::Token<char>(define_buf,0,strlen(define_buf),0);
		// UINT_FAST_MAX
		memset_strcpy<sizeof("#define ")-1>(define_buf,"UINT_FAST",define_buf.size());
		strcat(define_buf,buf+1);
		strcat(define_buf,"_MAX");
		strcat(define_buf,unsigned_max_buf[fast_type-1]);
		tmp[i--] = new zaimoni::Token<char>(define_buf,0,strlen(define_buf),0);
		// INT_FAST_MAX
		memset_strcpy<sizeof("#define ")-1>(define_buf,"INT_FAST",define_buf.size());
		strcat(define_buf,buf+1);
		strcat(define_buf,"_MAX");
		strcat(define_buf,signed_max_buf[fast_type-1]);
		tmp[i--] = new zaimoni::Token<char>(define_buf,0,strlen(define_buf),0);
		// INT_FAST_MIN
		memset_strcpy<sizeof("#define ")-1>(define_buf,"INT_FAST",define_buf.size());
		strcat(define_buf,buf+1);
		strcat(define_buf,"_MIN ");
		strcat(define_buf,signed_min_buf[fast_type-1]);
		tmp[i--] = new zaimoni::Token<char>(define_buf,0,strlen(define_buf),0);
		// UINT_LEAST_MAX
		memset_strcpy<sizeof("#define ")-1>(define_buf,"UINT_LEAST",define_buf.size());
		strcat(define_buf,buf+1);
		strcat(define_buf,"_MAX");
		strcat(define_buf,unsigned_max_buf[least_type-1]);
		tmp[i--] = new zaimoni::Token<char>(define_buf,0,strlen(define_buf),0);
		// INT_LEAST_MAX
		memset_strcpy<sizeof("#define ")-1>(define_buf,"INT_LEAST",define_buf.size());
		strcat(define_buf,buf+1);
		strcat(define_buf,"_MAX");
		strcat(define_buf,signed_max_buf[least_type-1]);
		tmp[i--] = new zaimoni::Token<char>(define_buf,0,strlen(define_buf),0);
		// INT_LEAST_MIN
		memset_strcpy<sizeof("#define ")-1>(define_buf,"INT_LEAST",define_buf.size());
		strcat(define_buf,buf+1);
		strcat(define_buf,"_MIN ");
		strcat(define_buf,signed_min_buf[least_type-1]);
		tmp[i--] = new zaimoni::Token<char>(define_buf,0,strlen(define_buf),0);
		// uint_least
		memset_strcpy<sizeof("typedef ")-1,sizeof(typedef_buf)>(typedef_buf,unsigned_type_from_machine(least_type));
		strcat(typedef_buf," uint_least");
		strcat(typedef_buf,buf+1);
		strcat(typedef_buf,"_t;");
		tmp[i--] = new zaimoni::Token<char>(typedef_buf,0,strlen(typedef_buf),0);
		// int_least
		memset_strcpy<sizeof("typedef ")-1,sizeof(typedef_buf)>(typedef_buf,signed_type_from_machine(least_type));
		strcat(typedef_buf," int_least");
		strcat(typedef_buf,buf+1);
		strcat(typedef_buf,"_t;");
		tmp[i--] = new zaimoni::Token<char>(typedef_buf,0,strlen(typedef_buf),0);
		// uint_fast
		memset_strcpy<sizeof("typedef ")-1,sizeof(typedef_buf)>(typedef_buf,unsigned_type_from_machine(fast_type));
		strcat(typedef_buf," uint_fast");
		strcat(typedef_buf,buf+1);
		strcat(typedef_buf,"_t;");
		tmp[i--] = new zaimoni::Token<char>(typedef_buf,0,strlen(typedef_buf),0);
		// int_fast
		memset_strcpy<sizeof("typedef ")-1,sizeof(typedef_buf)>(typedef_buf,signed_type_from_machine(least_type));
		strcat(typedef_buf," int_fast");
		strcat(typedef_buf,buf+1);
		strcat(typedef_buf,"_t;");
		tmp[i] = new zaimoni::Token<char>(typedef_buf,0,strlen(typedef_buf),0);
		}
	while(0<i);
	zaimoni::swap(TokenList,TmpTokenList);

	final_init_tokenlist(TokenList.c_array(),TokenList.size(),header_name);
}

