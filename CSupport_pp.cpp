// CSupport_pp.cpp
// support for C/C++ parsing
// (C)2009-2011 Kenneth Boyd, license: MIT.txt

#include "CSupport_pp.hpp"
#include "_CSupport1.hpp"
#include "_CSupport2.hpp"

#include "Zaimoni.STL/MetaRAM2.hpp"
#include "Zaimoni.STL/lite_alg.hpp"
#include "Zaimoni.STL/LexParse/LangConf.hpp"
#include "Zaimoni.STL/search.hpp"
#include "Zaimoni.STL/simple_lock.hpp"
#include "AtomicString.h"
#include "str_aux.h"
#include "Trigraph.hpp"
#include "Flat_UNI.hpp"
#include "end_lc.hpp"
#include "end_nl.hpp"
#include "errors.hpp"
#include "errcount.hpp"
#include "CPUInfo.hpp"
#include "ParseTree.hpp"
#include "type_system_pp.hpp"
#include "type_algebra.hpp"
#include "weak_token.hpp"
#include "C_PPDecimalInteger.hpp"
#include "C_PPHexInteger.hpp"
#include "C_PPOctalInteger.hpp"
#include "C_PPDecimalFloat.hpp"
#include "C_PPHexFloat.hpp"
#include "CheckReturn.hpp"

// handle function signature differences between z_cpp and other users
#ifdef SIG_CONST_TYPES
#error internal macro SIG_CONST_TYPES already defined 
#endif
#ifdef ARG_TYPES
#error internal macro ARG_TYPES already defined 
#endif
#define SIG_CONST_TYPES 
#define ARG_TYPES 

using namespace zaimoni;
using virtual_machine::umaxint;

const char* const list_hexadecimal_digits = C_HEXADECIMAL_DIGITS;

LangConf* CLexer = NULL;
LangConf* CPlusPlusLexer = NULL;
static const virtual_machine::CPUInfo* target_machine = NULL;

/* fundamental type */
/* all atomic charcters are preprocessing punctuation */
#define CPP_FLAG_CHAR_LITERAL Flag1_LC
#define CPP_FLAG_STRING_LITERAL Flag2_LC
#define CPP_FLAG_PP_OP_PUNC Flag3_LC
#define CPP_FLAG_IDENTIFIER Flag4_LC
#define CPP_FLAG_PP_NUMERAL Flag5_LC

/* general traits */
// wide character/string literals use this
#define CPP_WIDE_LITERAL Flag13_LC
// simplify macro preprocessing
#define CPP_FLAG_PAST_MACROS Flag14_LC

#define C_WHITESPACE_NO_NEWLINE " \t\r\v\f"
#define C_WHITESPACE "\n \t\r\v\f"
#define C_ATOMIC_CHAR "()[]{};~,?"

// beginning of multilingual support
#define ERR_STR "error: "
#define WARN_STR "warning: "

// would have been in ParseTree.hpp, except that we don't have AtomicString.h there
template<size_t i> void register_token(parse_tree& x)
{
	BOOST_STATIC_ASSERT(STATIC_SIZE(x.index_tokens)>i);
	if (!x.own_index_token<i>()) return;
	const char* const tmp = register_substring(x.index_tokens[i].token.first,x.index_tokens[i].token.second);
	assert(tmp!=x.index_tokens[i].token.first);
	free(const_cast<char*>(x.index_tokens[i].token.first));
	x.index_tokens[i].token.first = tmp;
	x.control_index_token<i>(false);
}

/* need for compiler implementation */
/* remember to review pragma definitions from GCC, MSVC++, etc. */
/*
Another way to prevent a header file from being included more than once is with the `#pragma once' directive. If `#pragma once' is seen when scanning a header file, that file will never be read again, no matter what. 

`#pragma once' does not have the problems that `#import' does, but it is not recognized by all preprocessors, so you cannot rely on it in a portable program.

#pragma GCC diagnostic kind option
Modifies the disposition of a diagnostic. Note that not all diagnostics are modifiable; at the moment only warnings (normally controlled by `-W...') can be controlled, and not all of them. Use -fdiagnostics-show-option to determine which diagnostics are controllable and which option controls them. 

kind is `error' to treat this diagnostic as an error, `warning' to treat it like a warning (even if -Werror is in effect), or `ignored' if the diagnostic is to be ignored. option is a double quoted string which matches the command line option. 
          #pragma GCC diagnostic warning "-Wformat"
          #pragma GCC diagnostic error "-Wformat"
          #pragma GCC diagnostic ignored "-Wformat"
     

Note that these pragmas override any command line options. Also, while it is syntactically valid to put these pragmas anywhere in your sources, the only supported location for them is before any data or functions are defined. Doing otherwise may result in unpredictable results depending on how the optimizer manages your sources. If the same option is listed multiple times, the last one specified is the one that is in effect. This pragma is not intended to be a general purpose replacement for command line options, but for implementing strict control over project policies.

Preprocessor
#pragma GCC dependency
#pragma GCC dependency allows you to check the relative dates of the current file and another file. If the other file is more recent than the current file, a warning is issued. This is useful if the current file is derived from the other file, and should be regenerated. The other file is searched for using the normal include search path. Optional trailing text can be used to give more information in the warning message. 
          #pragma GCC dependency "parse.y"
          #pragma GCC dependency "/usr/include/time.h" rerun fixincludes
     

#pragma GCC poison
Sometimes, there is an identifier that you want to remove completely from your program, and make sure that it never creeps back in. To enforce this, you can poison the identifier with this pragma. #pragma GCC poison is followed by a list of identifiers to poison. If any of those identifiers appears anywhere in the source after the directive, it is a hard error. For example, 
          #pragma GCC poison printf sprintf fprintf
          sprintf(some_string, "hello");
     
will produce an error. 

If a poisoned identifier appears as part of the expansion of a macro which was defined before the identifier was poisoned, it will not cause an error. This lets you poison an identifier without worrying about system headers defining macros that use it. 

For example, 
          #define strrchr rindex
          #pragma GCC poison rindex
          strrchr(some_string, 'h');
     

will not produce an error. 
#pragma GCC system_header
This pragma takes no arguments. It causes the rest of the code in the current file to be treated as if it came from a system header.

C99
#pragma STDC FP_CONTRACT on-off-switch
#pragma STDC FENV_ACCESS on-off-switch
#pragma STDC CX_LIMITED_RANGE on-off-switch

2 The usual mathematical formulas for complex multiply, divide, and absolute value are
problematic because of their treatment of infinities and because of undue overflow and
underflow. The CX_LIMITED_RANGE pragma can be used to inform the
implementation that (where the state is defined) the usual mathematical formulas are
acceptable.165) The pragma can occur either outside external declarations or preceding all
explicit declarations and statements inside a compound statement. When outside external
declarations, the pragma takes effect from its occurrence until another
CX_LIMITED_RANGE pragma is encountered, or until the end of the translation unit.
When inside a compound statement, the pragma takes effect from its occurrence until
another CX_LIMITED_RANGE pragma is encountered (including within a nested
165) The purpose of the pragma is to allow the implementation to use the formulas:
(x + iy) * (u + iv) = (xu . yv) + i(yu + xv)
(x + iy) / (u + iv) = [(xu + yv) + i(yu . xv)]/(u2 + v2)
| x + iy | = *. .... x2 + y2
where the programmer can determine they are safe.

The FENV_ACCESS pragma provides a means to inform the implementation when a
program might access the floating-point environment to test floating-point status flags or
run under non-default floating-point control modes.178) The pragma shall occur either
outside external declarations or preceding all explicit declarations and statements inside a
compound statement. When outside external declarations, the pragma takes effect from
its occurrence until another FENV_ACCESS pragma is encountered, or until the end of
the translation unit. When inside a compound statement, the pragma takes effect from its
occurrence until another FENV_ACCESS pragma is encountered (including within a
nested compound statement), or until the end of the compound statement; at the end of a
compound statement the state for the pragma is restored to its condition just before the
compound statement. If this pragma is used in any other context, the behavior is
undefined. If part of a program tests floating-point status flags, sets floating-point control
modes, or runs under non-default mode settings, but was translated with the state for the
FENV_ACCESS pragma ��off��, the behavior is undefined. The default state (��on�� or
��off��) for the pragma is implementation-defined. (When execution passes from a part of
the program translated with FENV_ACCESS ��off�� to a part translated with
FENV_ACCESS ��on��, the state of the floating-point status flags is unspecified and the
floating-point control modes have their default settings.)
178) The purpose of the FENV_ACCESS pragma is to allow certain optimizations that could subvert flag
tests and mode changes (e.g., global common subexpression elimination, code motion, and constant
folding). In general, if the state of FENV_ACCESS is ��off��, the translator can assume that default
modes are in effect and the flags are not tested.

The FP_CONTRACT pragma can be used to allow (if the state is ��on��) or disallow (if the
state is ��off��) the implementation to contract expressions (6.5). Each pragma can occur
either outside external declarations or preceding all explicit declarations and statements
inside a compound statement. When outside external declarations, the pragma takes
effect from its occurrence until another FP_CONTRACT pragma is encountered, or until
the end of the translation unit. When inside a compound statement, the pragma takes
effect from its occurrence until another FP_CONTRACT pragma is encountered
(including within a nested compound statement), or until the end of the compound
statement; at the end of a compound statement the state for the pragma is restored to its
condition just before the compound statement. If this pragma is used in any other
context, the behavior is undefined. The default state (��on�� or ��off��) for the pragma is
implementation-defined.
198) The term underflow here is intended to encompass both ��gradual underflow�� as in IEC 60559 and
also ��flush-to-zero�� underflow.

*/

bool IsUnaccentedAlphabeticChar(unsigned char x)
{	// FORMALLY CORRECT: Kenneth Boyd, 7/29/2001
	return in_range<'A','Z'>(x) || in_range<'a','z'>(x);
}

bool IsAlphabeticChar(unsigned char x)
{	// FORMALLY CORRECT: Kenneth Boyd, 7/27/2001
	// META: uses ASCII/default ISO web encoding implicitly
	// NOTE: lower-case eth (240) will pass as partial differential operator!
	if (   IsUnaccentedAlphabeticChar(x)
//		|| (unsigned char)('\x8c')==x				// OE ligature
//		|| (unsigned char)('\x9c')==x				// oe ligature
//		|| (unsigned char)('\x9f')==x				// Y umlaut
		|| in_range<'\xc0','\xd6'>(x)	// various accented characters
		|| in_range<'\xd8','\xf6'>(x)	// various accented characters
		|| ((unsigned char)('\xf8')<=x /* && (unsigned char)('\xff')>=x */))	// various accented characters
		return true;
	return false;
}

bool C_IsLegalSourceChar(char x)
{
	if (   IsAlphabeticChar(x) || in_range<'0','9'>(x)
		|| strchr(C_WHITESPACE,x) || strchr(C_ATOMIC_CHAR,x)
		|| strchr("_#<>%:.*+�/^&|!=\\",x))
		return true;
	return false;
}

static bool C_IsPrintableChar(unsigned char x)
{
	return in_range<' ','~'>(x);	//! \todo fix; assumes ASCII
}

#if 0
	identifier
		nondigit
		identifier nondigit
		identifier digit
		nondigit: one of
			universal-character-name
			_ a b c d e f g h i j k l m
			  n o p q r s t u v w x y z
			  A B C D E F G H I J K L M
			  N O P Q R S T U V W X Y Z
		digit: one of
			0 1 2 3 4 5 6 7 8 9
#endif

size_t LengthOfCIdentifier(const char* const x)
{	//! \todo should handle universal character names
	assert(x);
	const char* x2 = x;
	if (IsAlphabeticChar(*x2) || '_'==*x2)
		while(IsCIdentifierChar(*++x2));
	return x2-x;
}

#if 0
	pp-number
		digit
		. digit
		pp-number digit
		pp-number nondigit
		pp-number e sign
		pp-number E sign
		pp-number .
#endif
size_t LengthOfCPreprocessingNumber(const char* const x)
{
	assert(x);
	size_t i = 0;	// Length
	if (IsNumericChar(*x)) i = 1;
	else if ('.'==*x && IsNumericChar(x[1])) i = 2;
	if (0<i)
		do	if ('.'==x[i] || IsNumericChar(x[i]))
				++i;
			else if (IsAlphabeticChar(x[i]))
				{
				if (   ('+'==x[i+1] || '-'==x[i+1])
					&& ('E'==x[i] || 'e'==x[i] || 'P'==x[i] || 'p'==x[i]))
					i += 2;
				else
					i += 1;
				}
			else return i;
		while(1);
	return 0;
}

size_t LengthOfCCharLiteral(const char* const x)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	size_t Length;
	if ('\''==*x) Length = 1;
	else if ('L'==x[0] && '\''==x[1]) Length = 2;
	else return 0;

	const char* base = x+Length;
	const char* find_end = strpbrk(base,"\\'\n");
	while(find_end)
		{
		Length = find_end-x+1;
		if ('\''==find_end[0]) return Length;
		if ('\n'==find_end[0]) return Length-1;
		if ('\0'==find_end[1]) return Length;
		base = find_end+2;
		find_end = !base[0] ? NULL : strpbrk(base,"\\'\n");
		};
	return strlen(x);
}

size_t LengthOfCStringLiteral(const char* const x)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	size_t Length;
	if ('"'==*x) Length = 1;
	else if ('L'==x[0] && '"'==x[1]) Length = 2;
	else return 0;

	const char* base = x+Length;
	const char* find_end = strpbrk(base,"\\\"\n");
	while(find_end)
		{
		Length = find_end-x+1;
		if ('"'==find_end[0]) return Length;
		if ('\n'==find_end[0]) return Length-1;
		if ('\0'==find_end[1]) return Length;
		base = find_end+2;
		find_end = !base[0] ? NULL : strpbrk(base,"\\\"\n");
		};
	return strlen(x);
}

#if 0
          preprocessing-op-or-punc: one of
          {    }       [       ]      #      ##    (       )
          <:   :>      <%      %>     %:     %:%:  ;       :    ...
          new  delete  ?       ::     .      .*
          +    -       *       /      %      ^     &       |    ~
          !    =       <       >      +=     -=    *=      /=   %=
          ^=   &=      |=      <<     >>     >>=   <<=     ==   !=
          <=   >=      &&      ||     ++     --    ,       ->*  ->
          and  and_eq  bitand  bitor  compl  not   not_eq  or   or_eq 
          xor  xor_eq
#endif

#define ATOMIC_PREPROC_PUNC "()[]{};~,?"

static const POD_triple<const char*,size_t,unsigned int> valid_pure_preprocessing_op_punc[]
	=	{	DICT2_STRUCT("{",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT("}",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT("[",0),
			DICT2_STRUCT("]",0),
			DICT2_STRUCT("(",0),
			DICT2_STRUCT(")",0),
			DICT2_STRUCT(";",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT("~",0),
			DICT2_STRUCT(",",C_DISALLOW_CONSTANT_EXPR),	// double-check this
			DICT2_STRUCT("?",0),	// atomic

			DICT2_STRUCT(".",C_DISALLOW_IF_ELIF_CONTROL),
			DICT2_STRUCT("&",0),
			DICT2_STRUCT("+",0),
			DICT2_STRUCT("-",0),
			DICT2_STRUCT("*",0),
			DICT2_STRUCT("/",0),
			DICT2_STRUCT("%",0),
			DICT2_STRUCT("!",0),
			DICT2_STRUCT(":",0),
			DICT2_STRUCT("=",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT("<",0),
			DICT2_STRUCT(">",0),
			DICT2_STRUCT("^",0),
			DICT2_STRUCT("|",0),
			DICT2_STRUCT("#",C_DISALLOW_POSTPROCESSED_SOURCE),
			DICT2_STRUCT("##",C_DISALLOW_POSTPROCESSED_SOURCE),
			DICT2_STRUCT("->",C_DISALLOW_IF_ELIF_CONTROL),
			DICT2_STRUCT("++",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT("--",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT("<:",0),
			DICT2_STRUCT(":>",0),
			DICT2_STRUCT("<%",C_DISALLOW_CONSTANT_EXPR),	// }
			DICT2_STRUCT("%>",C_DISALLOW_CONSTANT_EXPR),	// {
			DICT2_STRUCT("%:",C_DISALLOW_POSTPROCESSED_SOURCE),	// #
			DICT2_STRUCT("+=",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT("-=",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT("*=",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT("/=",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT("%=",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT("&=",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT("|=",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT("^=",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT("<<",0),
			DICT2_STRUCT(">>",0),
			DICT2_STRUCT("==",0),
			DICT2_STRUCT("!=",0),
			DICT2_STRUCT("<=",0),
			DICT2_STRUCT(">=",0),
			DICT2_STRUCT("&&",0),
			DICT2_STRUCT("||",0),
			DICT2_STRUCT("...",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT("<<=",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT(">>=",C_DISALLOW_CONSTANT_EXPR),
			DICT2_STRUCT("%:%:",C_DISALLOW_POSTPROCESSED_SOURCE),	// ##	// C99

			DICT2_STRUCT("::",C_DISALLOW_IF_ELIF_CONTROL),
			DICT2_STRUCT(".*",C_DISALLOW_IF_ELIF_CONTROL),
			DICT2_STRUCT("->*",C_DISALLOW_IF_ELIF_CONTROL)		// C++0x
		};


#define CPP_PREPROC_OP_STRICT_UB STATIC_SIZE(valid_pure_preprocessing_op_punc)
#define C_PREPROC_OP_STRICT_UB (CPP_PREPROC_OP_STRICT_UB-3)
#define NONATOMIC_PREPROC_OP_LB 10

BOOST_STATIC_ASSERT(NONATOMIC_PREPROC_OP_LB<C_PREPROC_OP_STRICT_UB);

static const POD_pair<const char*,size_t> valid_keyword[]
	=	{	DICT_STRUCT("__asm"),		// reserved to the implementation, so OK to make a keyword for C only
			DICT_STRUCT("_Static_assert"),	// C1X keywords not in C++0X
			DICT_STRUCT("_Thread_local"),
			DICT_STRUCT("restrict"),	// C99 keywords not in C++98
			DICT_STRUCT("_Bool"),
			DICT_STRUCT("_Complex"),
			DICT_STRUCT("_Imaginary"),
			DICT_STRUCT("auto"),		// joint keywords in C99 and C++98
			DICT_STRUCT("break"),
			DICT_STRUCT("case"),
			DICT_STRUCT("char"),
			DICT_STRUCT("const"),
			DICT_STRUCT("continue"),
			DICT_STRUCT("default"),
			DICT_STRUCT("do"),
			DICT_STRUCT("double"),
			DICT_STRUCT("else"),
			DICT_STRUCT("enum"),
			DICT_STRUCT("extern"),
			DICT_STRUCT("float"),
			DICT_STRUCT("for"),
			DICT_STRUCT("goto"),
			DICT_STRUCT("if"),
			DICT_STRUCT("inline"),
			DICT_STRUCT("int"),
			DICT_STRUCT("long"),
			DICT_STRUCT("register"),
			DICT_STRUCT("return"),
			DICT_STRUCT("short"),
			DICT_STRUCT("signed"),
			DICT_STRUCT("sizeof"),
			DICT_STRUCT("static"),
			DICT_STRUCT("struct"),
			DICT_STRUCT("switch"),
			DICT_STRUCT("typedef"),
			DICT_STRUCT("union"),
			DICT_STRUCT("unsigned"),
			DICT_STRUCT("void"),
			DICT_STRUCT("volatile"),
			DICT_STRUCT("while"),		// C99 keywords
			DICT_STRUCT("asm"),			// common non-conforming extension to C99, C++98 keyword
			DICT_STRUCT("bool"),		// start C++98 keywords
			DICT_STRUCT("catch"),
			DICT_STRUCT("class"),
			DICT_STRUCT("const_cast"),
			DICT_STRUCT("delete"),
			DICT_STRUCT("dynamic_cast"),
			DICT_STRUCT("explicit"),
			DICT_STRUCT("false"),
			DICT_STRUCT("friend"),
			DICT_STRUCT("mutable"),
			DICT_STRUCT("namespace"),
			DICT_STRUCT("new"),
			DICT_STRUCT("operator"),
			DICT_STRUCT("private"),
			DICT_STRUCT("protected"),
			DICT_STRUCT("public"),
			DICT_STRUCT("reinterpret_cast"),
			DICT_STRUCT("static_cast"),
			DICT_STRUCT("template"),
			DICT_STRUCT("this"),
			DICT_STRUCT("throw"),
			DICT_STRUCT("true"),
			DICT_STRUCT("try"),
			DICT_STRUCT("typeid"),
			DICT_STRUCT("typename"),
			DICT_STRUCT("using"),
			DICT_STRUCT("virtual"),
			DICT_STRUCT("wchar_t"),		// end C++98 keywords
			DICT_STRUCT("and"),			// C++98 alternate-operators
			DICT_STRUCT("and_eq"),
			DICT_STRUCT("bitand"),
			DICT_STRUCT("bitor"),
			DICT_STRUCT("compl"),
			DICT_STRUCT("not"),
			DICT_STRUCT("not_eq"),
			DICT_STRUCT("or"),
			DICT_STRUCT("or_eq"),
			DICT_STRUCT("xor"),
			DICT_STRUCT("xor_eq"),		// end C++98 alternate-operators
			DICT_STRUCT("constexpr"),	// C++0X keywords we pay attention to
			DICT_STRUCT("noexcept"),	// C++0X n3090
			DICT_STRUCT("static_assert"),
			DICT_STRUCT("thread_local")
		};

// think about C++0x keywords later.
#define C_KEYWORD_NONSTRICT_LB 0
#define CPP_KEYWORD_NONSTRICT_LB 7
#define C_KEYWORD_STRICT_UB 40
#define CPP_KEYWORD_STRICT_UB STATIC_SIZE(valid_keyword)

BOOST_STATIC_ASSERT(C_KEYWORD_NONSTRICT_LB<C_KEYWORD_STRICT_UB);
BOOST_STATIC_ASSERT(CPP_KEYWORD_NONSTRICT_LB<C_KEYWORD_STRICT_UB);

namespace C_TYPE {

enum hard_type_indexes {
	VOID = 1,
	NOT_VOID,	// needs to be omnicompatible early on
	BOOL,
	CHAR,
	SCHAR,
	UCHAR,
	SHRT,
	USHRT,
	INT,
	UINT,
	LONG,
	ULONG,
	LLONG,
	ULLONG,
	INTEGERLIKE,
	FLOAT,
	DOUBLE,
	LDOUBLE,
	FLOAT__COMPLEX,
	DOUBLE__COMPLEX,
	LDOUBLE__COMPLEX,
	WCHAR_T,	// C++-specific
	TYPEINFO	// C++-specific
};

}

static inline virtual_machine::std_int_enum machine_type_from_type_index(size_t base_type_index)
{
	assert(C_TYPE::INT<=base_type_index && C_TYPE::ULLONG>=base_type_index);
	return (virtual_machine::std_int_enum)((base_type_index-C_TYPE::INT)/2+virtual_machine::std_int_int);
}

static bool is_innate_definite_type(size_t base_type_index)
{
	return C_TYPE::BOOL<=base_type_index && C_TYPE::LDOUBLE__COMPLEX>=base_type_index;
}

static bool converts_to_integerlike(size_t base_type_index SIG_CONST_TYPES)
{	//! \todo handle cast operator overloading
	return C_TYPE::BOOL<=base_type_index && C_TYPE::INTEGERLIKE>=base_type_index;
}

static bool converts_to_integerlike(const type_spec& type_code SIG_CONST_TYPES)
{	//! \todo handle cast operator overloading
	if (0<type_code.pointer_power) return false;	// pointers do not have a standard conversion to integers
	return converts_to_integerlike(type_code.base_type_index ARG_TYPES);
}

static bool converts_to_integer(const type_spec& type_code SIG_CONST_TYPES)
{	//! \todo handle cast operator overloading
	if (0<type_code.pointer_power) return false;	// pointers do not have a standard conversion to integers
	return C_TYPE::BOOL<=type_code.base_type_index && C_TYPE::INTEGERLIKE>type_code.base_type_index;
}

static bool converts_to_reallike(size_t base_type_index SIG_CONST_TYPES)
{	//! \todo handle cast operator overloading
	return C_TYPE::BOOL<=base_type_index && C_TYPE::LDOUBLE>=base_type_index;
}

static bool converts_to_arithmeticlike(size_t base_type_index SIG_CONST_TYPES)
{	//! \todo handle cast operator overloading
	return C_TYPE::BOOL<=base_type_index && C_TYPE::LDOUBLE__COMPLEX>=base_type_index;
}

static bool converts_to_arithmeticlike(const type_spec& type_code SIG_CONST_TYPES)
{	//! \todo handle cast operator overloading
	if (0<type_code.pointer_power) return false;	// pointers do not have a standard conversion to integers/floats/complex
	return converts_to_arithmeticlike(type_code.base_type_index ARG_TYPES);
}

static bool converts_to_bool(const type_spec& type_code SIG_CONST_TYPES)
{
	if (0<type_code.pointer_power) return true;	// pointers are comparable to NULL
	if (converts_to_arithmeticlike(type_code.base_type_index ARG_TYPES)) return true;	// arithmetic types are comparable to zero, and include bool
	// C++: run through type conversion weirdness
	return false;
}

// the integer promotions rely on low-level weirdness, so test that here
static size_t arithmetic_reconcile(size_t base_type_index1, size_t base_type_index2 SIG_CONST_TYPES)
{
	assert(is_innate_definite_type(base_type_index1));
	assert(is_innate_definite_type(base_type_index2));
	// identity, do not do anything
	if (base_type_index1==base_type_index2) return base_type_index1;

	//! \todo --do-what-i-mean will try to value-preserve integers when promoting to a float type (use global target_machine)

	// long double _Complex
	if (C_TYPE::LDOUBLE__COMPLEX==base_type_index1) return C_TYPE::LDOUBLE__COMPLEX;
	if (C_TYPE::LDOUBLE__COMPLEX==base_type_index2) return C_TYPE::LDOUBLE__COMPLEX;
	if (C_TYPE::LDOUBLE==base_type_index1 && C_TYPE::FLOAT__COMPLEX<=base_type_index2) return C_TYPE::LDOUBLE__COMPLEX;
	if (C_TYPE::LDOUBLE==base_type_index2 && C_TYPE::FLOAT__COMPLEX<=base_type_index1) return C_TYPE::LDOUBLE__COMPLEX;

	// double _Complex
	if (C_TYPE::DOUBLE__COMPLEX==base_type_index1) return C_TYPE::DOUBLE__COMPLEX;
	if (C_TYPE::DOUBLE__COMPLEX==base_type_index2) return C_TYPE::DOUBLE__COMPLEX;
	if (C_TYPE::DOUBLE==base_type_index1 && C_TYPE::FLOAT__COMPLEX<=base_type_index2) return C_TYPE::DOUBLE__COMPLEX;
	if (C_TYPE::DOUBLE==base_type_index2 && C_TYPE::FLOAT__COMPLEX<=base_type_index1) return C_TYPE::DOUBLE__COMPLEX;

	// float _Complex
	if (C_TYPE::FLOAT__COMPLEX==base_type_index1) return C_TYPE::FLOAT__COMPLEX;
	if (C_TYPE::FLOAT__COMPLEX==base_type_index2) return C_TYPE::FLOAT__COMPLEX;

	// long double
	if (C_TYPE::LDOUBLE==base_type_index1) return C_TYPE::LDOUBLE;
	if (C_TYPE::LDOUBLE==base_type_index2) return C_TYPE::LDOUBLE;

	// double
	if (C_TYPE::DOUBLE==base_type_index1) return C_TYPE::DOUBLE;
	if (C_TYPE::DOUBLE==base_type_index2) return C_TYPE::DOUBLE;

	// float
	if (C_TYPE::FLOAT==base_type_index1) return C_TYPE::FLOAT;
	if (C_TYPE::FLOAT==base_type_index2) return C_TYPE::FLOAT;

	// bool fits in any integer type
	if (C_TYPE::BOOL==base_type_index1) return base_type_index2;
	if (C_TYPE::BOOL==base_type_index2) return base_type_index1;

	// any integer type fits in integerlike
	if (C_TYPE::INTEGERLIKE==base_type_index1) return C_TYPE::INTEGERLIKE;
	if (C_TYPE::INTEGERLIKE==base_type_index2) return C_TYPE::INTEGERLIKE;

	// handle indeterminacy of char now
	BOOST_STATIC_ASSERT(C_TYPE::SCHAR+1==C_TYPE::UCHAR);
	if (C_TYPE::CHAR==base_type_index1)
		{
		base_type_index1 = C_TYPE::SCHAR + bool_options[boolopt::char_is_unsigned];
		// identity, do not do anything
		if (base_type_index1==base_type_index2) return base_type_index1;
		}
	if (C_TYPE::CHAR==base_type_index2)
		{
		base_type_index2 = C_TYPE::SCHAR + bool_options[boolopt::char_is_unsigned];
		// identity, do not do anything
		if (base_type_index1==base_type_index2) return base_type_index1;
		}

	// types of the same sign should have a difference divisible by 2
	BOOST_STATIC_ASSERT(0==(C_TYPE::SHRT-C_TYPE::SCHAR)%2);
	BOOST_STATIC_ASSERT(0==(C_TYPE::INT-C_TYPE::SHRT)%2);
	BOOST_STATIC_ASSERT(0==(C_TYPE::LONG-C_TYPE::INT)%2);
	BOOST_STATIC_ASSERT(0==(C_TYPE::LLONG-C_TYPE::LONG)%2);

	BOOST_STATIC_ASSERT(0==(C_TYPE::USHRT-C_TYPE::UCHAR)%2);
	BOOST_STATIC_ASSERT(0==(C_TYPE::UINT-C_TYPE::USHRT)%2);
	BOOST_STATIC_ASSERT(0==(C_TYPE::ULONG-C_TYPE::UINT)%2);
	BOOST_STATIC_ASSERT(0==(C_TYPE::ULLONG-C_TYPE::ULONG)%2);
	if (0==(base_type_index2-base_type_index1)%2) return (base_type_index1<base_type_index1) ? base_type_index1 : base_type_index1;

	// types of the same rank should calculate as having the same rank
	BOOST_STATIC_ASSERT((C_TYPE::SCHAR-1)/2==(C_TYPE::UCHAR-1)/2);
	BOOST_STATIC_ASSERT((C_TYPE::SHRT-1)/2==(C_TYPE::USHRT-1)/2);
	BOOST_STATIC_ASSERT((C_TYPE::INT-1)/2==(C_TYPE::UINT-1)/2);
	BOOST_STATIC_ASSERT((C_TYPE::LONG-1)/2==(C_TYPE::ULONG-1)/2);
	BOOST_STATIC_ASSERT((C_TYPE::LLONG-1)/2==(C_TYPE::ULLONG-1)/2);

	// signed types should be odd, unsigned types should be even
	BOOST_STATIC_ASSERT(0!=C_TYPE::SCHAR%2);
	BOOST_STATIC_ASSERT(0==C_TYPE::UCHAR%2);
	BOOST_STATIC_ASSERT(0!=C_TYPE::SHRT%2);
	BOOST_STATIC_ASSERT(0==C_TYPE::USHRT%2);
	BOOST_STATIC_ASSERT(0!=C_TYPE::INT%2);
	BOOST_STATIC_ASSERT(0==C_TYPE::UINT%2);
	BOOST_STATIC_ASSERT(0!=C_TYPE::LONG%2);
	BOOST_STATIC_ASSERT(0==C_TYPE::ULONG%2);
	BOOST_STATIC_ASSERT(0!=C_TYPE::LLONG%2);
	BOOST_STATIC_ASSERT(0==C_TYPE::ULLONG%2);

	//! \todo --do-what-i-mean is a bit more careful about signed/unsigned of the same rank; it promotes an equal-rank mismatch to the next larger signed integer type
	if (0==base_type_index1%2)
		{	// first is unsigned
		if ((base_type_index1-1)/2>=(base_type_index2-1)/2)
			return base_type_index1;
		else
			return base_type_index2;
		}
	else{	// second is unsigned
		if ((base_type_index1-1)/2<=(base_type_index2-1)/2)
			return base_type_index2;
		else
			return base_type_index1;
		}
}

static size_t default_promote_type(size_t i SIG_CONST_TYPES)
{
	switch(i)
	{
	case C_TYPE::BOOL: return C_TYPE::INT;
	case C_TYPE::SCHAR: return C_TYPE::INT;
	case C_TYPE::SHRT: return C_TYPE::INT;
	case C_TYPE::UCHAR: return (1<target_machine->C_sizeof_int()) ? C_TYPE::INT : C_TYPE::UINT;
	case C_TYPE::CHAR: return (1<target_machine->C_sizeof_int() || target_machine->char_is_signed_char()) ? C_TYPE::INT : C_TYPE::UINT;
	case C_TYPE::USHRT: return (target_machine->C_sizeof_short()<target_machine->C_sizeof_int()) ? C_TYPE::INT : C_TYPE::UINT;
	case C_TYPE::FLOAT: return C_TYPE::DOUBLE;
	};
	return i;
}

static POD_pair<size_t,bool> default_promotion_is_integerlike(const type_spec& type_code SIG_CONST_TYPES)
{	// uses NRVO
	POD_pair<size_t,bool> tmp = {0,false};
	if (0==type_code.pointer_power)	// pointers do not have a standard conversion to integers
		{
		tmp.first = default_promote_type(type_code.base_type_index ARG_TYPES);
		tmp.second = (C_TYPE::BOOL<=tmp.first && C_TYPE::INTEGERLIKE>=tmp.first);
		}
	return tmp;
}

static POD_pair<size_t,bool> default_promotion_is_arithmeticlike(const type_spec& type_code SIG_CONST_TYPES)
{	// uses NRVO
	POD_pair<size_t,bool> tmp = {0,false};
	if (0==type_code.pointer_power)	// pointers do not have a standard conversion to integers
		{
		tmp.first = default_promote_type(type_code.base_type_index ARG_TYPES);
		tmp.second = (C_TYPE::BOOL<=tmp.first && C_TYPE::LDOUBLE__COMPLEX>=tmp.first);
		}
	return tmp;
}

// auxilliary structure to aggregate useful information for type promotions
// this will malfunction badly for anything other than an integer type
class promote_aux : public virtual_machine::promotion_info
{
public:
	promote_aux(size_t base_type_index SIG_CONST_TYPES)
	{
		const size_t promoted_type = default_promote_type(base_type_index ARG_TYPES);
		machine_type = machine_type_from_type_index(promoted_type);
		bitcount = target_machine->C_bit(machine_type);
		is_signed = !((promoted_type-C_TYPE::INT)%2);
	};
};

static const char* literal_suffix(size_t i)
{
	switch(i)
	{
	case C_TYPE::UINT: return "U";
	case C_TYPE::LDOUBLE:
	case C_TYPE::LONG: return "L";
	case C_TYPE::ULONG: return "UL";
	case C_TYPE::LLONG: return "LL";
	case C_TYPE::ULLONG: return "ULL";
	case C_TYPE::FLOAT: return "F";
	}
	return NULL;
}

static lex_flags literal_flags(size_t i)
{
	switch(i)
	{
	case C_TYPE::CHAR:		return C_TESTFLAG_CHAR_LITERAL;
	case C_TYPE::INT:		return C_TESTFLAG_PP_NUMERAL | C_TESTFLAG_INTEGER;
	case C_TYPE::UINT:		return C_TESTFLAG_PP_NUMERAL | C_TESTFLAG_INTEGER | C_TESTFLAG_U;
	case C_TYPE::LONG:		return C_TESTFLAG_PP_NUMERAL | C_TESTFLAG_INTEGER | C_TESTFLAG_L;
	case C_TYPE::ULONG:		return C_TESTFLAG_PP_NUMERAL | C_TESTFLAG_INTEGER | C_TESTFLAG_U | C_TESTFLAG_L;
	case C_TYPE::LLONG:		return C_TESTFLAG_PP_NUMERAL | C_TESTFLAG_INTEGER | C_TESTFLAG_LL;
	case C_TYPE::ULLONG:	return C_TESTFLAG_PP_NUMERAL | C_TESTFLAG_INTEGER | C_TESTFLAG_U | C_TESTFLAG_LL;
	case C_TYPE::FLOAT:		return C_TESTFLAG_PP_NUMERAL | C_TESTFLAG_FLOAT | C_TESTFLAG_F;
	case C_TYPE::DOUBLE:	return C_TESTFLAG_PP_NUMERAL | C_TESTFLAG_FLOAT;
	case C_TYPE::LDOUBLE:	return C_TESTFLAG_PP_NUMERAL | C_TESTFLAG_FLOAT | C_TESTFLAG_L;
	}
	return 0;
}

/* reference arrays for instantiating type_system class with */
/* typenames starting with $ are internal, as $ is not a legal C-source character */
const POD_pair<const char* const,size_t> C_atomic_types[]
	=	{
		DICT_STRUCT("void"),
		DICT_STRUCT("$not-void"),
		DICT_STRUCT("_Bool"),
		DICT_STRUCT("char"),
		DICT_STRUCT("signed char"),
		DICT_STRUCT("unsigned char"),
		DICT_STRUCT("short"),
		DICT_STRUCT("unsigned short"),
		DICT_STRUCT("int"),
		DICT_STRUCT("unsigned"),
		DICT_STRUCT("long"),
		DICT_STRUCT("unsigned long"),
		DICT_STRUCT("long long"),
		DICT_STRUCT("unsigned long long"),
		DICT_STRUCT("$integer-like"),
		DICT_STRUCT("float"),
		DICT_STRUCT("double"),
		DICT_STRUCT("long double"),
		DICT_STRUCT("float _Complex"),		/* start C++ extension support: C99 _Complex in C++ (we can do this as _Complex is reserved to the implementation) */
		DICT_STRUCT("double _Complex"),
		DICT_STRUCT("long double _Complex")
		};

const POD_pair<const char* const,size_t> CPP_atomic_types[]
	=	{
		DICT_STRUCT("void"),
		DICT_STRUCT("$not-void"),
		DICT_STRUCT("bool"),
		DICT_STRUCT("char"),
		DICT_STRUCT("signed char"),
		DICT_STRUCT("unsigned char"),
		DICT_STRUCT("short"),
		DICT_STRUCT("unsigned short"),
		DICT_STRUCT("int"),
		DICT_STRUCT("unsigned"),
		DICT_STRUCT("long"),
		DICT_STRUCT("unsigned long"),
		DICT_STRUCT("long long"),
		DICT_STRUCT("unsigned long long"),
		DICT_STRUCT("$integer-like"),
		DICT_STRUCT("float"),
		DICT_STRUCT("double"),
		DICT_STRUCT("long double"),
		DICT_STRUCT("float _Complex"),		/* start C++ extension support: C99 _Complex in C++ (we can do this as _Complex is reserved to the implementation) */
		DICT_STRUCT("double _Complex"),
		DICT_STRUCT("long double _Complex"),
		DICT_STRUCT("wchar_t"),			// C++-specific
		DICT_STRUCT("std::typeinfo")	// C++-specific
		};

BOOST_STATIC_ASSERT(STATIC_SIZE(C_atomic_types)==C_TYPE_MAX);
BOOST_STATIC_ASSERT(STATIC_SIZE(CPP_atomic_types)==CPP_TYPE_MAX);

const size_t C_int_priority[]
	=	{
		C_TYPE::INT,
		C_TYPE::UINT,
		C_TYPE::LONG,
		C_TYPE::ULONG,
		C_TYPE::LLONG,
		C_TYPE::ULLONG,
		C_TYPE::INTEGERLIKE
		};

BOOST_STATIC_ASSERT(C_INT_PRIORITY_SIZE==STATIC_SIZE(C_int_priority));

// this only has to work on full strings, not embedded substrings
const char* const system_headers[]
	=	{	"assert.h",		// C99 headers
			"complex.h",
			"ctype.h",
			"errno.h",
			"float.h",
			"inttypes.h",
			"iso646.h",
			"limits.h",
			"locale.h",
			"math.h",
			"setjmp.h",
			"signal.h",
			"stdarg.h",
			"stdbool.h",
			"stddef.h",
			"stdint.h",
			"stdio.h",
			"stdlib.h",
			"string.h",
			"tgmath.h",
			"time.h",
			"wchar.h",
			"wctype.h",	// end C99 headers
			"cassert",	// C++98 C-functionality headers
			"cctype",
			"cerrno",
			"cfloat",
			"ciso646",
			"climits",
			"clocale",
			"cmath",
			"csetjmp",
			"csignal",
			"cstdarg",
			"cstddef",
			"cstdio",
			"cstdlib",
			"cstring",
			"ctime",
			"cwchar",
			"cwctype",	// end C++98 C-functionality headers
			"algorithm",	// C++98 headers proper
			"bitset",
			"complex",
			"deque",
			"exception",
			"fstream",
			"functional",
			"iomanip",
			"ios",
			"iosfwd",
			"iostream",
			"istream",
			"iterator",
			"limits",
			"list",
			"locale",
			"map",
			"memory",
			"new",
			"numeric",
			"ostream",
			"queue",
			"set",
			"sstream",
			"stack",
			"stdexcept",
			"streambuf",
			"string",
			"typeinfo",
			"utility",
			"valarray",
			"vector"	// end C++98 headers proper
		};

#define CPP_SYS_HEADER_STRICT_UB STATIC_SIZE(system_headers)
#define C_SYS_HEADER_STRICT_UB 23

static size_t LengthOfCSystemHeader(const char* src)
{
	const errr i = linear_find(src,system_headers,C_SYS_HEADER_STRICT_UB);
	if (0<=i) return strlen(system_headers[i]);
	return 0;
}

static size_t LengthOfCPPSystemHeader(const char* src)
{
	const errr i = linear_find(src,system_headers,CPP_SYS_HEADER_STRICT_UB);
	if (0<=i) return strlen(system_headers[i]);
	return 0;
}

/* XXX this may belong with weak_token XXX */
static void message_header(const weak_token& src)
{
	assert(src.src_filename && *src.src_filename);
	message_header(src.src_filename,src.logical_line.first);
}


// balanced character count
static POD_pair<size_t,size_t>
_balanced_character_count(const weak_token* tokenlist,size_t tokenlist_len,const char l_match,const char r_match)
{
	assert(tokenlist);
	assert(0<tokenlist_len);
	POD_pair<size_t,size_t> depth = {0,0};
	const weak_token* const iter_end = tokenlist+tokenlist_len;
	const weak_token* iter = tokenlist;
	do	if (1==iter->token.second)
			{
			if 		(l_match==iter->token.first[0]) ++depth.first;
			else if (r_match==iter->token.first[0]) ++depth.second;
			}
	while(++iter!=iter_end);
	return depth;
}

template<char l_match,char r_match>
inline static POD_pair<size_t,size_t> balanced_character_count(const weak_token* tokenlist,size_t tokenlist_len)
{
	assert(tokenlist);
	assert(0<tokenlist_len);
	return _balanced_character_count(tokenlist,tokenlist_len,l_match,r_match);
}

template<>
POD_pair<size_t,size_t> balanced_character_count<'[',']'>(const weak_token* tokenlist,size_t tokenlist_len)
{
	assert(tokenlist);
	assert(0<tokenlist_len);
	POD_pair<size_t,size_t> depth = {0, 0};
	const weak_token* const iter_end = tokenlist+tokenlist_len;
	const weak_token* iter = tokenlist;
	do	if 		(detect_C_left_bracket_op(iter->token.first,iter->token.second)) ++depth.first;
		else if (detect_C_right_bracket_op(iter->token.first,iter->token.second)) ++depth.second;
	while(++iter!=iter_end);
	return depth;
}

template<>
POD_pair<size_t,size_t> balanced_character_count<'{','}'>(const weak_token* tokenlist,size_t tokenlist_len)
{
	assert(tokenlist);
	assert(0<tokenlist_len);
	POD_pair<size_t,size_t> depth = {0, 0};
	const weak_token* const iter_end = tokenlist+tokenlist_len;
	const weak_token* iter = tokenlist;
	do	if 		(detect_C_left_brace_op(iter->token.first,iter->token.second)) ++depth.first;
		else if (detect_C_right_brace_op(iter->token.first,iter->token.second)) ++depth.second;
	while(++iter!=iter_end);
	return depth;
}

static POD_pair<size_t,size_t>
_balanced_character_count(const parse_tree* tokenlist,size_t tokenlist_len,const char l_match,const char r_match)
{
	assert(tokenlist);
	assert(0<tokenlist_len);
	POD_pair<size_t,size_t> depth = {0, 0};
	const parse_tree* const iter_end = tokenlist+tokenlist_len;
	const parse_tree* iter = tokenlist;
	do	if (1==iter->index_tokens[0].token.second && !iter->index_tokens[1].token.first)
			{
			if 		(l_match==iter->index_tokens[0].token.first[0]) ++depth.first;
			else if (r_match==iter->index_tokens[0].token.first[0]) ++depth.second;
			}
	while(++iter!=iter_end);
	return depth;
}

template<char l_match,char r_match>
inline static POD_pair<size_t,size_t> balanced_character_count(const parse_tree* tokenlist,size_t tokenlist_len)
{
	assert(tokenlist);
	assert(0<tokenlist_len);
	return _balanced_character_count(tokenlist,tokenlist_len,l_match,r_match);
}

template<>
POD_pair<size_t,size_t> balanced_character_count<'[',']'>(const parse_tree* tokenlist,size_t tokenlist_len)
{
	assert(tokenlist);
	assert(0<tokenlist_len);
	POD_pair<size_t,size_t> depth = {0, 0};
	const parse_tree* const iter_end = tokenlist+tokenlist_len;
	const parse_tree* iter = tokenlist;
	do	if (!iter->index_tokens[1].token.first)
			{
			if 		(detect_C_left_bracket_op(iter->index_tokens[0].token.first,iter->index_tokens[0].token.second)) ++depth.first;
			else if (detect_C_right_bracket_op(iter->index_tokens[0].token.first,iter->index_tokens[0].token.second)) ++depth.second;
			}
	while(++iter!=iter_end);
	return depth;
}

template<>
POD_pair<size_t,size_t> balanced_character_count<'{','}'>(const parse_tree* tokenlist,size_t tokenlist_len)
{
	assert(tokenlist);
	assert(0<tokenlist_len);
	POD_pair<size_t,size_t> depth = {0, 0};
	const parse_tree* const iter_end = tokenlist+tokenlist_len;
	const parse_tree* iter = tokenlist;
	do	if (!iter->index_tokens[1].token.first)
			{
			if 		(detect_C_left_brace_op(iter->index_tokens[0].token.first,iter->index_tokens[0].token.second)) ++depth.first;
			else if (detect_C_right_brace_op(iter->index_tokens[0].token.first,iter->index_tokens[0].token.second)) ++depth.second;
			}
	while(++iter!=iter_end);
	return depth;
}

static void unbalanced_error(const weak_token& src,size_t count, char match)
{
	assert(0<count);
	message_header(src);
	INC_INFORM(ERR_STR);
	INC_INFORM(count);
	INC_INFORM(" unbalanced '");
	INC_INFORM(match);
	INFORM('\'');
	zcc_errors.inc_error();
}

static void _construct_matched_pairs(const weak_token* tokenlist,size_t tokenlist_len, autovalarray_ptr<POD_pair<size_t,size_t> >& stack1,const char l_match,const char r_match)
{
	assert(tokenlist);
	assert(0<tokenlist_len);
	POD_pair<size_t,size_t> depth = _balanced_character_count(tokenlist,tokenlist_len,l_match,r_match);	// pre-scan
	std::pair<size_t,size_t> unbalanced_loc(0,0);
	const size_t starting_errors = zcc_errors.err_count();

	if (0<depth.first && 0<depth.second)
		{	// reality-check: balanced parentheses
		autovalarray_ptr_throws<size_t> fixedstack(depth.first);
		autovalarray_ptr_throws<POD_pair<size_t,size_t> > pair_fixedstack(depth.first<depth.second ? depth.first : depth.second);

		depth.first = 0;
		depth.second = 0;
		size_t balanced_paren = 0;
		size_t i = 0;
		do	if (1==tokenlist[i].token.second)
				{
				assert(tokenlist[i].token.first);
				if 		(l_match==tokenlist[i].token.first[0])
					{	// soft-left: not an error
					if (0<depth.second)
						{
						unbalanced_error(tokenlist[i],depth.second,r_match);
						depth.second = 0;
						}
					if (0==depth.first) unbalanced_loc.first = i;
					fixedstack[depth.first++] = i;
					}
				else if (r_match==tokenlist[i].token.first[0])
					{
					if (0<depth.first)
						{
						pair_fixedstack[balanced_paren].first = fixedstack[--depth.first];
						pair_fixedstack[balanced_paren++].second = i;
						}
					else{
						++depth.second;
						unbalanced_loc.second = i;
						}
					};
				}
		while(tokenlist_len > ++i);
		if (0==depth.first && 0==depth.second && starting_errors==zcc_errors.err_count())	// need to be more flexible here as well
			{
			assert(pair_fixedstack.size()==balanced_paren);	// only for hard-left hard-right
			pair_fixedstack.MoveInto(stack1);
			}
		};

	assert(0==depth.first || 0==depth.second);
	if (0<depth.second)
		// soft-left: not an error
		unbalanced_error(tokenlist[unbalanced_loc.second],depth.second,r_match);
	if (0<depth.first)
		// soft-right: not an error
		unbalanced_error(tokenlist[unbalanced_loc.first],depth.first,l_match);
}

template<char l_match,char r_match>
static void construct_matched_pairs(const weak_token* tokenlist,size_t tokenlist_len, autovalarray_ptr<POD_pair<size_t,size_t> >& stack1)
{
	assert(tokenlist);
	assert(0<tokenlist_len);
	_construct_matched_pairs(tokenlist,tokenlist_len,stack1,l_match,r_match);
}

template<>
void construct_matched_pairs<'[',']'>(const weak_token* tokenlist,size_t tokenlist_len, autovalarray_ptr<POD_pair<size_t,size_t> >& stack1)
{
	assert(tokenlist);
	assert(0<tokenlist_len);
	POD_pair<size_t,size_t> depth = balanced_character_count<'[',']'>(tokenlist,tokenlist_len);	// pre-scan
	std::pair<size_t,size_t> unbalanced_loc(0,0);
	const size_t starting_errors = zcc_errors.err_count();

	if (0<depth.first && 0<depth.second)
		{	// reality-check: balanced parentheses
		autovalarray_ptr_throws<size_t> fixedstack(depth.first);
		autovalarray_ptr_throws<POD_pair<size_t,size_t> > pair_fixedstack(depth.first<depth.second ? depth.first : depth.second);

		depth.first = 0;
		depth.second = 0;
		size_t balanced_paren = 0;
		size_t i = 0;
		do	if (1==tokenlist[i].token.second)
				{
				assert(tokenlist[i].token.first);
				if 		(detect_C_left_bracket_op(tokenlist[i].token.first,tokenlist[i].token.second))
					{
					if (0<depth.second)
						{
						unbalanced_error(tokenlist[i],depth.second,']');
						depth.second = 0;
						}
					if (0==depth.first) unbalanced_loc.first = i;
					fixedstack[depth.first++] = i;
					}
				else if (detect_C_right_bracket_op(tokenlist[i].token.first,tokenlist[i].token.second))
					{
					if (0<depth.first)
						{
						pair_fixedstack[balanced_paren].first = fixedstack[--depth.first];
						pair_fixedstack[balanced_paren++].second = i;
						}
					else{
						++depth.second;
						unbalanced_loc.second = i;
						}
					};
				}
		while(tokenlist_len > ++i);
		if (0==depth.first && 0==depth.second && starting_errors==zcc_errors.err_count())
			{
			assert(pair_fixedstack.size()==balanced_paren);
			pair_fixedstack.MoveInto(stack1);
			}
		};

	assert(0==depth.first || 0==depth.second);
	if (0<depth.second)
		unbalanced_error(tokenlist[unbalanced_loc.second],depth.second,']');
	if (0<depth.first)
		unbalanced_error(tokenlist[unbalanced_loc.first],depth.first,'[');
}

template<>
void construct_matched_pairs<'{','}'>(const weak_token* tokenlist,size_t tokenlist_len, autovalarray_ptr<POD_pair<size_t,size_t> >& stack1)
{
	assert(tokenlist);
	assert(0<tokenlist_len);
	POD_pair<size_t,size_t> depth = balanced_character_count<'{','}'>(tokenlist,tokenlist_len);	// pre-scan
	std::pair<size_t,size_t> unbalanced_loc(0,0);
	const size_t starting_errors = zcc_errors.err_count();

	if (0<depth.first && 0<depth.second)
		{	// reality-check: balanced parentheses
		autovalarray_ptr_throws<size_t> fixedstack(depth.first);
		autovalarray_ptr_throws<POD_pair<size_t,size_t> > pair_fixedstack(depth.first<depth.second ? depth.first : depth.second);

		depth.first = 0;
		depth.second = 0;
		size_t balanced_paren = 0;
		size_t i = 0;
		do	if (1==tokenlist[i].token.second)
				{
				assert(tokenlist[i].token.first);
				if 		(detect_C_left_brace_op(tokenlist[i].token.first,tokenlist[i].token.second))
					{
					if (0<depth.second)
						{
						unbalanced_error(tokenlist[i],depth.second,'}');
						depth.second = 0;
						}
					if (0==depth.first) unbalanced_loc.first = i;
					fixedstack[depth.first++] = i;
					}
				else if (detect_C_right_brace_op(tokenlist[i].token.first,tokenlist[i].token.second))
					{
					if (0<depth.first)
						{
						pair_fixedstack[balanced_paren].first = fixedstack[--depth.first];
						pair_fixedstack[balanced_paren++].second = i;
						}
					else{
						++depth.second;
						unbalanced_loc.second = i;
						}
					};
				}
		while(tokenlist_len > ++i);
		if (0==depth.first && 0==depth.second && starting_errors==zcc_errors.err_count())
			{
			assert(pair_fixedstack.size()==balanced_paren);
			pair_fixedstack.MoveInto(stack1);
			}
		};

	assert(0==depth.first || 0==depth.second);
	if (0<depth.second)
		unbalanced_error(tokenlist[unbalanced_loc.second],depth.second,'}');
	if (0<depth.first)
		unbalanced_error(tokenlist[unbalanced_loc.first],depth.first,'{');
}


static void
_slice_error(const weak_token& src,size_t slice_count,const char cut,const std::pair<char,char>& knife)
{
	message_header(src);
	INC_INFORM(ERR_STR);
	INC_INFORM(slice_count);
	INC_INFORM(' ');
	INC_INFORM(cut);
	INC_INFORM(" are unbalanced by improper grouping within ");
	INC_INFORM(knife.first);
	INC_INFORM(' ');
	INFORM(knife.second);
	zcc_errors.inc_error();
}

static void
find_sliced_pairs(const weak_token* tokenlist, const autovalarray_ptr<POD_pair<size_t,size_t> >& stack1, const autovalarray_ptr<POD_pair<size_t,size_t> >& stack2,const std::pair<char,char>& pair1,const std::pair<char,char>& pair2)
{
	assert(tokenlist);
	if (stack1.empty()) return;
	if (stack2.empty()) return;
	size_t i = 0;
	size_t j = 0;
	do	{
		if (stack1[i].second<stack2[j].first)
			{	// ok (disjoint)
			++i;
			continue;
			};
		if (stack2[j].second<stack1[i].first)
			{	// ok (disjoint)
			++j;
			continue;
			};
		if (stack1[i].first<stack2[j].first)
			{
			if (stack2[j].second<stack1[i].second)
				{	// ok (proper nesting)
				++i;
				continue;
				}
			size_t slice_count = 1;
			while(++i < stack1.size() && stack1[i].first<stack2[j].first && stack2[j].second>=stack1[i].second) ++slice_count;
			_slice_error(tokenlist[stack1[i-1].first],slice_count,pair1.second,pair2);
			continue;
			}
		if (stack2[j].first<stack1[i].first)
			{
			if (stack1[i].second<stack2[j].second)
				{	// ok (proper nesting)
				++j;
				continue;
				}
			size_t slice_count = 1;
			while(++j < stack2.size() && stack2[j].first<stack1[i].first && stack1[i].second>=stack2[j].second) ++slice_count;
			_slice_error(tokenlist[stack2[j-1].first],slice_count,pair2.second,pair1);
			continue;
			}
		}
	while(i<stack1.size() && j<stack2.size());
}

static bool C_like_BalancingCheck(const weak_token* tokenlist,size_t tokenlist_len,bool hard_start,bool hard_end)
{
	assert(tokenlist);
	assert(0<tokenlist_len);
	autovalarray_ptr<POD_pair<size_t,size_t> > parenpair_stack;
	autovalarray_ptr<POD_pair<size_t,size_t> > bracketpair_stack;
	autovalarray_ptr<POD_pair<size_t,size_t> > bracepair_stack;
	const size_t starting_errors = zcc_errors.err_count();

	// responsible for context-free error checking	
	if (hard_start && hard_end)
		{	// other test cases cannot be done from preprocessor
		construct_matched_pairs<'(',')'>(tokenlist,tokenlist_len,parenpair_stack);
		construct_matched_pairs<'[',']'>(tokenlist,tokenlist_len,bracketpair_stack);
		construct_matched_pairs<'{','}'>(tokenlist,tokenlist_len,bracepair_stack);
		if (starting_errors==zcc_errors.err_count())
			{	// check for slicing
			const int test_these = (!parenpair_stack.empty())+2*(!bracketpair_stack.empty())+4*(!bracepair_stack.empty());
			switch(test_these)
			{
			default:	break;
			case 7:		{	// all three
						find_sliced_pairs(tokenlist,bracketpair_stack,bracepair_stack,std::pair<char,char>('[',']'),std::pair<char,char>('{','}'));
						find_sliced_pairs(tokenlist,parenpair_stack,bracepair_stack,std::pair<char,char>('(',')'),std::pair<char,char>('{','}'));
						find_sliced_pairs(tokenlist,parenpair_stack,bracketpair_stack,std::pair<char,char>('(',')'),std::pair<char,char>('[',']'));
						break;
						}
			case 6:		{	// bracket and brace
						find_sliced_pairs(tokenlist,bracketpair_stack,bracepair_stack,std::pair<char,char>('[',']'),std::pair<char,char>('{','}'));
						break;
						}
			case 5:		{	// paren and brace
						find_sliced_pairs(tokenlist,parenpair_stack,bracepair_stack,std::pair<char,char>('(',')'),std::pair<char,char>('{','}'));
						break;
						}
			case 3:		{	// paren and bracket
							//! \test cpp/default/Error_if_control70.hpp, cpp/default/Error_if_control70.h
							//! \test cpp/default/Error_if_control71.hpp, cpp/default/Error_if_control71.h
						find_sliced_pairs(tokenlist,parenpair_stack,bracketpair_stack,std::pair<char,char>('(',')'),std::pair<char,char>('[',']'));
						break;
						}
			}
			}
		};
	return starting_errors!=zcc_errors.err_count();
}

template<size_t targ_len>
static inline bool
robust_token_is_string(const POD_pair<const char*,size_t>& x,const char* const target)
{
	assert(target);
	assert(targ_len==strlen(target));
	return x.first && targ_len==x.second && !strncmp(x.first,target,targ_len);
}

static inline bool
robust_token_is_string(const POD_pair<const char*,size_t>& x,const char* const target)
{
	assert(target);
	const size_t targ_len = strlen(target);
	return x.first && targ_len==x.second && !strncmp(x.first,target,targ_len);
}

template<size_t targ_len>
static inline bool
token_is_string(const POD_pair<const char*,size_t>& x,const char* const target)
{
	assert(target);
	assert(targ_len==strlen(target));
	assert(x.first);
	return targ_len==x.second && !strncmp(x.first,target,targ_len);
}

static inline bool
token_is_string(const POD_pair<const char*,size_t>& x,const char* const target)
{
	assert(target);
	assert(x.first);
	const size_t targ_len = strlen(target);
	return targ_len==x.second && !strncmp(x.first,target,targ_len);
}

template<size_t targ_len>
static inline bool
robust_token_is_string(const parse_tree& x,const char* const target)
{
	return x.is_atomic() && token_is_string<targ_len>(x.index_tokens[0].token,target);
}

static inline bool
robust_token_is_string(const parse_tree& x,const char* const target)
{
	return x.is_atomic() && token_is_string(x.index_tokens[0].token,target);
}

template<char c>
static inline bool
token_is_char(const POD_pair<const char*,size_t>& x)
{
	assert(x.first);
	return 1==x.second && c== *x.first;
}

template<>
inline bool token_is_char<'#'>(const POD_pair<const char*,size_t>& x)
{
	assert(x.first);
	return detect_C_stringize_op(x.first,x.second);
}

template<>
inline bool token_is_char<'['>(const POD_pair<const char*,size_t>& x)
{
	assert(x.first);
	return detect_C_left_bracket_op(x.first,x.second);
}

template<>
inline bool token_is_char<']'>(const POD_pair<const char*,size_t>& x)
{
	assert(x.first);
	return detect_C_right_bracket_op(x.first,x.second);
}

template<>
inline bool token_is_char<'{'>(const POD_pair<const char*,size_t>& x)
{
	assert(x.first);
	return detect_C_left_brace_op(x.first,x.second);
}

template<>
inline bool token_is_char<'}'>(const POD_pair<const char*,size_t>& x)
{
	assert(x.first);
	return detect_C_right_brace_op(x.first,x.second);
}

template<char c>
inline bool robust_token_is_char(const POD_pair<const char*,size_t>& x)
{
	return x.first && 1==x.second && c== *x.first;
}

template<>
inline bool robust_token_is_char<'#'>(const POD_pair<const char*,size_t>& x)
{
	return x.first && detect_C_stringize_op(x.first,x.second);
}

template<>
inline bool robust_token_is_char<'['>(const POD_pair<const char*,size_t>& x)
{
	return x.first && detect_C_left_bracket_op(x.first,x.second);
}

template<>
inline bool robust_token_is_char<']'>(const POD_pair<const char*,size_t>& x)
{
	return x.first && detect_C_right_bracket_op(x.first,x.second);
}

template<>
inline bool robust_token_is_char<'{'>(const POD_pair<const char*,size_t>& x)
{
	return x.first && detect_C_left_brace_op(x.first,x.second);
}

template<>
inline bool robust_token_is_char<'}'>(const POD_pair<const char*,size_t>& x)
{
	return x.first && detect_C_right_brace_op(x.first,x.second);
}

template<char c> inline bool robust_token_is_char(const parse_tree& x)
{
	return x.is_atomic() && token_is_char<c>(x.index_tokens[0].token);
}

//! \todo if we have an asphyxiates_left_brace, suppress_naked_brackets_and_braces goes obsolete
static bool asphyxiates_left_bracket(const weak_token& x)
{
	if ((C_TESTFLAG_IDENTIFIER | C_TESTFLAG_CHAR_LITERAL | C_TESTFLAG_STRING_LITERAL | C_TESTFLAG_PP_NUMERAL) & x.flags) return false;
	if (token_is_char<')'>(x.token)) return false;
	if (token_is_char<']'>(x.token)) return false;
	return true;
}

//! \todo this forks when distinctions between C, C++ are supported
// balancing errors are handled earlier
static bool right_paren_asphyxiates(const weak_token& token)
{
	if (   token_is_char<'&'>(token.token)	// potentially unary operators
		|| token_is_char<'*'>(token.token)
		|| token_is_char<'+'>(token.token)
		|| token_is_char<'-'>(token.token)
		|| token_is_char<'~'>(token.token)
		|| token_is_char<'!'>(token.token)
	    || token_is_char<'/'>(token.token)	// proper multiplicative operators
	    || token_is_char<'%'>(token.token)
//		|| token_is_string<2>(token.token,"<<")	// shift operators -- watch out for parsing problems with templates, this really is C only
//		|| token_is_string<2>(token.token,">>")
//		|| token_is_char<'<'>(token.token)	// relational operators -- watch out for parsing problems with templates
//	    || token_is_char<'>'>(token.token)
		|| token_is_string<2>(token.token,"<=")
		|| token_is_string<2>(token.token,">=")
		|| token_is_string<2>(token.token,"==")	// equality operators
		|| token_is_string<2>(token.token,"!=")
	    || token_is_char<'^'>(token.token)		// bitwise XOR
		|| token_is_char<'|'>(token.token)		// bitwise OR
		|| token_is_string<2>(token.token,"&&")	// logical AND
		|| token_is_string<2>(token.token,"||")	// logical OR
		|| token_is_char<'?'>(token.token))	// operator ? :
		return true;
	return false;
}

//! \todo this forks when distinctions between C, C++ are supported
static bool left_paren_asphyxiates(const weak_token& token)
{
	if (	token_is_char<'/'>(token.token)	// proper multiplicative operators
	    ||	token_is_char<'%'>(token.token)
//		|| token_is_string<2>(token.token,"<<")	// shift operators -- watch out for parsing problems with templates, this really is C only
//		|| token_is_string<2>(token.token,">>")
//		|| token_is_char<'<'>(token.token)	// relational operators -- watch out for parsing problems with templates
//	    || token_is_char<'>'>(token.token)
		|| token_is_string<2>(token.token,"<=")
		|| token_is_string<2>(token.token,">=")
		|| token_is_string<2>(token.token,"==")	// equality operators
		|| token_is_string<2>(token.token,"!=")
	    || token_is_char<'^'>(token.token)		// bitwise XOR
		|| token_is_char<'|'>(token.token)		// bitwise OR
		|| token_is_string<2>(token.token,"&&")	// logical AND
		|| token_is_string<2>(token.token,"||")	// logical OR
		||	token_is_char<'?'>(token.token)		// operator ? : 
		||	token_is_char<':'>(token.token))	// one of operator ? :, or a label
		return true;
	return false;
}

static bool paren_is_bad_news(const weak_token& lhs, const weak_token& rhs)
{
	if (token_is_char<'['>(rhs.token) && asphyxiates_left_bracket(lhs))
		{
		message_header(rhs);
		INC_INFORM(ERR_STR);
		INC_INFORM(lhs);
		INFORM(" denies [ ] its left argument (C99 6.5.2p1/C++98 5.2p1)");
		zcc_errors.inc_error();
		};
	if (token_is_char<')'>(rhs.token) || token_is_char<']'>(rhs.token))
		{
		if (right_paren_asphyxiates(lhs))
			{
			message_header(rhs);
			INC_INFORM(ERR_STR);
			INC_INFORM(rhs);
			INC_INFORM(" denies ");
			INC_INFORM(lhs);
			INFORM(" its right argument (C99 6.5.3p1/C++98 5.3p1)");
			zcc_errors.inc_error();
			}
		}
	if (token_is_char<'('>(lhs.token) || token_is_char<'['>(lhs.token))
		{
		if (left_paren_asphyxiates(rhs))
			{
			message_header(lhs);
			INC_INFORM(ERR_STR);
			INC_INFORM(lhs);
			INC_INFORM(" denies ");
			INC_INFORM(rhs);
			INFORM(" its left argument");
			zcc_errors.inc_error();
			}
		}
	return false;	//! \todo don't abuse std::adjacent_find
}

static bool C99_CoreControlExpressionContextFreeErrorCount(const weak_token* tokenlist,size_t tokenlist_len,bool hard_start,bool hard_end)
{
	assert(tokenlist);
	assert(0<tokenlist_len);
	const size_t starting_errors = zcc_errors.err_count();
	bool already_errored = false;

	if (hard_start && token_is_char<'['>(tokenlist[0].token))
		{	//! \test if.C99/Error_control3.h, if.C99/Error_control3.hpp
		message_header(tokenlist[0]);
		INC_INFORM(ERR_STR);
		INFORM("[ at start of expression denies [ ] its left argument (C99 6.5.2p1/C++98 5.2p1)");
		zcc_errors.inc_error();
		};
	if (hard_start && left_paren_asphyxiates(tokenlist[0]))
		{	//! \test if.C99/Error_control4.h, if.C99/Error_control4.hpp
			//! \test if.C99/Error_control11.h, if.C99/Error_control11.hpp
			//! \test if.C99/Error_control12.h, if.C99/Error_control12.hpp
			//! \test if.C99/Error_control13.h, if.C99/Error_control13.hpp
			//! \test if.C99/Error_control14.h, if.C99/Error_control14.hpp
			//! \test if.C99/Error_control15.h, if.C99/Error_control15.hpp
			//! \test if.C99/Error_control16.h, if.C99/Error_control16.hpp
			//! \test if.C99/Error_control17.h, if.C99/Error_control17.hpp
			//! \test if.C99/Error_control18.h, if.C99/Error_control18.hpp
			//! \test if.C99/Error_control19.h, if.C99/Error_control19.hpp
			//! \test if.C99/Error_control20.h, if.C99/Error_control20.hpp
			//! \test if.C99/Error_control21.h, if.C99/Error_control21.hpp
		message_header(tokenlist[0]);
		INC_INFORM(ERR_STR);
		INC_INFORM(tokenlist[0]);
		INFORM((1==tokenlist_len && hard_end && right_paren_asphyxiates(tokenlist[0])) ? " as only token doesn't have either of its arguments (C99 6.5.3p1/C++98 5.3p1)"
				: " as first token doesn't have its left argument (C99 6.5.3p1/C++98 5.3p1)");
		zcc_errors.inc_error();
		already_errored = 1==tokenlist_len;
		};
	std::adjacent_find(tokenlist,tokenlist+tokenlist_len,paren_is_bad_news);
	if (hard_end && !already_errored && right_paren_asphyxiates(tokenlist[tokenlist_len-1]))
		{	//! \test if.C99/Error_control5.h, if.C99/Error_control5.hpp
			//! \test if.C99/Error_control6.h, if.C99/Error_control6.hpp
			//! \test if.C99/Error_control7.h, if.C99/Error_control7.hpp
			//! \test if.C99/Error_control8.h, if.C99/Error_control8.hpp
			//! \test if.C99/Error_control9.h, if.C99/Error_control9.hpp
			//! \test if.C99/Error_control10.h, if.C99/Error_control10.hpp
		message_header(tokenlist[tokenlist_len-1]);
		INC_INFORM(ERR_STR);
		INC_INFORM(tokenlist[tokenlist_len-1]);
		INFORM(" as last token doesn't have its right argument (C99 6.5.3p1/C++98 5.3p1)");
		zcc_errors.inc_error();
		}

	return starting_errors!=zcc_errors.err_count();
}

static bool C99_ControlExpressionContextFreeErrorCount(const weak_token* tokenlist,size_t tokenlist_len,bool hard_start,bool hard_end)
{
	assert(tokenlist);
	assert(0<tokenlist_len);
	return C99_CoreControlExpressionContextFreeErrorCount(tokenlist,tokenlist_len,hard_start,hard_end);
}

static bool CPP_ControlExpressionContextFreeErrorCount(const weak_token* tokenlist,size_t tokenlist_len,bool hard_start,bool hard_end)
{
	assert(tokenlist);
	assert(0<tokenlist_len);
	return C99_CoreControlExpressionContextFreeErrorCount(tokenlist,tokenlist_len,hard_start,hard_end);
}

size_t LengthOfCPurePreprocessingOperatorPunctuation(const char* const x)
{
	assert(x && *x);
	if (strchr(ATOMIC_PREPROC_PUNC,*x)) return 1;
	const errr i = linear_reverse_find_prefix_in_lencached(x,valid_pure_preprocessing_op_punc+NONATOMIC_PREPROC_OP_LB,C_PREPROC_OP_STRICT_UB-NONATOMIC_PREPROC_OP_LB);
	if (0<=i) return valid_pure_preprocessing_op_punc[i+NONATOMIC_PREPROC_OP_LB].second;
	return 0;
}

size_t LengthOfCPPPurePreprocessingOperatorPunctuation(const char* const x)
{
	assert(x && *x);
	if (strchr(ATOMIC_PREPROC_PUNC,*x)) return 1;
	const errr i = linear_reverse_find_prefix_in_lencached(x,valid_pure_preprocessing_op_punc+NONATOMIC_PREPROC_OP_LB,CPP_PREPROC_OP_STRICT_UB-NONATOMIC_PREPROC_OP_LB);
	if (0<=i) return valid_pure_preprocessing_op_punc[i+NONATOMIC_PREPROC_OP_LB].second;
	return 0;
}

static unsigned int CPurePreprocessingOperatorPunctuationFlags(signed int i)
{
	assert(0<i && C_PREPROC_OP_STRICT_UB>=(unsigned int)i);
	return valid_pure_preprocessing_op_punc[i-1].third;
}

static unsigned int CPPPurePreprocessingOperatorPunctuationFlags(signed int i)
{
	assert(0<i && CPP_PREPROC_OP_STRICT_UB>=(unsigned int)i);
	return valid_pure_preprocessing_op_punc[i-1].third;
}

// encoding reality checks
BOOST_STATIC_ASSERT(PP_CODE_MASK>CPP_PREPROC_OP_STRICT_UB);
BOOST_STATIC_ASSERT((PP_CODE_MASK>>1)<=CPP_PREPROC_OP_STRICT_UB);
static signed int
CPurePreprocessingOperatorPunctuationCode(const char* const x, size_t x_len)
{
	BOOST_STATIC_ASSERT(INT_MAX-1>=C_PREPROC_OP_STRICT_UB);
	return 1+linear_reverse_find_lencached(x,x_len,valid_pure_preprocessing_op_punc,C_PREPROC_OP_STRICT_UB);
}

static signed int
CPPPurePreprocessingOperatorPunctuationCode(const char* const x, size_t x_len)
{
	BOOST_STATIC_ASSERT(INT_MAX-1>=CPP_PREPROC_OP_STRICT_UB);
	return 1+linear_reverse_find_lencached(x,x_len,valid_pure_preprocessing_op_punc,CPP_PREPROC_OP_STRICT_UB);
}

static void _bad_syntax_tokenized(const char* const x, size_t x_len, lex_flags& flags, const char* const src_filename, size_t line_no, func_traits<signed int (*)(const char* const, size_t)>::function_type find_pp_code)
{
	assert(x);
	assert(src_filename && *src_filename);
	assert(x_len<=strlen(x));
	assert((C_TESTFLAG_PP_NUMERAL | C_TESTFLAG_PP_OP_PUNC | C_TESTFLAG_STRING_LITERAL | C_TESTFLAG_CHAR_LITERAL | C_TESTFLAG_IDENTIFIER) & flags);

	// reality checks on relation between flag constants and enums
	BOOST_STATIC_ASSERT((C_PPFloatCore::F<<10)==C_TESTFLAG_F);
	BOOST_STATIC_ASSERT((C_PPFloatCore::L<<10)==C_TESTFLAG_L);

	BOOST_STATIC_ASSERT((C_PPIntCore::U<<10)==C_TESTFLAG_U);
	BOOST_STATIC_ASSERT((C_PPIntCore::L<<10)==C_TESTFLAG_L);
	BOOST_STATIC_ASSERT((C_PPIntCore::UL<<10)==(C_TESTFLAG_L | C_TESTFLAG_U));
	BOOST_STATIC_ASSERT((C_PPIntCore::LL<<10)==C_TESTFLAG_LL);
	BOOST_STATIC_ASSERT((C_PPIntCore::ULL<<10)==(C_TESTFLAG_LL | C_TESTFLAG_U));

	if (C_TESTFLAG_PP_NUMERAL==flags)
		{
		union_quartet<C_PPIntCore,C_PPFloatCore,C_PPDecimalFloat,C_PPHexFloat> test;
		if 		(C_PPDecimalFloat::is(x,x_len,test.third))
			flags |= C_TESTFLAG_FLOAT | C_TESTFLAG_DECIMAL;
		else if	(C_PPHexFloat::is(x,x_len,test.fourth))
			flags |= C_TESTFLAG_FLOAT | C_TESTFLAG_HEXADECIMAL;
		else if (C_PPIntCore::is(x,x_len,test.first))
			{
			assert(C_PPIntCore::ULL>=test.first.hinted_type);
			flags |= (((lex_flags)(test.first.hinted_type))<<10);
			assert(8==test.first.radix || 10==test.first.radix || 16==test.first.radix);
			switch(test.first.radix)
			{
			case 8:		{
						flags |= C_TESTFLAG_INTEGER | C_TESTFLAG_OCTAL;
						break;
						}
			case 10:	{
						flags |= C_TESTFLAG_INTEGER | C_TESTFLAG_DECIMAL;
						break;
						}
			case 16:	{
						flags |= C_TESTFLAG_INTEGER | C_TESTFLAG_HEXADECIMAL;
						break;
						}
			};
			}
		if 		(flags & C_TESTFLAG_FLOAT)
			{
			assert(C_PPFloatCore::L>=test.second.hinted_type);
			flags |= (((lex_flags)(test.second.hinted_type))<<10);
			};
		if (C_TESTFLAG_PP_NUMERAL==flags)
			{
			INC_INFORM(src_filename);
			INC_INFORM(':');
			INC_INFORM(line_no);
			INC_INFORM(": ");
			INC_INFORM(ERR_STR);
			INC_INFORM("invalid preprocessing number");
			INC_INFORM(x,x_len);
			INFORM(" (C99 6.4.4.1p1,6.4.4.2p1/C++98 2.13.1,2.13.3)");
			zcc_errors.inc_error();
			return;
			}
		}
	else if (C_TESTFLAG_PP_OP_PUNC & flags)
		{	// language-sensitive token blacklisting
		const signed int pp_code = find_pp_code(x,x_len);
		assert(0<pp_code);
		C_PP_ENCODE(flags,pp_code);
		}
	else if (C_TESTFLAG_STRING_LITERAL==flags)
		{	// This gets in by C99 6.6p10, as 6.6p6 doesn't list string literals as legitimate
		if (!IsLegalCString(x,x_len))
			{
			INC_INFORM(src_filename);
			INC_INFORM(':');
			INC_INFORM(line_no);
			INC_INFORM(": ");
			INC_INFORM(ERR_STR);
			INC_INFORM(x,x_len);
			INFORM(" : invalid string (C99 6.4.5p1/C++98 2.13.4)");
			zcc_errors.inc_error();
			return;
			}
		else if (bool_options[boolopt::pedantic])
			{
			INC_INFORM(src_filename);
			INC_INFORM(':');
			INC_INFORM(line_no);
			INC_INFORM(": ");
			INC_INFORM(WARN_STR);
			INC_INFORM(x,x_len);
			INFORM(" : string literals in integer constant expressions are only permitted, not required (C99 6.6p10)");
			if (bool_options[boolopt::warnings_are_errors])
				{
				zcc_errors.inc_error();
				return;
				}
			}
		}
	else if (C_TESTFLAG_CHAR_LITERAL==flags)
		{
		if (!IsLegalCCharacterLiteral(x,x_len))
			{
			INC_INFORM(src_filename);
			INC_INFORM(':');
			INC_INFORM(line_no);
			INC_INFORM(": ");
			INC_INFORM(ERR_STR);
			INC_INFORM("invalid character literal ");
			INC_INFORM(x,x_len);
			INFORM(" (C99 6.4.4.4p1/C++98 2.13.2)");
			zcc_errors.inc_error();
			return;
			}
		}
}

static void C99_bad_syntax_tokenized(const char* const x, size_t x_len, lex_flags& flags, const char* const src_filename, size_t line_no)
{
	_bad_syntax_tokenized(x,x_len,flags,src_filename,line_no,CPurePreprocessingOperatorPunctuationCode);
}

static void CPP_bad_syntax_tokenized(const char* const x, size_t x_len, lex_flags& flags, const char* const src_filename, size_t line_no)
{
	_bad_syntax_tokenized(x,x_len,flags,src_filename,line_no,CPPPurePreprocessingOperatorPunctuationCode);
}

//! \todo fix these to not assume perfect matching character sets
#define C99_SYMBOLIC_ESCAPED_ESCAPES "\a\b\f\n\r\t\v"
#define C99_SYMBOLIC_ESCAPES "abfnrtv"
#define C99_COPY_ESCAPES "\"'?\\"
static const char* const c99_symbolic_escaped_escapes = C99_SYMBOLIC_ESCAPED_ESCAPES;
static const char* const c99_symbolic_escapes = C99_SYMBOLIC_ESCAPES;

static size_t LengthOfEscapedCString(const char* src, size_t src_len)
{
	assert(src);
	assert(0<src_len);
	size_t actual_size = src_len;
	size_t i = 0;
	do	{
		if ('\0'!=src[i])
			{	// deal with the symbolic escapes
			if (   strchr(c99_symbolic_escaped_escapes,src[i])
				|| strchr("\"\\?",src[i]))
				{
				++actual_size;
				continue;
				};
			if (!C_IsPrintableChar(src[i]))
				{
				// note that octal escaping can only go up to 511, which is a problem if our CHAR_BIT exceeds 9 
				unsigned char tmp = src[i];
				bool must_escape = (1<src_len-i && strchr(list_hexadecimal_digits,src[i+1]));
				//! \todo fix to handle target CHAR_BIT different than ours
#if 9>=CHAR_BIT
				if (7U>=tmp || must_escape)
#else
				if (7U>=tmp || (must_escape && 511U>=tmp))
#endif
					{	// octal-escape if it's readable, or if it prevents escaping the next printable character
					if (must_escape)
						{
						actual_size += 3;
						continue;
						}
					do	{
						++actual_size;
						tmp /= 8U;
						}
					while(0<tmp);
					continue;
					}
#if 9<CHAR_BIT
#	if 16>=CHAR_BIT
				if (must_escape)
					{	// hexadecimal breaks; try universal-char-encoding
					actual_size += 5;
					continue;
					}
#	elif 32>=CHAR_BIT
				if (must_escape)
					{	// hexadecimal breaks; try universal-char-encoding
					actual_size += 5;
					if (65536<=tmp) actual_size += 4;
					continue;
					}
#	else
				if (must_escape && 65536*65536>tmp)
					{	// hexadecimal breaks; try universal-char-encoding
					actual_size += 5;
					if (65536<=tmp) actual_size += 4;
					continue;
					}
#	endif
#endif
				// hex-escape for legibility
				while(16<=tmp)
					{
					++actual_size;
					tmp /= 16;
					};
				actual_size += 2;
				continue;
				}
			};
		}
	while(src_len > ++i);
	return actual_size;
}

static size_t LengthOfEscapedCString(const my_UNICODE* src, size_t src_len)
{	//! \todo synchronize with EscapeCString
	assert(src);
	assert(0<src_len);
	size_t actual_size = src_len;
	size_t i = 0;
	do	{
		if ('\0'!=src[i])
			{	// deal with the symbolic escapes
			if (   strchr(c99_symbolic_escaped_escapes,src[i])
				|| strchr("\"\\?",src[i]))
				{
				++actual_size;
				continue;
				};
			if (!C_IsPrintableChar(src[i]))
				{	// note that octal escaping can only go up to 511, which is a problem if our CHAR_BIT exceeds 9 
				my_UNICODE tmp = src[i];
				bool must_escape = (1<src_len-i && strchr(list_hexadecimal_digits,src[i+1]));
				//! \todo fix to handle target CHAR_BIT different than ours
#if 9>=CHAR_BIT*C_UNICODE_SIZE
				if (7U>=tmp || must_escape)
#else
				if (7U>=tmp || (must_escape && 511U>=tmp))
#endif
					{	// octal-escape if it's readable, or if it prevents escaping the next printable character
					if (must_escape)
						{
						actual_size += 3;
						continue;
						}
					do	{
						++actual_size;
						tmp /= 8;
						}
					while(0<tmp);
					continue;
					}
#if 9<CHAR_BIT*C_UNICODE_SIZE
#	if 16>=CHAR_BIT*C_UNICODE_SIZE
				if (must_escape)
					{	// hexadecimal breaks; try universal-char-encoding
					actual_size += 5;
					continue;
					}
#	elif 32>=CHAR_BIT*C_UNICODE_SIZE
				if (must_escape)
					{	// hexadecimal breaks; try universal-char-encoding
					actual_size += 5;
					if (65536U<=tmp) actual_size += 4;
					continue;
					}
#	else
				if (must_escape && 65536ULL*65536ULL>tmp)
					{	// hexadecimal breaks; try universal-char-encoding
					actual_size += 5;
					if (65536U<=tmp) actual_size += 4;
					continue;
					}
#	endif
#endif
				// hex-escape for legibility
				while(16<=tmp)
					{
					++actual_size;
					tmp /= 16;
					};
				actual_size += 2;
				continue;
				}
			};
		}
	while(src_len > ++i);
	return actual_size;
}

static void EscapeCString(char* dest, const char* src, size_t src_len)
{	// \todo fix ASCII dependency.
	assert(src);
	assert(0<src_len);
	size_t i = 0;
	do	{
		if ('\0'!=src[i])
			{	// deal with the symbolic escapes
			const char* const symbolic_escaped_escape = strchr(c99_symbolic_escaped_escapes,src[i]);
			if (symbolic_escaped_escape)
				{
				*(dest++) = '\\';
				*(dest++) = c99_symbolic_escapes[symbolic_escaped_escape-c99_symbolic_escaped_escapes];
				continue;
				};
			//! \todo probably too cautious; we need to escape ? only if a ? immediately precedes it (break trigraph sequences)
			if (strchr("\"\\?",src[i]))
				{
				*(dest++) = '\\';
				*(dest++) = src[i];
				continue;
				}
			};
		// Note that hex escape sequence requires at least 2 bytes, so octal is just-as-cheap through decimal 63 (which takes 3 bytes in hex)
		// however, octal isn't that user friendly; we clearly want to octal-escape only through 7
		// \bug need test cases
		if (!C_IsPrintableChar(src[i]))
			{	// note that octal escaping can only go up to 511, which is a problem if our CHAR_BIT exceeds 9 
			unsigned char tmp = src[i];
			bool must_escape = (1<src_len-i && strchr(list_hexadecimal_digits,src[i+1]));
			*(dest++) = '\\';
			//! \todo fix to handle target CHAR_BIT different than ours
#if 9>=CHAR_BIT
			if (7U>=tmp || must_escape)
#else
			if (7U>=tmp || (must_escape && 511U>=tmp))
#endif
				{	// octal-escape if it's readable, or if it prevents escaping the next printable character
				if (must_escape && 64U>tmp)
					{
					*(dest++) = '0';
					if (8U>tmp) *(dest++) = '0';
					};
				do	{
					*(dest++) = (tmp%8U)+(unsigned char)('0');
					tmp /= 8U;
					}
				while(0<tmp);
				continue;
				}
#if 9<CHAR_BIT
#	if 16>=CHAR_BIT
			if (must_escape)
				{
				*(dest++) = 'u';
#		if 12<CHAR_BIT
				*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/4096U)%16];
#		else
				*(dest++) = '0';
#		endif
				*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/256U)%16];
				*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/16U)%16];
				*(dest++) = C_HEXADECIMAL_DIGITS[tmp%16U];
				continue;
				}
#	elif 32>=CHAR_BIT
			if (must_escape)
				{
				if (65536U<=tmp)
					{
					*(dest++) = 'U';
#		if 28<CHAR_BIT
					*(dest++) = C_HEXADECIMAL_DIGITS[tmp/(65536ULL*4096ULL)];
#		else
					*(dest++) = '0';
#		endif
#		if 24<CHAR_BIT
					*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/(65536ULL*256ULL))%16];
#		else
					*(dest++) = '0';
#		endif
#		if 20<CHAR_BIT
					*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/(65536ULL*16ULL))%16];
#		else
					*(dest++) = '0';
#		endif
					*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/(65536ULL))%16];
					}
				else{
					*(dest++) = 'u';
					}
				*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/4096U)%16];
				*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/256U)%16];
				*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/16U)%16];
				*(dest++) = C_HEXADECIMAL_DIGITS[tmp%16];
				continue;
				}
#	else
			if (must_escape && 65536ULL*65536ULL>tmp)
				{
				if (65536U<=tmp)
					{
					*(dest++) = 'U';
					*(dest++) = C_HEXADECIMAL_DIGITS[tmp/(65536ULL*4096ULL)];
					*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/(65536ULL*256ULL))%16];
					*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/(65536ULL*16ULL))%16];
					*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/(65536ULL))%16];
					}
				else{
					*(dest++) = 'u';
					}
				*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/4096U)%16];
				*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/256U)%16];
				*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/16U)%16];
				*(dest++) = C_HEXADECIMAL_DIGITS[tmp%16];
				continue;
				}
			if (must_escape) _fatal("unescapable string: escaping code points above 2^32-1 followed by a hexadecimal digit disallowed by C99; bug for C++ (should be escaping the hexadecimal digit as well)");
#	endif
#endif
			// hex-escape for legibility
			*(dest++) = 'x';
			unsigned char power = 1;
			while(power<=tmp/16) power *= 16;
			do	{
				*(dest++) = list_hexadecimal_digits[tmp/power];
				tmp %= power;
				power /= 16;
				}
			while(0<power);
			continue;
			}
		*(dest++) = src[i];
		}
	while(src_len > ++i);
}

static void EscapeCString(char* dest, const my_UNICODE* src, size_t src_len)
{	// \todo fix ASCII dependency.
	assert(src);
	assert(0<src_len);
	size_t i = 0;
	do	{
		if (0!=src[i] && UCHAR_MAX>=src[i])
			{	// deal with the symbolic escapes
			const char* const symbolic_escaped_escape = strchr(c99_symbolic_escaped_escapes,src[i]);
			if (symbolic_escaped_escape)
				{
				*(dest++) = '\\';
				*(dest++) = c99_symbolic_escapes[symbolic_escaped_escape-c99_symbolic_escaped_escapes];
				continue;
				};
			//! \todo probably too cautious; we need to escape ? only if a ? immediately precedes it (break trigraph sequences)
			if (strchr("\"\\?",src[i]))
				{
				*(dest++) = '\\';
				*(dest++) = src[i];
				continue;
				}
			};
		// Note that hex escape sequence requires at least 2 bytes, so octal is just-as-cheap through decimal 63 (which takes 3 bytes in hex)
		// however, octal isn't that user friendly; we clearly want to octal-escape only through 7
		// \bug need test cases
		if (!C_IsPrintableChar(src[i]))
			{	// note that octal escaping can only go up to 511, which is a problem if our CHAR_BIT exceeds 9 
			my_UNICODE tmp = src[i];
			bool must_escape = (1<src_len-i && UCHAR_MAX>=src[i] && strchr(list_hexadecimal_digits,src[i+1]));
			*(dest++) = '\\';
			//! \todo fix to handle target CHAR_BIT different than ours
#if 9>=CHAR_BIT*C_UNICODE_SIZE
			if (7U>=tmp || must_escape)
#else
			if (7U>=tmp || (must_escape && 511U>=tmp))
#endif
				{	// octal-escape if it's readable, or if it prevents escaping the next printable character
				if (must_escape && 64U>tmp)
					{
					*(dest++) = '0';
					if (8U>tmp) *(dest++) = '0';
					};
				do	{
					*(dest++) = (tmp%8U)+(unsigned char)('0');
					tmp /= 8U;
					}
				while(0<tmp);
				continue;
				}
#if 32>=CHAR_BIT*C_UNICODE_SIZE
			if (must_escape)
				{
				if (65536U<=tmp)
					{
					*(dest++) = 'U';
#	if 28<CHAR_BIT*C_UNICODE_SIZE
					*(dest++) = C_HEXADECIMAL_DIGITS[tmp/(65536ULL*4096ULL)];
#	else
					*(dest++) = '0';
#	endif
#	if 24<CHAR_BIT*C_UNICODE_SIZE
					*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/(65536ULL*256ULL))%16];
#	else
					*(dest++) = '0';
#	endif
#	if 20<CHAR_BIT*C_UNICODE_SIZE
					*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/(65536ULL*16ULL))%16];
#	else
					*(dest++) = '0';
#	endif
					*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/(65536ULL))%16];
					}
				else{
					*(dest++) = 'u';
					}
				*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/4096U)%16];
				*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/256U)%16];
				*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/16U)%16];
				*(dest++) = C_HEXADECIMAL_DIGITS[tmp%16];
				continue;
				}
#else
			if (must_escape && 65536ULL*65536ULL>tmp)
				{
				if (65536U<=tmp)
					{
					*(dest++) = 'U';
					*(dest++) = C_HEXADECIMAL_DIGITS[tmp/(65536ULL*4096ULL)];
					*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/(65536ULL*256ULL))%16];
					*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/(65536ULL*16ULL))%16];
					*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/(65536ULL))%16];
					}
				else{
					*(dest++) = 'u';
					}
				*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/4096U)%16];
				*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/256U)%16];
				*(dest++) = C_HEXADECIMAL_DIGITS[(tmp/16U)%16];
				*(dest++) = C_HEXADECIMAL_DIGITS[tmp%16];
				continue;
				}
			if (must_escape) _fatal("unescapable string: escaping code points above 2^32-1 followed by a hexadecimal digit disallowed by C99; bug for C++ (should be escaping the hexadecimal digit as well)");
#endif
			// hex-escape for legibility
			*(dest++) = 'x';
			unsigned char power = 1;
			while(power<=tmp/16) power *= 16;
			do	{
				*(dest++) = list_hexadecimal_digits[tmp/power];
				tmp %= power;
				power /= 16;
				}
			while(0<power);
			continue;
			}
		*(dest++) = src[i];
		}
	while(src_len > ++i);
}

static size_t octal_escape_length(const char* const src, const size_t ub)
{
	assert(src);
	const size_t oct_len = strspn(src,C_OCTAL_DIGITS);
	return (ub<oct_len) ? ub : oct_len;
}

static unsigned int eval_octal_escape(const char* src, size_t src_len)
{
	assert(src);
	assert(0<src_len && src_len<=3);
	unsigned int tmp = 0;
	do	{
		const unsigned char tmp2 = *(src++);
		assert((in_range<'0','7'>(tmp2)));
		tmp *= 8;
		tmp += (tmp2-'0');
		}
	while(0< --src_len);
	return tmp;
}

static size_t hex_escape_length(const char* const src, const size_t ub)
{
	assert(src);
	const size_t hex_len = strspn(src,C_HEXADECIMAL_DIGITS);
	return (ub<hex_len) ? ub : hex_len;
}

static umaxint eval_hex_escape(const char* src, size_t src_len)
{
	assert(src);
	assert(0<src_len);
	unsigned_var_int tmp(0,unsigned_var_int::bytes_from_bits(VM_MAX_BIT_PLATFORM));
#ifndef NDEBUG
	umaxint uchar_max(target_machine->unsigned_max<virtual_machine::std_int_long_long>());
	uchar_max >>= 4;
#endif
	do	{
		const unsigned char tmp2 = *(src++);
		assert(strchr(C_HEXADECIMAL_DIGITS,tmp2));
		assert(uchar_max>=tmp);
		tmp <<= 4;
		tmp += InterpretHexadecimalDigit(tmp2);
		}
	while(0< --src_len);
	return tmp;
}

// must remain synchronized with RobustEscapedCharLength_C
static size_t EscapedCharLength_C(const char* src, size_t src_len)
{
	assert(src);
	assert(0<src_len);
	if ('\\' != *src) return 1;
	assert(1<src_len && '\0'!=src[1]);
	if (   strchr(C99_SYMBOLIC_ESCAPES,src[1])
		|| strchr(C99_COPY_ESCAPES,src[1]))
		return 2;
	if ('U'==src[1] || 'u'==src[1] || 'x'==src[1])
		{
		assert(3<=src_len);
		const size_t hex_len = hex_escape_length(src+2,src_len-2U);
		switch(src[1])
		{
		case 'u':	{	// UNICODE escape
					assert(4<=hex_len);
					return 6;
					}
		case 'U':	{	// astral UNICODE escape
					assert(8<=hex_len);
					return 10;
					}
		case 'x':	{	// hexadecimal escape
					assert(0<hex_len);
					return hex_len+2;
					}
		}
		};
	const size_t oct_len = octal_escape_length(src+1,(3U>src_len-1U) ? 3U : src_len-1U);
	assert(0<oct_len);
	return oct_len+1;
}

// must remain synchronized with EscapedCharLength_C
static size_t RobustEscapedCharLength_C(const char* src, size_t src_len)
{
	assert(src);
	assert(0<src_len);
	if ('\\' != *src) return 1;
	if (1>=src_len || '\0'==src[1]) return 0;
	if (   strchr(C99_SYMBOLIC_ESCAPES,src[1])
		|| strchr(C99_COPY_ESCAPES,src[1]))
		return 2;
	if ('U'==src[1] || 'u'==src[1] || 'x'==src[1])
		{
		if (2>=src_len) return 0;
		const size_t hex_len = hex_escape_length(src+2,src_len-2U);
		switch(src[1])
		{
		case 'u':	{	// UNICODE escape
					if (4>hex_len) return 0;
					return 6;
					}
		case 'U':	{	// astral UNICODE escape
					if (8>hex_len) return 0;
					return 10;
					}
		case 'x':	{	// hexadecimal escape
					if (0>=hex_len) return 0;
					return hex_len+2;
					}
		}
		};
	const size_t oct_len = octal_escape_length(src+1,(3U>src_len-1U) ? 3U : src_len-1U);
	if (0>=oct_len) return 0;
	return oct_len+1;
}

static size_t LengthOfUnescapedCString(const char* src, size_t src_len)
{
	assert(src);
	assert(0<src_len);

	size_t analyze_length = 0;
	size_t i = 0;
	do	{
		++analyze_length;
		i += EscapedCharLength_C(src+i,src_len-i);
		}
	while(src_len > i);
	return analyze_length;
}

static uintmax_t _eval_character(const char* src, size_t src_len)
{
	assert(src);
	assert(0<src_len);
	if (1==src_len) return (unsigned char)(*src);
	const char* tmp_escape = strchr(C_OCTAL_DIGITS,src[1]);
	if (tmp_escape)
		{
		const size_t oct_len = octal_escape_length(src+1,(3U>src_len-1U) ? 3U : src_len-1U);
		assert(0<oct_len);
		return eval_octal_escape(src+1,oct_len);
		};
	if (2==src_len)
		{
		tmp_escape = strchr(c99_symbolic_escapes,src[1]);
		if (tmp_escape) return (unsigned char)(c99_symbolic_escaped_escapes[tmp_escape-c99_symbolic_escapes]);

		assert(strchr(C99_COPY_ESCAPES,src[1]));
		return (unsigned char)(src[1]);
		}
	assert((strchr("uUx",src[1])));
	assert(3<=src_len);
	return eval_hex_escape(src+2,src_len-2).to_uint();
}

static void UnescapeCString(char* dest, const char* src, size_t src_len)
{	//! \todo cross-compiler augmentation target, dest needs to be able represent target strings
	assert(src);
	assert(0<src_len);
	assert(CHAR_BIT>=target_machine->C_char_bit());

	size_t i = 0;
	do	{
		const size_t step = EscapedCharLength_C(src+i,src_len-i);
		assert(UCHAR_MAX>=_eval_character(src+i,step));
		*(dest++) = _eval_character(src+i,step);
		i += step;
		}
	while(src_len > i);
}

static void UnescapeCWideString(my_UNICODE* dest, const char* src, size_t src_len)
{	//! \todo cross-compiler change target, dest needs to be able represent target strings
	assert(src);
	assert(0<src_len);
	assert(C_UNICODE_MAX>=target_machine->unsigned_max(target_machine->UNICODE_wchar_t()));

	size_t i = 0;
	do	{
		const size_t step = EscapedCharLength_C(src+i,src_len-i);
		assert(C_UNICODE_MAX>=_eval_character(src+i,step));
		*(dest++) = _eval_character(src+i,step);
		i += step;
		}
	while(src_len > i);
}

bool IsLegalCString(const char* x, size_t x_len)
{
	assert(x);
	assert(0<x_len);
	if ('"' != x[x_len-1]) return false;
	if (0 == --x_len) return false;
	const bool wide_string = 'L' == *x;
	if (wide_string)
		{
		if (0 == --x_len) return false;	
		++x;
		}
	if ('"' != *(x++)) return false;
	if (0 == --x_len) return true;	// empty string is legal
	const umaxint& uchar_max = target_machine->unsigned_max((wide_string) ? target_machine->UNICODE_wchar_t() : virtual_machine::std_int_char);

	size_t i = 0;
	do	{
		const size_t step = RobustEscapedCharLength_C(x+i,x_len-i);
		if (0==step) return false;
		if (uchar_max<_eval_character(x+i,step)) return false;
		i += step;
		}
	while(x_len > i);
	return true;
}

bool IsLegalCCharacterLiteral(const char* x, size_t x_len)
{
	assert(x);
	assert(0<x_len);
	if ('\'' != x[x_len-1]) return false;
	if (0 == --x_len) return false;
	const bool wide_string = 'L' == *x;
	if (wide_string)
		{
		if (0 == --x_len) return false;	
		++x;
		}
	if ('\'' != *(x++)) return false;
	if (0 == --x_len) return false;	// empty character literal is illegal
	const umaxint& uchar_max = target_machine->unsigned_max((wide_string) ? target_machine->UNICODE_wchar_t() : virtual_machine::std_int_char);

	size_t i = 0;
	do	{
		const size_t step = RobustEscapedCharLength_C(x+i,x_len-i);
		if (0==step) return false;
		if (uchar_max<_eval_character(x+i,step)) return false;
		i += step;
		}
	while(x_len > i);
	return true;
}

static size_t LengthOfCStringLiteral(const char* src, size_t src_len)
{
	assert(src);
	assert(2<=src_len);
	const bool wide_str = ('L'==src[0]);
	if (wide_str)
		{
		++src;
		if (2 > --src_len) return 0;
		}
	if ('"'!=src[--src_len]) return 0;
	if ('"'!=*(src++)) return 0;
	if (0 == --src_len) return 1;
	return LengthOfUnescapedCString(src,src_len)+1;
}

static size_t LengthOfCCharLiteral(const char* src, size_t src_len)
{
	assert(src);
	assert(2<=src_len);
	const bool wide_char = ('L'==src[0]);
	if (wide_char)
		{
		++src;
		if (2 > --src_len) return 0;
		}
	if ('\''!=src[--src_len]) return 0;
	if ('\''!=*(src++)) return 0;
	if (0 == --src_len) return 1;
	return LengthOfUnescapedCString(src,src_len);
}

/*! 
 * Locates the character in a character literal as a substring.  Use this as preparation for "collapsing in place".
 * 
 * \param src string to locate character in
 * \param src_len length of string
 * \param target_idx index we actually want
 * \param char_offset offset of character literal body
 * \param char_len length of character literal body
 * 
 * \return bool true iff located as-is
 */
bool LocateCCharacterLiteralAt(const char* const src, size_t src_len, size_t target_idx, size_t& char_offset, size_t& char_len)
{
	assert(src);
	assert(2<=src_len);
	assert(IsLegalCString(src,src_len));
	const char* src2 = src;
	const size_t C_str_len = LengthOfCStringLiteral(src,src_len);
	assert(C_str_len>target_idx);
	// NUL; using <= to be failsafed in release mode
	if (target_idx+1<=C_str_len) return false;
	const bool wide_str = ('L'==src[0]);
	if (wide_str)
		{
		++src2;
		--src_len;
		assert(2<=src_len);
		}
	++src2;
	src_len -= 2;

	size_t i = 0;
	size_t j = 0;
	do	{
		const size_t step = EscapedCharLength_C(src2+i,src_len-i);
		if (j==target_idx)
			{
			char_offset = i+(src2-src);
			char_len = step;
			return true;
			}
		i += step;
		++j;
		}
	while(src_len > i);
	return false;
}

static char* GetCCharacterLiteralAt(const char* src, size_t src_len, size_t target_idx)
{
	assert(src);
	assert(2<=src_len);
	assert(IsLegalCString(src,src_len));
	char* tmp = NULL;
	const size_t C_str_len = LengthOfCStringLiteral(src,src_len);
	assert(C_str_len>target_idx);
	const bool wide_str = ('L'==src[0]);
	if (wide_str)
		{
		++src;
		--src_len;
		assert(2<=src_len);
		}
	++src;
	src_len -= 2;
	if (target_idx+1==C_str_len)
		{
		char* tmp2 = _new_buffer_nonNULL_throws<char>((wide_str) ? 6 : 5);
		tmp = tmp2;
		if (wide_str) *(tmp2++) = 'L';
		strcpy(tmp2,"'\\0'");
		return tmp;
		};

	size_t i = 0;
	size_t j = 0;
	do	{
		const size_t step = EscapedCharLength_C(src+i,src_len-i);
		if (j==target_idx)
			{
			char* tmp2 = _new_buffer_nonNULL_throws<char>(((wide_str) ? 3 : 2)+step);
			tmp = tmp2;
			if (wide_str) *(tmp2++) = 'L';
			*(tmp2++) = '\'';
			strncpy(tmp2,src+i,step);
			*(tmp2 += step) = '\'';
			return tmp;
			}
		i += step;
		++j;
		}
	while(src_len > i);
	return tmp;
}

/*! 
 * concatenates two string literals into a string literal.
 * 
 * \param src		C string literal 1, escaped
 * \param src_len	length of C string literal 1
 * \param src2		C string literal 2, escaped
 * \param src2_len	length of C string literal 2
 * \param target	where to put a dynamically created string literal, escaped
 * 
 * \return 1: success, new token in target
 * \return 0: failure
 * \return -1: use first string as concatenation
 * \return -2: use second string as concatenation
 * \return -3: use first string as content, but wide-string header from second string
 * \return -4: use second string as content, but wide-string header from first string
 * \return -5: failed to allocate memory
 *
 * \post return value is between -5 and 1 inclusive
 * \post returns 1 iff NULL!=target
 * \post if NULL!=target, target points to a valid string literal
 */
static int ConcatenateCStringLiterals(const char* src, size_t src_len, const char* src2, size_t src2_len, char*& target)
{
	assert(src);
	assert(src2);
	assert(2<=src_len);
	assert(2<=src2_len);
	assert(IsLegalCString(src,src_len));
	assert(IsLegalCString(src2,src2_len));
	assert(!target);

	const char* str1 = src;
	const char* str2 = src2;
	size_t str1_len = src_len;
	size_t str2_len = src2_len;
	const bool str1_wide = ('L'==src[0]);
	const bool str2_wide = ('L'==src2[0]);
	const bool str_target_wide = str1_wide || str2_wide;

	if (str1_wide)
		{
		++str1;
		--str1_len;
		}
	if (str2_wide)
		{
		++str2;
		--str2_len;
		}
	++str1;
	++str2;
	str1_len -= 2;
	str2_len -= 2;

	if (0==str1_len)
		{
		if (str2_wide==str_target_wide) return -2;
		return -4;
		}
	if (0==str2_len)
		{
		if (str1_wide==str_target_wide) return -1;
		return -3;
		}

	const size_t str1_un_len = LengthOfUnescapedCString(src,src_len);

	/* will simple algorithm work? */
	bool simple_paste_ok = !strchr(C_HEXADECIMAL_DIGITS,*str2);
	if (!simple_paste_ok)
		{
		POD_pair<size_t,size_t> loc;
#ifndef NDEBUG
		assert(LocateCCharacterLiteralAt(src,src_len,str1_un_len-1,loc.first,loc.second));
#else
		if (!LocateCCharacterLiteralAt(src,src_len,str1_un_len-1,loc.first,loc.second)) return 0;
#endif
		// octal and hexadecimal are bad for simple pasting
		if (1==loc.second || !strchr("x01234567",src[loc.first+1]))
			simple_paste_ok = true;
		// but a 3-digit octal escape isn't
		else if ('x'!=src[loc.first+1] && 4==loc.second)
			simple_paste_ok = true;
		if (!simple_paste_ok)
			{	// a hex escape of more than 8 effective digits will force a hexadecimal digit to be escaped, pretty much requires hex-escaping in C99 (charset weirdness)
				// C++ will let us off because it allows escaping printable characters, but this still is not good (charset weirdness) so we don't handle it yet
				// fail immediately because we haven't implemented cross-charset preprocessing yet
			if ('x'==src[loc.first+1] && 8<(loc.second-2)-strspn(src+loc.first+2,"0")) return 0;
			}
		}

	if (simple_paste_ok)
		{
		const size_t new_start = (str_target_wide) ? 2 : 1;
		const size_t new_width = str1_len+str2_len+new_start+1U;
		target = reinterpret_cast<char*>(calloc(new_width,1));
		if (!target) return -5;
		target[new_width-1] = '"';
		target[new_start-1] = '"';
		if (str_target_wide) target[0] = 'L';
		strncpy(target+new_start,str1,str1_len);
		strncpy(target+new_start+str1_len,str2,str2_len);
		assert(IsLegalCString(target,new_width));
		return 1;
		};

	// working buffer
	const size_t str2_un_len = LengthOfUnescapedCString(src2,src2_len);
	const size_t buf_len = str1_un_len+str2_un_len;
	union_pair<char*,my_UNICODE*> buf;
	if (str_target_wide)
		{
		buf.second = zaimoni::_new_buffer<my_UNICODE>(buf_len);
		if (!buf.second) return -5;
		UnescapeCWideString(buf.second,str1,str1_len);
		UnescapeCWideString(buf.second+str1_un_len,str2,str2_len);
		//! \todo C vs C++
		const size_t target_len = LengthOfEscapedCString(buf.second,buf_len);
		target = zaimoni::_new_buffer<char>(target_len);
		if (!target) return free(buf.second),-5; 
		EscapeCString(target,buf.second,buf_len);
		free(buf.second);
		assert(IsLegalCString(target,target_len));
		return 1;
		}
	else{
		buf.first = zaimoni::_new_buffer<char>(buf_len);
		if (!buf.first) return -5;
		UnescapeCString(buf.first,str1,str1_len);
		UnescapeCString(buf.first+str1_un_len,str2,str2_len);
		const size_t target_len = LengthOfEscapedCString(buf.first,buf_len);
		target = zaimoni::_new_buffer<char>(target_len);
		if (!target) return free(buf.first),-5;
		EscapeCString(target,buf.first,buf_len);
		free(buf.first);
		assert(IsLegalCString(target,target_len));
		return 1;
		}

}

static uintmax_t EvalCharacterLiteral(const char* src, size_t src_len)
{
	assert(src);
	assert(3<=src_len);
	assert(IsLegalCCharacterLiteral(src,src_len));
	const bool is_wide = 'L'== *src;
	if (is_wide)
		{
		++src;
		--src_len;
		};
	++src;
	src_len -= 2;

	const unsigned int target_char_bit = (is_wide ? target_machine->C_bit(target_machine->UNICODE_wchar_t()) : target_machine->C_char_bit());
	assert(sizeof(uintmax_t)*CHAR_BIT >= target_char_bit);
	const uintmax_t safe_limit = (UINTMAX_MAX>>target_char_bit);
	uintmax_t tmp = 0;

	size_t i = 0;
	do	{
		tmp <<= target_char_bit;
		const size_t step = EscapedCharLength_C(src+i,src_len-i);
		tmp += _eval_character(src+i,step);
		i += step;
		}
	while(src_len > i && safe_limit>=tmp);
	return tmp;
}

bool CCharLiteralIsFalse(const char* x,size_t x_len)
{
	assert(x);
	assert(0<x_len);
	assert(IsLegalCCharacterLiteral(x,x_len));
	const uintmax_t result = EvalCharacterLiteral(x,x_len);
	if (0==result) return true;
	if (bool_options[boolopt::char_is_unsigned]) return false;
	switch(target_machine->C_signed_int_representation())
	{
	default: return false;
	case virtual_machine::ones_complement:		{
												unsigned_var_int tmp(0,unsigned_var_int::bytes_from_bits(VM_MAX_BIT_PLATFORM));
												if (VM_MAX_BIT_PLATFORM>target_machine->C_char_bit()) tmp.set(target_machine->C_char_bit());
												tmp -= 1;
												return tmp==result;
												}
	case virtual_machine::sign_and_magnitude:	{
												unsigned_var_int tmp(0,unsigned_var_int::bytes_from_bits(VM_MAX_BIT_PLATFORM));
												tmp.set(target_machine->C_char_bit()-1);
												return tmp==result;
												}
	};
}

// not sure if we need this bit, but it matches the standards
// PM expression is C++ only
#define PARSE_PRIMARY_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-1))
#define PARSE_STRICT_POSTFIX_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-2))
#define PARSE_STRICT_UNARY_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-3))
#define PARSE_STRICT_CAST_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-4))
#define PARSE_STRICT_PM_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-5))
#define PARSE_STRICT_MULT_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-6))
#define PARSE_STRICT_ADD_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-7))
#define PARSE_STRICT_SHIFT_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-8))
#define PARSE_STRICT_RELATIONAL_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-9))
#define PARSE_STRICT_EQUALITY_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-10))
#define PARSE_STRICT_BITAND_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-11))
#define PARSE_STRICT_BITXOR_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-12))
#define PARSE_STRICT_BITOR_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-13))
#define PARSE_STRICT_LOGICAND_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-14))
#define PARSE_STRICT_LOGICOR_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-15))
#define PARSE_STRICT_CONDITIONAL_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-16))
#define PARSE_STRICT_ASSIGNMENT_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-17))
#define PARSE_STRICT_COMMA_EXPRESSION ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-18))

/* strict type categories of parsing */
#define PARSE_PRIMARY_TYPE ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-19))
#define PARSE_UNION_TYPE ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-20))
#define PARSE_CLASS_STRUCT_TYPE ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-21))
#define PARSE_ENUM_TYPE ((lex_flags)(1)<<(sizeof(lex_flags)*CHAR_BIT-22))

// check for collision with lowest three bits
BOOST_STATIC_ASSERT(sizeof(lex_flags)*CHAR_BIT-parse_tree::PREDEFINED_STRICT_UB>=22);

/* nonstrict expression types */
#define PARSE_POSTFIX_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION)
#define PARSE_UNARY_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION | PARSE_STRICT_UNARY_EXPRESSION)
#define PARSE_CAST_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION | PARSE_STRICT_UNARY_EXPRESSION | PARSE_STRICT_CAST_EXPRESSION)
#define PARSE_PM_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION | PARSE_STRICT_UNARY_EXPRESSION | PARSE_STRICT_CAST_EXPRESSION | PARSE_STRICT_PM_EXPRESSION)
#define PARSE_MULT_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION | PARSE_STRICT_UNARY_EXPRESSION | PARSE_STRICT_CAST_EXPRESSION | PARSE_STRICT_PM_EXPRESSION | PARSE_STRICT_MULT_EXPRESSION)
#define PARSE_ADD_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION | PARSE_STRICT_UNARY_EXPRESSION | PARSE_STRICT_CAST_EXPRESSION | PARSE_STRICT_PM_EXPRESSION | PARSE_STRICT_MULT_EXPRESSION | PARSE_STRICT_ADD_EXPRESSION)
#define PARSE_SHIFT_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION | PARSE_STRICT_UNARY_EXPRESSION | PARSE_STRICT_CAST_EXPRESSION | PARSE_STRICT_PM_EXPRESSION | PARSE_STRICT_MULT_EXPRESSION | PARSE_STRICT_ADD_EXPRESSION | PARSE_STRICT_SHIFT_EXPRESSION)
#define PARSE_RELATIONAL_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION | PARSE_STRICT_UNARY_EXPRESSION | PARSE_STRICT_CAST_EXPRESSION | PARSE_STRICT_PM_EXPRESSION | PARSE_STRICT_MULT_EXPRESSION | PARSE_STRICT_ADD_EXPRESSION | PARSE_STRICT_SHIFT_EXPRESSION | PARSE_STRICT_RELATIONAL_EXPRESSION)
#define PARSE_EQUALITY_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION | PARSE_STRICT_UNARY_EXPRESSION | PARSE_STRICT_CAST_EXPRESSION | PARSE_STRICT_PM_EXPRESSION | PARSE_STRICT_MULT_EXPRESSION | PARSE_STRICT_ADD_EXPRESSION | PARSE_STRICT_SHIFT_EXPRESSION | PARSE_STRICT_RELATIONAL_EXPRESSION | PARSE_STRICT_EQUALITY_EXPRESSION)
#define PARSE_BITAND_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION | PARSE_STRICT_UNARY_EXPRESSION | PARSE_STRICT_CAST_EXPRESSION | PARSE_STRICT_PM_EXPRESSION | PARSE_STRICT_MULT_EXPRESSION | PARSE_STRICT_ADD_EXPRESSION | PARSE_STRICT_SHIFT_EXPRESSION | PARSE_STRICT_RELATIONAL_EXPRESSION | PARSE_STRICT_EQUALITY_EXPRESSION | PARSE_STRICT_BITAND_EXPRESSION)
#define PARSE_BITXOR_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION | PARSE_STRICT_UNARY_EXPRESSION | PARSE_STRICT_CAST_EXPRESSION | PARSE_STRICT_PM_EXPRESSION | PARSE_STRICT_MULT_EXPRESSION | PARSE_STRICT_ADD_EXPRESSION | PARSE_STRICT_SHIFT_EXPRESSION | PARSE_STRICT_RELATIONAL_EXPRESSION | PARSE_STRICT_EQUALITY_EXPRESSION | PARSE_STRICT_BITAND_EXPRESSION | PARSE_STRICT_BITXOR_EXPRESSION)
#define PARSE_BITOR_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION | PARSE_STRICT_UNARY_EXPRESSION | PARSE_STRICT_CAST_EXPRESSION | PARSE_STRICT_PM_EXPRESSION | PARSE_STRICT_MULT_EXPRESSION | PARSE_STRICT_ADD_EXPRESSION | PARSE_STRICT_SHIFT_EXPRESSION | PARSE_STRICT_RELATIONAL_EXPRESSION | PARSE_STRICT_EQUALITY_EXPRESSION | PARSE_STRICT_BITAND_EXPRESSION | PARSE_STRICT_BITXOR_EXPRESSION | PARSE_STRICT_BITOR_EXPRESSION)
#define PARSE_LOGICAND_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION | PARSE_STRICT_UNARY_EXPRESSION | PARSE_STRICT_CAST_EXPRESSION | PARSE_STRICT_PM_EXPRESSION | PARSE_STRICT_MULT_EXPRESSION | PARSE_STRICT_ADD_EXPRESSION | PARSE_STRICT_SHIFT_EXPRESSION | PARSE_STRICT_RELATIONAL_EXPRESSION | PARSE_STRICT_EQUALITY_EXPRESSION | PARSE_STRICT_BITAND_EXPRESSION | PARSE_STRICT_BITXOR_EXPRESSION | PARSE_STRICT_BITOR_EXPRESSION | PARSE_STRICT_LOGICAND_EXPRESSION)
#define PARSE_LOGICOR_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION | PARSE_STRICT_UNARY_EXPRESSION | PARSE_STRICT_CAST_EXPRESSION | PARSE_STRICT_PM_EXPRESSION | PARSE_STRICT_MULT_EXPRESSION | PARSE_STRICT_ADD_EXPRESSION | PARSE_STRICT_SHIFT_EXPRESSION | PARSE_STRICT_RELATIONAL_EXPRESSION | PARSE_STRICT_EQUALITY_EXPRESSION | PARSE_STRICT_BITAND_EXPRESSION | PARSE_STRICT_BITXOR_EXPRESSION | PARSE_STRICT_BITOR_EXPRESSION | PARSE_STRICT_LOGICAND_EXPRESSION | PARSE_STRICT_LOGICOR_EXPRESSION)
#define PARSE_CONDITIONAL_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION | PARSE_STRICT_UNARY_EXPRESSION | PARSE_STRICT_CAST_EXPRESSION | PARSE_STRICT_PM_EXPRESSION | PARSE_STRICT_MULT_EXPRESSION | PARSE_STRICT_ADD_EXPRESSION | PARSE_STRICT_SHIFT_EXPRESSION | PARSE_STRICT_RELATIONAL_EXPRESSION | PARSE_STRICT_EQUALITY_EXPRESSION | PARSE_STRICT_BITAND_EXPRESSION | PARSE_STRICT_BITXOR_EXPRESSION | PARSE_STRICT_BITOR_EXPRESSION | PARSE_STRICT_LOGICAND_EXPRESSION | PARSE_STRICT_LOGICOR_EXPRESSION | PARSE_STRICT_CONDITIONAL_EXPRESSION)
#define PARSE_ASSIGNMENT_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION | PARSE_STRICT_UNARY_EXPRESSION | PARSE_STRICT_CAST_EXPRESSION | PARSE_STRICT_PM_EXPRESSION | PARSE_STRICT_MULT_EXPRESSION | PARSE_STRICT_ADD_EXPRESSION | PARSE_STRICT_SHIFT_EXPRESSION | PARSE_STRICT_RELATIONAL_EXPRESSION | PARSE_STRICT_EQUALITY_EXPRESSION | PARSE_STRICT_BITAND_EXPRESSION | PARSE_STRICT_BITXOR_EXPRESSION | PARSE_STRICT_BITOR_EXPRESSION | PARSE_STRICT_LOGICAND_EXPRESSION | PARSE_STRICT_LOGICOR_EXPRESSION | PARSE_STRICT_CONDITIONAL_EXPRESSION | PARSE_STRICT_ASSIGNMENT_EXPRESSION)
#define PARSE_EXPRESSION (PARSE_PRIMARY_EXPRESSION | PARSE_STRICT_POSTFIX_EXPRESSION | PARSE_STRICT_UNARY_EXPRESSION | PARSE_STRICT_CAST_EXPRESSION | PARSE_STRICT_PM_EXPRESSION | PARSE_STRICT_MULT_EXPRESSION | PARSE_STRICT_ADD_EXPRESSION | PARSE_STRICT_SHIFT_EXPRESSION | PARSE_STRICT_RELATIONAL_EXPRESSION | PARSE_STRICT_EQUALITY_EXPRESSION | PARSE_STRICT_BITAND_EXPRESSION | PARSE_STRICT_BITXOR_EXPRESSION | PARSE_STRICT_BITOR_EXPRESSION | PARSE_STRICT_LOGICAND_EXPRESSION | PARSE_STRICT_LOGICOR_EXPRESSION | PARSE_STRICT_CONDITIONAL_EXPRESSION | PARSE_STRICT_ASSIGNMENT_EXPRESSION | PARSE_STRICT_COMMA_EXPRESSION)

/* nonstrict type categories */
#define PARSE_TYPE (PARSE_PRIMARY_TYPE | PARSE_UNION_TYPE | PARSE_CLASS_STRUCT_TYPE | PARSE_ENUM_TYPE)

/* already-parsed */
#define PARSE_OBVIOUS (PARSE_EXPRESSION | PARSE_TYPE | parse_tree::INVALID)

#define PARSE_PAREN_PRIMARY_PASSTHROUGH (parse_tree::CONSTANT_EXPRESSION)

/* XXX this may belong with parse_tree XXX */
static void simple_error(parse_tree& src, const char* const err_str)
{
	assert(err_str && *err_str);
	if (parse_tree::INVALID & src.flags) return;
	src.flags |= parse_tree::INVALID;
	message_header(src.index_tokens[0]);
	INC_INFORM(ERR_STR);
	INC_INFORM(src);
	INFORM(err_str);
	zcc_errors.inc_error();
}

//! \todo generalize -- function pointer parameter target, functor target
static size_t _count_identifiers(const parse_tree& src)
{
	size_t count_id = 0;
	if (src.index_tokens[0].token.first && C_TESTFLAG_IDENTIFIER==src.index_tokens[0].flags) ++count_id;
	if (src.index_tokens[1].token.first && C_TESTFLAG_IDENTIFIER==src.index_tokens[1].flags) ++count_id;
	size_t i = src.size<0>();
	while(0<i) count_id += _count_identifiers(src.data<0>()[--i]);
	i = src.size<1>();
	while(0<i) count_id += _count_identifiers(src.data<1>()[--i]);
	i = src.size<2>();
	while(0<i) count_id += _count_identifiers(src.data<2>()[--i]);
	return count_id;
}

static bool is_naked_parentheses_pair(const parse_tree& src)
{
	return		robust_token_is_char<'('>(src.index_tokens[0].token)
			&&	robust_token_is_char<')'>(src.index_tokens[1].token)
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename && src.index_tokens[1].src_filename
#endif
			&&	src.empty<1>() && src.empty<2>();
}

#ifndef NDEBUG
static bool is_array_deref_strict(const parse_tree& src)
{
	return		robust_token_is_char<'['>(src.index_tokens[0].token)
			&&	robust_token_is_char<']'>(src.index_tokens[1].token)
			&&	src.index_tokens[0].src_filename && src.index_tokens[1].src_filename
			&&	1==src.size<0>() && (PARSE_EXPRESSION & src.data<0>()->flags)			// content of [ ]
			&&	1==src.size<1>() && (PARSE_POSTFIX_EXPRESSION & src.data<1>()->flags)	// prefix arg of [ ]
			&&	src.empty<2>();
}
#endif

static bool is_array_deref(const parse_tree& src)
{
	return		robust_token_is_char<'['>(src.index_tokens[0].token)
			&&	robust_token_is_char<']'>(src.index_tokens[1].token)
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename && src.index_tokens[1].src_filename
#endif
			&&	1==src.size<0>() && (PARSE_EXPRESSION & src.data<0>()->flags)			// content of [ ]
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)	// prefix arg of [ ]
			&&	src.empty<2>();
}

#define C99_UNARY_SUBTYPE_PLUS 1
#define C99_UNARY_SUBTYPE_NEG 2
#define C99_UNARY_SUBTYPE_DEREF 3
#define C99_UNARY_SUBTYPE_ADDRESSOF 4
#define C99_UNARY_SUBTYPE_NOT 5
#define C99_UNARY_SUBTYPE_COMPL 6

template<char c> static bool is_C99_unary_operator_expression(const parse_tree& src)
{
	return		robust_token_is_char<c>(src.index_tokens[0].token)
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>() && src.empty<1>()
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags);
//			&&	1==src.size<2>() && (PARSE_CAST_EXPRESSION & src.data<2>()->flags);
}

static bool is_CPP_logical_NOT_expression(const parse_tree& src)
{
	return		(robust_token_is_char<'!'>(src.index_tokens[0].token) || robust_token_is_string<3>(src.index_tokens[0].token,"not"))
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>() && src.empty<1>()
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags);
//			&&	1==src.size<2>() && (PARSE_CAST_EXPRESSION & src.data<2>()->flags);
}

static bool is_CPP_bitwise_complement_expression(const parse_tree& src)
{
	return		(robust_token_is_char<'~'>(src.index_tokens[0].token) || robust_token_is_string<5>(src.index_tokens[0].token,"compl"))
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>() && src.empty<1>()
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags);
//			&&	1==src.size<2>() && (PARSE_CAST_EXPRESSION & src.data<2>()->flags);
}

#define C99_MULT_SUBTYPE_DIV 1
#define C99_MULT_SUBTYPE_MOD 2
#define C99_MULT_SUBTYPE_MULT 3

BOOST_STATIC_ASSERT(C99_UNARY_SUBTYPE_DEREF==C99_MULT_SUBTYPE_MULT);

#ifndef NDEBUG
static bool is_C99_mult_operator_expression(const parse_tree& src)
{
	return		(robust_token_is_char<'/'>(src.index_tokens[0].token) || robust_token_is_char<'%'>(src.index_tokens[0].token) || robust_token_is_char<'*'>(src.index_tokens[0].token))
			&&	src.index_tokens[0].src_filename
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags);
//			&&	1==src.size<1>() && (PARSE_MULT_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_PM_EXPRESSION & src.data<2>()->flags);
}
#endif

template<char c> static bool is_C99_mult_operator_expression(const parse_tree& src)
{
	return		robust_token_is_char<c>(src.index_tokens[0].token)
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags);
//			&&	1==src.size<1>() && (PARSE_MULT_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_PM_EXPRESSION & src.data<2>()->flags);
}

#define C99_ADD_SUBTYPE_PLUS 1
#define C99_ADD_SUBTYPE_MINUS 2

BOOST_STATIC_ASSERT(C99_UNARY_SUBTYPE_PLUS==C99_ADD_SUBTYPE_PLUS);
BOOST_STATIC_ASSERT(C99_UNARY_SUBTYPE_NEG==C99_ADD_SUBTYPE_MINUS);

#ifndef NDEBUG
static bool is_C99_add_operator_expression(const parse_tree& src)
{
	return		(robust_token_is_char<'+'>(src.index_tokens[0].token) || robust_token_is_char<'-'>(src.index_tokens[0].token))
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags);
//			&&	1==src.size<1>() && (PARSE_ADD_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_MULT_EXPRESSION & src.data<2>()->flags);
}
#endif

template<char c> static bool is_C99_add_operator_expression(const parse_tree& src)
{
	return		robust_token_is_char<c>(src.index_tokens[0].token)
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags);
//			&&	1==src.size<1>() && (PARSE_ADD_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_MULT_EXPRESSION & src.data<2>()->flags);
}

#define C99_SHIFT_SUBTYPE_LEFT 1
#define C99_SHIFT_SUBTYPE_RIGHT 2
static bool is_C99_shift_expression(const parse_tree& src)
{
	return		(robust_token_is_string<2>(src.index_tokens[0].token,"<<") || robust_token_is_string<2>(src.index_tokens[0].token,">>"))
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags);
//			&&	1==src.size<1>() && (PARSE_SHIFT_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_ADD_EXPRESSION & src.data<2>()->flags);
}

#define C99_RELATION_SUBTYPE_LT 1
#define C99_RELATION_SUBTYPE_GT 2
#define C99_RELATION_SUBTYPE_LTE 3
#define C99_RELATION_SUBTYPE_GTE 4

static bool is_C99_relation_expression(const parse_tree& src)
{
	return		(robust_token_is_char<'<'>(src.index_tokens[0].token) || robust_token_is_char<'>'>(src.index_tokens[0].token) || robust_token_is_string<2>(src.index_tokens[0].token,"<=") || robust_token_is_string<2>(src.index_tokens[0].token,">="))
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags);
//			&&	1==src.size<1>() && (PARSE_RELATIONAL_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_SHIFT_EXPRESSION & src.data<2>()->flags);
}

#define C99_EQUALITY_SUBTYPE_EQ 1
#define C99_EQUALITY_SUBTYPE_NEQ 2
static bool is_C99_equality_expression(const parse_tree& src)
{
	return		(robust_token_is_string<2>(src.index_tokens[0].token,"==") || robust_token_is_string<2>(src.index_tokens[0].token,"!="))
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags);
//			&&	1==src.size<1>() && (PARSE_EQUALITY_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_RELATIONAL_EXPRESSION & src.data<2>()->flags);
}

static bool is_CPP_equality_expression(const parse_tree& src)
{
	return		(robust_token_is_string<2>(src.index_tokens[0].token,"==") || robust_token_is_string<2>(src.index_tokens[0].token,"!=") || robust_token_is_string<6>(src.index_tokens[0].token,"not_eq"))
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags);
//			&&	1==src.size<1>() && (PARSE_EQUALITY_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_RELATIONAL_EXPRESSION & src.data<2>()->flags);
}

static bool is_C99_bitwise_AND_expression(const parse_tree& src)
{
	return (	robust_token_is_char<'&'>(src.index_tokens[0].token)
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags));
//			&&	1==src.size<1>() && (PARSE_BITAND_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_EQUALITY_EXPRESSION & src.data<2>()->flags));
}

static bool is_CPP_bitwise_AND_expression(const parse_tree& src)
{
	return (	(robust_token_is_char<'&'>(src.index_tokens[0].token) || robust_token_is_string<6>(src.index_tokens[0].token,"bitand"))
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags));
//			&&	1==src.size<1>() && (PARSE_BITAND_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_EQUALITY_EXPRESSION & src.data<2>()->flags));
}

static bool is_C99_bitwise_XOR_expression(const parse_tree& src)
{
	return (	robust_token_is_char<'^'>(src.index_tokens[0].token)
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags));
//			&&	1==src.size<1>() && (PARSE_BITXOR_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_BITAND_EXPRESSION & src.data<2>()->flags));
}

static bool is_CPP_bitwise_XOR_expression(const parse_tree& src)
{
	return (	(robust_token_is_char<'^'>(src.index_tokens[0].token) || robust_token_is_string<3>(src.index_tokens[0].token,"xor"))
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags));
//			&&	1==src.size<1>() && (PARSE_BITXOR_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_BITAND_EXPRESSION & src.data<2>()->flags));
}

static bool is_C99_bitwise_OR_expression(const parse_tree& src)
{
	return (	robust_token_is_char<'|'>(src.index_tokens[0].token)
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags));
//			&&	1==src.size<1>() && (PARSE_BITOR_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_BITXOR_EXPRESSION & src.data<2>()->flags));
}

static bool is_CPP_bitwise_OR_expression(const parse_tree& src)
{
	return (	(robust_token_is_char<'|'>(src.index_tokens[0].token) || robust_token_is_string<5>(src.index_tokens[0].token,"bitor"))
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags));
//			&&	1==src.size<1>() && (PARSE_BITOR_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_BITXOR_EXPRESSION & src.data<2>()->flags));
}

static bool is_C99_logical_AND_expression(const parse_tree& src)
{
	return (	robust_token_is_string<2>(src.index_tokens[0].token,"&&")
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags));
//			&&	1==src.size<1>() && (PARSE_LOGICAND_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_BITOR_EXPRESSION & src.data<2>()->flags));
}

static bool is_CPP_logical_AND_expression(const parse_tree& src)
{
	return (	(robust_token_is_string<2>(src.index_tokens[0].token,"&&") || robust_token_is_string<3>(src.index_tokens[0].token,"and"))
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags));
//			&&	1==src.size<1>() && (PARSE_LOGICAND_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_BITOR_EXPRESSION & src.data<2>()->flags));
}

static bool is_C99_logical_OR_expression(const parse_tree& src)
{
	return (	robust_token_is_string<2>(src.index_tokens[0].token,"||")
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags));
//			&&	1==src.size<1>() && (PARSE_LOGICOR_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_LOGICAND_EXPRESSION & src.data<2>()->flags));
}

static bool is_CPP_logical_OR_expression(const parse_tree& src)
{
	return (	(robust_token_is_string<2>(src.index_tokens[0].token,"||") || robust_token_is_string<2>(src.index_tokens[0].token,"or"))
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename
#endif
			&&	!src.index_tokens[1].token.first
			&&	src.empty<0>()
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<1>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags));
//			&&	1==src.size<1>() && (PARSE_LOGICOR_EXPRESSION & src.data<1>()->flags)
//			&&	1==src.size<2>() && (PARSE_LOGICAND_EXPRESSION & src.data<2>()->flags));
}

#ifndef NDEBUG
static bool is_C99_conditional_operator_expression_strict(const parse_tree& src)
{
	return		robust_token_is_char<'?'>(src.index_tokens[0].token)
			&&	robust_token_is_char<':'>(src.index_tokens[1].token)
			&&	src.index_tokens[0].src_filename && src.index_tokens[1].src_filename
			&&	1==src.size<0>() && (PARSE_EXPRESSION & src.data<2>()->flags)
			&&	1==src.size<1>() && (PARSE_LOGICOR_EXPRESSION & src.data<2>()->flags)
			&&	1==src.size<2>() && (PARSE_CONDITIONAL_EXPRESSION & src.data<2>()->flags);		
}
#endif

static bool is_C99_conditional_operator_expression(const parse_tree& src)
{
	return		robust_token_is_char<'?'>(src.index_tokens[0].token)
			&&	robust_token_is_char<':'>(src.index_tokens[1].token)
#ifndef NDEBUG
			&&	src.index_tokens[0].src_filename && src.index_tokens[1].src_filename
#endif
			&&	1==src.size<0>() && (PARSE_EXPRESSION & src.data<2>()->flags)
			&&	1==src.size<1>() && (PARSE_EXPRESSION & src.data<2>()->flags)
			&&	1==src.size<2>() && (PARSE_EXPRESSION & src.data<2>()->flags);
}

bool convert_to(umaxint& dest,const C_PPIntCore& src)
{
	assert(8==src.radix || 10==src.radix || 16==src.radix);
	assert(src.ptr && 0<src.digit_span);

	const unsigned_var_int alt_radix(src.radix,unsigned_var_int::bytes_from_bits(VM_MAX_BIT_PLATFORM));
	unsigned_var_int strict_ub(0,unsigned_var_int::bytes_from_bits(VM_MAX_BIT_PLATFORM));
	const char* target = src.ptr;
	size_t target_len = src.digit_span;

	strict_ub.set_max();
	strict_ub /= alt_radix;

	// internally zero could be generated as a decimal rather than an octal integer literal
	if (1<target_len)
		{	// Duff's device
		switch(src.radix)
		{
		case 16:	++target;	if (0== --target_len) return false;	// leading 0x
		case 8:		++target;	if (0== --target_len) return false;	// leading 0
		}
		}

	const char* const end = target+target_len;
	dest.set_bitcount(VM_MAX_BIT_PLATFORM);
	dest.clear();
	dest += InterpretHexadecimalDigit(*target);
	while(++target!=end)
		{
		if (dest>strict_ub) return false;
		dest *= alt_radix;
		dest += InterpretHexadecimalDigit(*target);
		};
	return true;
}

// forward-declare to handle recursion
static bool C99_intlike_literal_to_VM(umaxint& dest, const parse_tree& src SIG_CONST_TYPES);

static bool _C99_intlike_literal_to_VM(umaxint& dest, const parse_tree& src SIG_CONST_TYPES)
{
	assert(C_TYPE::INTEGERLIKE!=src.type_code.base_type_index);

	if (	virtual_machine::twos_complement==target_machine->C_signed_int_representation()
		&&  !bool_options[boolopt::int_traps]
		&&	is_C99_add_operator_expression<'-'>(src))
		{
		const promote_aux old(src.type_code.base_type_index ARG_TYPES);
		if (old.is_signed)
			{
			const promote_aux lhs(src.data<1>()->type_code.base_type_index ARG_TYPES);
			assert(old.bitcount>=lhs.bitcount);
			if (lhs.is_signed)
				{
				umaxint lhs_int;
				umaxint rhs_int;
				if (	C99_intlike_literal_to_VM(lhs_int,*src.data<1>() ARG_TYPES)
					&&	C99_intlike_literal_to_VM(rhs_int,*src.data<2>() ARG_TYPES))
					{
					const promote_aux rhs(src.data<2>()->type_code.base_type_index ARG_TYPES);
					assert(old.bitcount>=rhs.bitcount);
					assert(old.bitcount>rhs.bitcount || rhs.is_signed);
					if (lhs_int.test(lhs.bitcount-1) && (!rhs.is_signed || !rhs_int.test(rhs.bitcount-1)))
						{	// lhs -, rhs +: could land exactly on INT_MIN/LONG_MIN/LLONG_MIN
						target_machine->signed_additive_inverse(lhs_int,lhs.machine_type);
						lhs_int += rhs_int;
						lhs_int -= 1;
						if (lhs_int==target_machine->signed_max(old.machine_type))
							{
							lhs_int += 1;
							dest = lhs_int;
							return true;	// signed additive inverse on twos-complement nontrapping machines has INT_MIN/LONG_MIN/LLONG_MIN as a fixed point
							}
						}
					}
				}
			}
		}

	if (!src.is_atomic() || !(PARSE_PRIMARY_EXPRESSION & src.flags))
		return false;

	if (C_TESTFLAG_CHAR_LITERAL & src.index_tokens[0].flags)
		{
		dest = EvalCharacterLiteral(src.index_tokens[0].token.first,src.index_tokens[0].token.second);
		dest.set_bitcount(VM_MAX_BIT_PLATFORM);
		return true;
		}	
		
	if (!(C_TESTFLAG_INTEGER & src.index_tokens[0].flags)) return false;
	C_PPIntCore tmp;
	ZAIMONI_PASSTHROUGH_ASSERT(C_PPIntCore::is(src.index_tokens[0].token.first,src.index_tokens[0].token.second,tmp));
	ZAIMONI_PASSTHROUGH_ASSERT(convert_to(dest,tmp));
	return true;
}

static bool _CPP_intlike_literal_to_VM(umaxint& dest, const parse_tree& src)
{	//! \todo: similar code for handling LLONG_MIN as above.  Need that only for zcc; can't test in preprocessor as the true reserved word won't make it this far.
	if (!src.is_atomic()) return false;
	// intercept true, false
	if 		(token_is_string<4>(src.index_tokens[0].token,"true"))
		{
		dest.set_bitcount(VM_MAX_BIT_PLATFORM);
		dest = 1;
		return true;
		}
	else if (token_is_string<5>(src.index_tokens[0].token,"false"))
		{
		dest.set_bitcount(VM_MAX_BIT_PLATFORM);
		dest.clear();
		return true;
		};
	return false;
}

// return value: literal to parse, whether additive inverse applies
static POD_pair<const parse_tree*,bool>
_find_intlike_literal(const parse_tree* src SIG_CONST_TYPES)
{
	assert(src);
	POD_pair<const parse_tree*,bool> ret = {src,false};
	while(converts_to_integer(ret.first->type_code ARG_TYPES))
		{
		if 		(is_C99_unary_operator_expression<'-'>(*ret.first))
			{
			ret.second = !ret.second;
			ret.first = ret.first->data<2>();
			assert(ret.first);
			}
		else if (is_C99_unary_operator_expression<'+'>(*ret.first))
			{
			ret.first = ret.first->data<2>();
			assert(ret.first);
			}
		else
			break;
		};
	return ret;
}

// use this typedef to cope with signature varying by build
typedef bool (intlike_literal_to_VM_func)(umaxint& dest, const parse_tree& src SIG_CONST_TYPES);

static bool C99_intlike_literal_to_VM(umaxint& dest, const parse_tree& src SIG_CONST_TYPES)
{
	const POD_pair<const parse_tree*,bool> actual = _find_intlike_literal(&src ARG_TYPES);

	if (C_TYPE::INTEGERLIKE==actual.first->type_code.base_type_index)
		return false;	

	if (!_C99_intlike_literal_to_VM(dest,*actual.first ARG_TYPES)) return false;
	if (actual.second)
		{
		const promote_aux old(src.type_code.base_type_index ARG_TYPES);
		if (old.is_signed)
			target_machine->signed_additive_inverse(dest,old.machine_type);
		else
			target_machine->unsigned_additive_inverse(dest,old.machine_type);
		};
	return true;
}

static bool CPP_intlike_literal_to_VM(umaxint& dest, const parse_tree& src SIG_CONST_TYPES)
{
	const POD_pair<const parse_tree*,bool> actual = _find_intlike_literal(&src ARG_TYPES);

	if (!_CPP_intlike_literal_to_VM(dest,*actual.first))
		{
		if (C_TYPE::INTEGERLIKE==actual.first->type_code.base_type_index)
			return false;	

		if (!_C99_intlike_literal_to_VM(dest,*actual.first ARG_TYPES)) return false;
		};
	if (actual.second)
		{
		const promote_aux old(src.type_code.base_type_index ARG_TYPES);
		if (old.is_signed)
			target_machine->signed_additive_inverse(dest,old.machine_type);
		else
			target_machine->unsigned_additive_inverse(dest,old.machine_type);
		};
	return true;
}

/** decides whether the given entry is a null pointer constant
 * \return 1 : yes, null pointer constant 
 * \return 0 : no, not null pointer constant 
 * \return -1 : can't decide quickly whether this is a null 
 *         pointer constant
 */
static int is_null_pointer_constant(const parse_tree& src,intlike_literal_to_VM_func& intlike_literal_to_VM SIG_CONST_TYPES)
{
	if (!converts_to_integerlike(src.type_code ARG_TYPES)) return 0;
	umaxint tmp;
	if (intlike_literal_to_VM(tmp,src ARG_TYPES)) return tmp==0;
	return -1;
}

static void _label_one_literal(parse_tree& src,const type_system& types)
{
	assert(src.is_atomic());
	if ((C_TESTFLAG_CHAR_LITERAL | C_TESTFLAG_STRING_LITERAL | C_TESTFLAG_PP_NUMERAL) & src.index_tokens[0].flags)
		{
		src.flags |= (PARSE_PRIMARY_EXPRESSION | parse_tree::CONSTANT_EXPRESSION);
		if (C_TESTFLAG_STRING_LITERAL==src.index_tokens[0].flags)
			{
			src.type_code.set_type(C_TYPE::CHAR);
			src.type_code.make_C_array(LengthOfCStringLiteral(src.index_tokens[0].token.first,src.index_tokens[0].token.second));
			return;
			}
		else if (C_TESTFLAG_CHAR_LITERAL==src.index_tokens[0].flags)
			{	// C99 6.4.4.4p10: the type of a character literal is int
				// C++0X 2.13.4p1: the type of a single character literal is char 
				// but the type of a multiple-character literal is int
				// unfortunately, we don't have the language here.  Go with
				// C++ rules, and patch up C elsewhere
			src.type_code.set_type(LengthOfCCharLiteral(src.index_tokens[0].token.first,src.index_tokens[0].token.second) ? C_TYPE::CHAR : C_TYPE::INT);
			return;
			};
		assert(C_TESTFLAG_PP_NUMERAL & src.index_tokens[0].flags);
		C_REALITY_CHECK_PP_NUMERAL_FLAGS(src.index_tokens[0].flags);
		if (C_TESTFLAG_INTEGER & src.index_tokens[0].flags)
			{
			src.type_code.set_type(C_TYPE::INTEGERLIKE);
			C_PPIntCore parse_tmp;
			ZAIMONI_PASSTHROUGH_ASSERT(C_PPIntCore::is(src.index_tokens[0].token.first,src.index_tokens[0].token.second,parse_tmp));
			umaxint tmp;
			const unsigned char type_hint = parse_tmp.hinted_type;
			const bool no_signed = 1==type_hint%2;
			const bool no_unsigned = !no_signed && 10==parse_tmp.radix;
			if (convert_to(tmp,parse_tmp))
				{	// failover to IntegerLike if won't convert
				size_t i = 0;
				do	switch(types.int_priority[i])
					{
					case C_TYPE::INT:
						if (no_signed || C_PPIntCore::L<=type_hint) continue;
						if (tmp>target_machine->signed_max<virtual_machine::std_int_int>()) continue;
						src.type_code.base_type_index = C_TYPE::INT;
						return;
					case C_TYPE::UINT:
						if (no_unsigned || C_PPIntCore::L<=type_hint) continue;
						if (tmp>target_machine->unsigned_max<virtual_machine::std_int_int>()) continue;
						src.type_code.base_type_index = C_TYPE::UINT;
						return;
					case C_TYPE::LONG:
						if (no_signed || C_PPIntCore::LL<=type_hint) continue;
						if (tmp>target_machine->signed_max<virtual_machine::std_int_long>()) continue;
						src.type_code.base_type_index = C_TYPE::LONG;
						return;
					case C_TYPE::ULONG:
						if (no_unsigned || C_PPIntCore::LL<=type_hint) continue;
						if (tmp>target_machine->unsigned_max<virtual_machine::std_int_long>()) continue;
						src.type_code.base_type_index = C_TYPE::ULONG;
						return;
					case C_TYPE::LLONG:
						if (no_signed) continue;
						if (tmp>target_machine->signed_max<virtual_machine::std_int_long_long>()) continue;
						src.type_code.base_type_index = C_TYPE::LLONG;
						return;
					case C_TYPE::ULLONG:
						if (no_unsigned) continue;
						if (tmp>target_machine->unsigned_max<virtual_machine::std_int_long_long>()) continue;
						src.type_code.base_type_index = C_TYPE::ULLONG;
						return;
					}
				while(types.int_priority_size > ++i);
				};
			assert(C_TYPE::INTEGERLIKE==src.type_code.base_type_index);
			// integer literal has no useful type to represent it
			//! \test if.C99/Error_control22.hpp, if.C99/Error_control22.h
			//! \test if.C99/Error_control23.hpp, if.C99/Error_control23.h
			src.flags |= parse_tree::INVALID;
			message_header(src.index_tokens[0]);
			INC_INFORM(ERR_STR);
			INC_INFORM(src);
			INC_INFORM(" cannot be represented as ");
			INC_INFORM(no_unsigned ? "signed long long" : "unsigned long long");
			INFORM(" (C99 6.4.4.1p5/C++0x 2.13.1p3)");
			zcc_errors.inc_error();
			}
		else{	//! \todo --do-what-i-mean should check for floating-point numerals that convert exactly to integers
			src.type_code.set_type(	(C_TESTFLAG_L & src.index_tokens[0].flags) ? C_TYPE::LDOUBLE : 
									(C_TESTFLAG_F & src.index_tokens[0].flags) ? C_TYPE::FLOAT : C_TYPE::DOUBLE);
			}
		}
}

void C99_literal_is_legal(const char* const x,const size_t x_len,const lex_flags flags,const char* src_filename,size_t lineno,const type_system& types)
{
	parse_tree tmp;
	tmp.clear();
	tmp.index_tokens[0].token.first = x;
	tmp.index_tokens[0].token.second = x_len;
	tmp.index_tokens[0].flags = flags;
	tmp.index_tokens[0].src_filename = src_filename;
	tmp.index_tokens[0].logical_line.first = lineno;
	_label_one_literal(tmp,types);
}

// C and C++ agree that literals are constant primary expressions
// note that on some huge-char platforms not all strings can be concatenated safely in C
// we almost certainly have have memory problems if there are non-concatenated strings around
// a psuedo-concatenated string literal has string literals for both of its indexes
static void _label_literals(parse_tree& src,const type_system& types)
{
	std::pair<size_t,size_t> str_span(SIZE_MAX,SIZE_MAX);
	size_t str_count = 0;
	size_t i = src.size<0>();
	while(0<i)
		{
		if (!src.data<0>()[--i].is_atomic()) continue;
		_label_one_literal(src.c_array<0>()[i],types);
		if (C_TESTFLAG_STRING_LITERAL==src.data<0>()[i].index_tokens[0].flags)
			{
			if (SIZE_MAX==str_span.first) str_span.second = i;
			str_span.first = i;
			++str_count;
			}
		};
	while((assert(str_count<=(str_span.second-str_span.first)+1),2<=str_count) && (2<str_count || 1==str_span.second-str_span.first))
		{
		bool want_first_slideup = false;
		bool want_second_slidedown = false;
		bool RAMfail = false;
		if (C_TESTFLAG_STRING_LITERAL==src.data<0>()[str_span.first+1].index_tokens[0].flags)
			{
			if (src.size<0>()<=str_span.second+2 || C_TESTFLAG_STRING_LITERAL!=src.data<0>()[str_span.first+2].index_tokens[0].flags)
				{	// psuedo-concatenate
					// that this is still a constant primary expression, as we are just pretending that the string concatenation went through
				src.c_array<0>()[str_span.first].grab_index_token_from<1,0>(src.c_array<0>()[str_span.first+1]);
				src.DeleteIdx<0>(str_span.first+1);
				if (1>=(str_count -= 2)) break;
				str_span.first += 2;
				want_first_slideup = true;
				}
			else{	// more than two strings to psuedo-concatenate
				POD_pair<size_t,size_t> scan = {str_span.first,str_span.first+2};
				while(src.size<0>()>scan.second+1 && C_TESTFLAG_STRING_LITERAL==src.data<0>()[scan.second+1].index_tokens[0].flags) ++scan.second;
				if (parse_tree::collapse_matched_pair(src,scan))
					src.c_array<0>()[scan.first].flags |= (PARSE_PRIMARY_EXPRESSION | parse_tree::CONSTANT_EXPRESSION);
				else
					RAMfail = true;
				}
			}
		else{
			want_first_slideup = true;
			++str_span.first;
			--str_count;
			};
		if (C_TESTFLAG_STRING_LITERAL==src.data<0>()[str_span.second-1].index_tokens[0].flags)
			{
			if (2<=str_span.second || C_TESTFLAG_STRING_LITERAL!=src.data<0>()[str_span.second-2].index_tokens[0].flags)
				{	// psuedo-concatenate
					// this is still a constant primary expression, as we are just pretending that the string concatenation went through
				src.c_array<0>()[str_span.second-1].grab_index_token_from<1,0>(src.c_array<0>()[str_span.second]);
				src.DeleteIdx<0>(str_span.second);
				if (1>=(str_count -= 2)) break;
				str_span.second -= 2;
				want_second_slidedown = true;
				}
			else{	// more than two strings to psuedo-concatenate
				POD_pair<size_t,size_t> scan = {str_span.second-2,str_span.second};
				while(0<scan.first && C_TESTFLAG_STRING_LITERAL==src.data<0>()[scan.first-1].index_tokens[0].flags) --scan.first;
				if (parse_tree::collapse_matched_pair(src,scan))
					src.c_array<0>()[scan.first].flags |= (PARSE_PRIMARY_EXPRESSION | parse_tree::CONSTANT_EXPRESSION);
					// note: as current item was already typed properly, do not need to update
				else
					RAMfail = true;
				}
			}
		else{
			if (1>=(--str_count)) break;
			want_second_slidedown = true;
			--str_span.second;
			}

		if (want_first_slideup)
			{
			while(C_TESTFLAG_STRING_LITERAL!=src.data<0>()[str_span.first].index_tokens[0].flags)
				{
				++str_span.first;
				assert(str_span.second > str_span.first);
				};
			RAMfail = false;
			}
		if (want_second_slidedown)
			{
			while(C_TESTFLAG_STRING_LITERAL!=src.data<0>()[str_span.second].index_tokens[0].flags)
				{
				--str_span.second;
				assert(str_span.first < str_span.second);
				};
			RAMfail = false;
			}		
		if (RAMfail) throw std::bad_alloc();	// couldn't do anything at all: stalled
		}
}


// this handles: ( ), [ ], { }
// the content of ( ), [ ], { } fills the zeroth argument array
// C++ *sometimes* wants to match < > as well, but its approaches are...painful.  Do that elsewhere
// returns true if successful
static bool _match_pairs(parse_tree& src)
{
	assert(!src.empty<0>());
	POD_pair<size_t,size_t> depth_parens = balanced_character_count<'(',')'>(src.data<0>(),src.size<0>());	// pre-scan
	POD_pair<size_t,size_t> depth_brackets = balanced_character_count<'[',']'>(src.data<0>(),src.size<0>());	// pre-scan
	POD_pair<size_t,size_t> depth_braces = balanced_character_count<'{','}'>(src.data<0>(),src.size<0>());	// pre-scan
	assert(depth_parens.first==depth_parens.second);
	assert(depth_brackets.first==depth_brackets.second);
	assert(depth_braces.first==depth_braces.second);
	if (0==depth_parens.first && 0==depth_brackets.first && 0==depth_braces.first) return true;
	autovalarray_ptr_throws<size_t> paren_stack(depth_parens.first);
	autovalarray_ptr_throws<size_t> bracket_stack(depth_brackets.first);
	autovalarray_ptr_throws<size_t> brace_stack(depth_braces.first);

	const size_t starting_errors = zcc_errors.err_count();
	size_t paren_idx = 0;
	size_t bracket_idx = 0;
	size_t brace_idx = 0;
	size_t i = 0;
	//! \todo optimize this loop
	do	{
		if (src.data<0>()[i].index_tokens[1].token.first) continue;
		if 		(token_is_char<')'>(src.data<0>()[i].index_tokens[0].token))
			{
			assert(0<paren_idx);
			assert(0==bracket_idx || bracket_stack[bracket_idx-1]<paren_stack[paren_idx-1]);
			assert(0==brace_idx || brace_stack[brace_idx-1]<paren_stack[paren_idx-1]);
			const POD_pair<size_t,size_t> target = {paren_stack[--paren_idx],i};
			if (!parse_tree::collapse_matched_pair(src,target)) throw std::bad_alloc();
			i = paren_stack[paren_idx];
			// do not suppress inner parentheses here, this only works for known expressions
			if (0==paren_idx && 1<src.size<0>()-i)
				{
				depth_parens = balanced_character_count<'(',')'>(src.data<0>()+i+1,src.size<0>()-(i+1));
				assert(depth_parens.first==depth_parens.second);
				if (0==depth_parens.first && 0==depth_brackets.first && 0==depth_braces.first) return starting_errors==zcc_errors.err_count();
				paren_stack.Shrink(depth_parens.first);
				}
			}
		else if (token_is_char<']'>(src.data<0>()[i].index_tokens[0].token))
			{
			assert(0<bracket_idx);
			assert(0==paren_idx || paren_stack[paren_idx-1]<bracket_stack[bracket_idx-1]);
			assert(0==brace_idx || brace_stack[brace_idx-1]<bracket_stack[bracket_idx-1]);
			const POD_pair<size_t,size_t> target = {bracket_stack[--bracket_idx],i};
			if (!parse_tree::collapse_matched_pair(src,target)) throw std::bad_alloc();
			i = bracket_stack[bracket_idx];
			// do not suppress inner parentheses here, this only works for known expressions
			if (0==bracket_idx && 1<src.size<0>()-i)
				{
				depth_brackets = balanced_character_count<'[',']'>(src.data<0>()+i+1,src.size<0>()-(i+1));
				assert(depth_brackets.first==depth_brackets.second);
				if (0==depth_parens.first && 0==depth_brackets.first && 0==depth_braces.first) return starting_errors==zcc_errors.err_count();
				bracket_stack.Shrink(depth_brackets.first);
				}
			}
		else if (token_is_char<'}'>(src.data<0>()[i].index_tokens[0].token))
			{
			assert(0<brace_idx);
			assert(0==paren_idx || paren_stack[paren_idx-1]<brace_stack[brace_idx-1]);
			assert(0==bracket_idx || bracket_stack[bracket_idx-1]<brace_stack[brace_idx-1]);
			const POD_pair<size_t,size_t> target = {brace_stack[--brace_idx],i};
			if (!parse_tree::collapse_matched_pair(src,target)) throw std::bad_alloc();
			i = brace_stack[brace_idx];
			if (0==brace_idx && 1<src.size<0>()-i)
				{
				depth_braces = balanced_character_count<'{','}'>(src.data<0>()+i+1,src.size<0>()-(i+1));
				assert(depth_braces.first==depth_braces.second);
				if (0==depth_parens.first && 0==depth_brackets.first && 0==depth_braces.first) return starting_errors==zcc_errors.err_count();
				brace_stack.Shrink(depth_braces.first);
				}
			}
		else if (token_is_char<'('>(src.data<0>()[i].index_tokens[0].token))
			{
			assert(paren_stack.size()>paren_idx);
			paren_stack[paren_idx++] = i;
			}
		else if (token_is_char<'['>(src.data<0>()[i].index_tokens[0].token))
			{
			assert(bracket_stack.size()>bracket_idx);
			bracket_stack[bracket_idx++] = i;
			}
		else if (token_is_char<'{'>(src.data<0>()[i].index_tokens[0].token))
			{
			assert(brace_stack.size()>brace_idx);
			brace_stack[brace_idx++] = i;
			}
		// introduces sequence points; this causes errors if caught in brackets or parentheses
		// cannot test within preprocessor expression (trigger is intercepted earlier)
		else if (token_is_char<';'>(src.data<0>()[i].index_tokens[0].token))
			{
			if (0<paren_idx || 0<bracket_idx)
				{
				const size_t nearest_break = (paren_idx<bracket_idx) ? bracket_idx : paren_idx;
				message_header(src.data<0>()[i].index_tokens[0]);
				INC_INFORM(ERR_STR);
				INC_INFORM(" ; breaks intended balancing of ");
				INFORM(src.data<0>()[nearest_break].index_tokens[0].token.first[0]);
				zcc_errors.inc_error();
				}
			};
		}
	while(src.size<0>() > ++i);
	return starting_errors==zcc_errors.err_count();
}

static bool C99_literal_converts_to_integer(const parse_tree& src)
{
	if (!src.is_atomic()) return false;
	return ((C_TESTFLAG_CHAR_LITERAL | C_TESTFLAG_INTEGER) & src.index_tokens[0].flags);
	//! \todo --do-what-i-mean should try to identify floats that are really integers
}

static bool CPP_literal_converts_to_integer(const parse_tree& src)
{
	if (!src.is_atomic()) return false;
	if (token_is_string<4>(src.index_tokens[0].token,"true") || token_is_string<5>(src.index_tokens[0].token,"false")) return true;
	return ((C_TESTFLAG_CHAR_LITERAL | C_TESTFLAG_INTEGER) & src.index_tokens[0].flags);
	//! \todo --do-what-i-mean should try to identify floats that are really integers
}

//! the returned parse_tree struct has no content; safe to use OvewriteInto on
//! \throw std::bad_alloc()
static zaimoni::Loki::CheckReturnDisallow<NULL,parse_tree*>::value_type repurpose_inner_parentheses(parse_tree& src)
{
	if (1==src.size<0>() && is_naked_parentheses_pair(*src.data<0>()))
		{
		zaimoni::POD_autoarray_ptr<parse_tree> tmp = src.c_array<0>()->_args[0];
		src.c_array<0>()->_args[0].NULLPtr();
		src.c_array<0>()->destroy();
		parse_tree* const tmp2 = src.c_array<0>();
		src._args[0] = tmp;
		return tmp2;
		};
	return _new_buffer_nonNULL_throws<parse_tree>(1);
}

static void cancel_inner_parentheses(parse_tree& src)
{
	while(1==src.size<0>() && is_naked_parentheses_pair(*src.data<0>()))
		{
		zaimoni::POD_autoarray_ptr<parse_tree> tmp = src.c_array<0>()->_args[0];
		src.c_array<0>()->_args[0].NULLPtr();
		src.c_array<0>()->destroy();
		free(src.c_array<0>());
		src._args[0] = tmp;
		}
}

static void cancel_outermost_parentheses(parse_tree& src)
{
	while(1==src.size<0>() && is_naked_parentheses_pair(src))
		src.eval_to_arg<0>(0);
}

/*! 
 * determines whether a context-driven primary expression is obviously one
 * 
 * \param src target to inspect
 * 
 * \return true iff ( ... ) expression was recognized
 * \throw std::bad_alloc only if 1==src.size<0>() and src.type_code.pointer_power<src.data<0>()->type_code.pointer_power
 */
static bool inspect_potential_paren_primary_expression(parse_tree& src)
{
	if (!(PARSE_OBVIOUS & src.flags) && is_naked_parentheses_pair(src))
		{	// we're a naked parentheses pair
		cancel_inner_parentheses(src);
		const size_t content_length = src.size<0>();
		if (0==content_length)
			{	// ahem...invalid
				// untestable as a preprocessor expression, balanced-kill gets this first
			src.flags &= parse_tree::RESERVED_MASK;	// just in case
			src.flags |= (PARSE_PRIMARY_EXPRESSION | parse_tree::CONSTANT_EXPRESSION);
			simple_error(src," cannot be an expression (C99 6.5.2p1/C++98 5.2p1)");
			return true;
			};
		if (1==content_length)
			{
			if (PARSE_PRIMARY_EXPRESSION & src.data<0>()->flags)
				{	// primary expression that got parenthesized
				src.eval_to_arg<0>(0);
				return true;
				}
			else if (PARSE_EXPRESSION & src.data<0>()->flags)
				{	// normal expression that got parenthesized
				value_copy(src.type_code,src.data<0>()->type_code);
				src.flags &= parse_tree::RESERVED_MASK;	// just in case
				src.flags |= PARSE_PRIMARY_EXPRESSION;
				src.flags |= (PARSE_PAREN_PRIMARY_PASSTHROUGH & src.data<0>()->flags);
				return true;
				}
			};
		}
	return false;
}

static bool suppress_naked_brackets_and_braces(parse_tree& src,const char* const err_prefix,size_t err_len)
{
	if (!(PARSE_OBVIOUS & src.flags) && src.empty<1>() && src.empty<2>())
		{	// top-level [ ] dies regardless of contents
		// not testable with preprocessor expression (not sure whether reachable with full source code)
		if 		(robust_token_is_char<'['>(src.index_tokens[0].token))
			{
			if (robust_token_is_char<']'>(src.index_tokens[1].token))
				{
				src.flags &= parse_tree::RESERVED_MASK;	// just in case
				src.flags |= parse_tree::INVALID;
				message_header(src.index_tokens[0]);
				INC_INFORM(ERR_STR);
				INC_INFORM(err_prefix,err_len);
				INFORM(" [ ... ] has no context to interpret (C99 6.5.2p1/C++98 5.2p1)");
				zcc_errors.inc_error();
				return true;
				}
			}
		// top-level { } dies regardless of contents
		// not testable with preprocessor expression
		else if	(robust_token_is_char<'{'>(src.index_tokens[0].token))
			{
			if (robust_token_is_char<'}'>(src.index_tokens[1].token))
				{
				src.flags &= parse_tree::RESERVED_MASK;	// just in case
				src.flags |= parse_tree::INVALID;
				message_header(src.index_tokens[0]);
				INC_INFORM(ERR_STR);
				INC_INFORM(err_prefix,err_len);
				INFORM(" { ... } has no context to interpret (C99 6.5.2p1/C++98 5.2p1)");
				zcc_errors.inc_error();
				return true;
				}
			}
		}
	return false;
}

// \throw std::bad_alloc only if src.data<0>()[i-1].type_code.pointer_power<src.data<0>()[i-1].data<0>()->type_code.pointer_power
static bool terse_locate_array_deref(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(src.data<0>()[i].empty<1>());
	assert(src.data<0>()[i].empty<2>());
	assert(src.data<0>()[i].index_tokens[0].token.first);
	assert(src.data<0>()[i].index_tokens[1].token.first);

	if (	!token_is_char<'['>(src.data<0>()[i].index_tokens[0].token)
		|| 	!token_is_char<']'>(src.data<0>()[i].index_tokens[1].token))
		return false;

	assert(1<=i);
	if (   1==src.data<0>()[i].size<0>()
		&& (PARSE_EXPRESSION & src.data<0>()[i].data<0>()->flags))
		{	// array dereference operator; put preceding argument src.data<0>()[i-1] in src.data<0>()[i].data<1>()[0]
		inspect_potential_paren_primary_expression(src.c_array<0>()[i-1]);
		if (PARSE_POSTFIX_EXPRESSION & src.data<0>()[i-1].flags)
			{
			parse_tree* const tmp = repurpose_inner_parentheses(src.c_array<0>()[i]);	// RAM conservation
			src.c_array<0>()[i-1].OverwriteInto(*tmp);
			src.c_array<0>()[i].fast_set_arg<1>(tmp);
			src.c_array<0>()[i].core_flag_update();
			src.c_array<0>()[i].flags |= PARSE_STRICT_POSTFIX_EXPRESSION;
			src.DeleteIdx<0>(--i);
			assert(is_array_deref_strict(src.data<0>()[i]));
			cancel_outermost_parentheses(src.c_array<0>()[i].c_array<1>()[0]);
			cancel_outermost_parentheses(src.c_array<0>()[i].c_array<0>()[0]);
			src.type_code.set_type(C_TYPE::NOT_VOID);
			src.c_array<0>()[i].type_code.q_vector.front() |= type_spec::lvalue;
			assert(is_array_deref(src.data<0>()[i]));
			return true;
			};
		if (!(parse_tree::INVALID & src.flags))
			{	//! \test default/Error_if_control3.hpp, default/Error_if_control3.h
				//! \test default/Error_if_control4.hpp, default/Error_if_control4.h
				//! \test default/Error_if_control5.hpp, default/Error_if_control5.h
				//! \test default/Error_if_control6.hpp, default/Error_if_control6.h
				//! \test default/Error_if_control7.hpp, default/Error_if_control7.h
				//! \test default/Error_if_control8.hpp, default/Error_if_control8.h
				//! \test default/Error_if_control9.hpp, default/Error_if_control9.h
				//! \test default/Error_if_control10.hpp, default/Error_if_control10.h
				//! \test default/Error_if_control11.hpp, default/Error_if_control11.h
				//! \test default/Error_if_control12.hpp, default/Error_if_control12.h
				//! \test default/Error_if_control13.hpp, default/Error_if_control13.h
				//! \test default/Error_if_control14.hpp, default/Error_if_control14.h
				//! \test default/Error_if_control15.hpp, default/Error_if_control15.h
				//! \test default/Error_if_control16.hpp, default/Error_if_control16.h
				//! \test default/Error_if_control17.hpp, default/Error_if_control17.h
				//! \test default/Error_if_control18.hpp, default/Error_if_control18.h
				//! \test default/Error_if_control19.hpp, default/Error_if_control19.h
				//! \test default/Error_if_control20.hpp, default/Error_if_control20.h
				//! \test default/Error_if_control21.hpp, default/Error_if_control21.h
				//! \test default/Error_if_control22.hpp, default/Error_if_control22.h
				//! \test default/Error_if_control23.hpp, default/Error_if_control23.h
			src.flags |= parse_tree::INVALID;
			src.c_array<0>()[i].flags |= PARSE_STRICT_POSTFIX_EXPRESSION;
			src.type_code.set_type(0);
			message_header(src.index_tokens[0]);
			INC_INFORM(ERR_STR);
			INC_INFORM("array dereference ");
			INC_INFORM(src);
			INC_INFORM(" has invalid postfix expression ");
			INC_INFORM(src.data<0>()[i-1]);
			INFORM(" to dereference (C99 6.5.2.1p1)");
			zcc_errors.inc_error();
			};
		}
	return false;
}

//! \throw std::bad_alloc
static void C_array_easy_syntax_check(parse_tree& src,const type_system& types)
{
	if (parse_tree::INVALID & src.flags) return;	// cannot optimize to valid

	const size_t effective_pointer_power_prefix = src.data<1>()->type_code.pointer_power;
	const size_t effective_pointer_power_infix = src.data<0>()->type_code.pointer_power;
	if (0<effective_pointer_power_prefix)
		{
		if (0<effective_pointer_power_infix)
			{	// uses extension to test in preprocessor
				//! \test default/Error_if_control1.h
			simple_error(src,"array dereference of pointer by pointer (C99 6.5.2.1p1; C++98 5.2.1p1,13.3.1.2p1)");
			return;
			}
		else if (converts_to_integerlike(src.data<0>()->type_code.base_type_index ARG_TYPES))
			ZAIMONI_PASSTHROUGH_ASSERT(src.data<1>()->type_code.dereference(src.type_code));
		else{	// not testable from preprocessor yet (need floating-point literals as extension)
			src.flags |= parse_tree::INVALID;
			message_header(src.index_tokens[0]);
			INC_INFORM(ERR_STR);
			INC_INFORM("array dereference of pointer by ");
			INFORM(types.name(src.data<0>()->type_code.base_type_index));
			INFORM(" (C99 6.5.2.1p1; C++98 5.2.1p1,13.5.5p1)");
			zcc_errors.inc_error();
			return;
			}
		}
	else if (0<effective_pointer_power_infix)
		{
		if (converts_to_integerlike(src.data<1>()->type_code.base_type_index ARG_TYPES))
			ZAIMONI_PASSTHROUGH_ASSERT(src.data<0>()->type_code.dereference(src.type_code));
		else{	// autofails in C
				// not testable from preprocessor yet (need floating-point literals, would be extension regardless)
			src.flags |= parse_tree::INVALID;
			message_header(src.index_tokens[0]);
			INC_INFORM(ERR_STR);
			INC_INFORM("array dereference of pointer by ");
			INFORM(types.name(src.data<1>()->type_code.base_type_index));
			INFORM(" (C99 6.5.2.1p1; C++98 5.2.1p1,13.5.5p1)");
			zcc_errors.inc_error();
			return;
			}
		}
	else{	// autofails in C; uses extension to test in preprocessor
			//! \test default/Error_if_control2.h
		src.flags |= parse_tree::INVALID;
		if (!(parse_tree::INVALID & src.data<0>()->flags) && !(parse_tree::INVALID & src.data<1>()->flags))
			{
			message_header(src.index_tokens[0]);
			INC_INFORM(ERR_STR);
			INC_INFORM("array dereference of ");
			INC_INFORM(types.name(src.data<1>()->type_code.base_type_index));
			INC_INFORM(" by ");
			INFORM(types.name(src.data<0>()->type_code.base_type_index));
			INFORM(" (C99 6.5.2.1p1; C++98 5.2.1p1,13.5.5p1)");
			zcc_errors.inc_error();
			}
		return;
		}
}

/*(6.5.2) postfix-expression:
	primary-expression
	postfix-expression [ expression ]
	postfix-expression ( argument-expression-listopt )
	postfix-expression . identifier
	postfix-expression -> identifier
	postfix-expression ++
	postfix-expression --
	( type-name ) { initializer-list }
	( type-name ) { initializer-list , }
*/
static void locate_C99_postfix_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].empty<1>()
		|| !src.data<0>()[i].empty<2>()
		|| !src.data<0>()[i].index_tokens[0].token.first) return;
	
	if (src.data<0>()[i].index_tokens[1].token.first)
		{
		if (terse_locate_array_deref(src,i))
			{
			C_array_easy_syntax_check(src.c_array<0>()[i],types);
			return;
			}
#if 0
		else if (   token_is_char<'('>(src.data<0>()[i].index_tokens[0].token)
				 && token_is_char<')'>(src.data<0>()[i].index_tokens[1].token))
			{
			if (1<=i)
				{
				}
			else if (1<src.size<0>()-i)
				{
				}
			}
		}
	else{	// if (NULL==src.data<0>()[i].index_tokens[1].token.first)
		if (token_is_char<'.'>(src.data<0>()[i].index_tokens[0].token))
			{
			if (1<=i && 1<src.size<0>()-i)
				{
				}
			else{
				}
			}
		else if (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"->"))
			{
			if (1<=i && 1<src.size<0>()-i)
				{
				}
			else{
				}
			}
		else if (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"++"))
			{
			if (1<=i)
				{
				}
			}
		else if (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"--"))
			{
			if (1<=i)
				{
				}
			}
#endif
		}
}

/*postfixexpression:
	primaryexpression
	postfixexpression [ expression ]
	postfixexpression ( expressionlistopt )
	simpletypespecifier ( expressionlistopt )
	postfixexpression . templateopt ::opt idexpression
	postfixexpression -> templateopt ::opt idexpression
	postfixexpression . pseudodestructorname
	postfixexpression -> pseudodestructorname
	postfixexpression ++
	postfixexpression --
	dynamic_cast < typeid > ( expression )
	static_cast < typeid > ( expression )
	reinterpret_cast < typeid > ( expression )
	const_cast < typeid > ( expression )
	typeid ( expression )
	typeid ( typeid ) */
/* returns error count */
static void locate_CPP_postfix_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].empty<1>()
		|| !src.data<0>()[i].empty<2>()) return;

	if (src.data<0>()[i].index_tokens[1].token.first)
		{
		if (terse_locate_array_deref(src,i))
			{	//! \todo handle operator [] overloading
			C_array_easy_syntax_check(src.c_array<0>()[i],types);
			return;
			}
#if 0
		else if (   token_is_char<'('>(src.data<0>()[i].index_tokens[0].token)
				 && token_is_char<')'>(src.data<0>()[i].index_tokens[1].token))
			{
			if (1<=i)
				{
				}
			}
		if (token_is_char<'.'>(src.data<0>()[i].index_tokens[0].token))
			{
			if (1<=i && 1<src.size<0>()-i)
				{
				}
			else{
				}
			}
		else if (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"->"))
			{
			if (1<=i && 1<src.size<0>()-i)
				{
				}
			else{
				}
			}
		else if (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"++"))
			{
			if (1<=i)
				{
				}
			}
		else if (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"--"))
			{
			if (1<=i)
				{
				}
			}
		else if (token_is_string<12>(src.data<0>()[i].index_tokens[0].token,"dynamic_cast"))
			{
			}
		else if (token_is_string<11>(src.data<0>()[i].index_tokens[0].token,"static_cast"))
			{
			}
		else if (token_is_string<16>(src.data<0>()[i].index_tokens[0].token,"reinterpret_cast"))
			{
			}
		else if (token_is_string<10>(src.data<0>()[i].index_tokens[0].token,"const_cast"))
			{
			}
		else if (token_is_string<6>(src.data<0>()[i].index_tokens[0].token,"typeid"))
			{
			}
#endif
		}
}

// typedef to simplify compatibility changes
typedef bool literal_converts_to_bool_func(const parse_tree& src, bool& is_true SIG_CONST_TYPES);

// Closely related to if_elif_control_is_zero/CPreproc.cpp
static bool _C99_literal_converts_to_bool(const parse_tree& src, bool& is_true SIG_CONST_TYPES)
{
	assert(src.is_atomic());
	// string literals always test true (decay to non-NULL pointer)
	if (C_TESTFLAG_STRING_LITERAL==src.index_tokens[0].flags)
		{
		is_true = true;
		return true;
		}
	if (C_TESTFLAG_CHAR_LITERAL==src.index_tokens[0].flags)
		{
		is_true = !CCharLiteralIsFalse(src.index_tokens[0].token.first,src.index_tokens[0].token.second);
		return true;
		};
	if (!(C_TESTFLAG_PP_NUMERAL & src.index_tokens[0].flags)) return false;
	C_REALITY_CHECK_PP_NUMERAL_FLAGS(src.index_tokens[0].flags);
	if (C_TESTFLAG_FLOAT & src.index_tokens[0].flags) return false;	//! \todo handle floats as well (underflow to zero is target-sensitive)
	// zeros go to zero, everything else canonicalizes to one
	is_true = !C99_integer_literal_is_zero(src.index_tokens[0].token.first,src.index_tokens[0].token.second,src.index_tokens[0].flags);
	return true;
}

static bool C99_literal_converts_to_bool(const parse_tree& src, bool& is_true SIG_CONST_TYPES)
{	// deal with -1 et. al.
	if (is_C99_unary_operator_expression<'-'>(src) && src.data<2>()->is_atomic()) return _C99_literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES);

	if (!src.is_atomic()) return false;
	return _C99_literal_converts_to_bool(src,is_true ARG_TYPES);
}

static bool CPP_literal_converts_to_bool(const parse_tree& src, bool& is_true SIG_CONST_TYPES)
{
	// deal with -1 et. al.
	if (is_C99_unary_operator_expression<'-'>(src) && src.data<2>()->is_atomic()) return CPP_literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES);

	if (!src.is_atomic()) return false;
	if (_C99_literal_converts_to_bool(src,is_true ARG_TYPES)) return true;
	// deal with: this, true, false
	if (token_is_string<5>(src.index_tokens[0].token,"false"))
		{
		is_true = false;
		return true;
		}
	else if (	token_is_string<4>(src.index_tokens[0].token,"true")
			 ||	token_is_string<4>(src.index_tokens[0].token,"this"))
		{
		is_true = true;
		return true;
		};
	return false;
}

//! \throw std::bad_alloc
static void assemble_unary_postfix_arguments(parse_tree& src, const size_t i, const size_t _subtype)
{
	assert(1<src.size<0>()-i);
	parse_tree* const tmp = repurpose_inner_parentheses(src.c_array<0>()[i+1]);	// RAM conservation
	src.c_array<0>()[i+1].OverwriteInto(*tmp);
	src.c_array<0>()[i].fast_set_arg<2>(tmp);
	src.c_array<0>()[i].core_flag_update();
	src.c_array<0>()[i].flags |= PARSE_STRICT_UNARY_EXPRESSION;
	src.c_array<0>()[i].subtype = _subtype;
	src.DeleteIdx<0>(i+1);
	cancel_outermost_parentheses(src.c_array<0>()[i].c_array<2>()[0]);
}

// can't do much syntax-checking or immediate-evaluation here because of binary +/-
// unary +/- syntax checking out out of place as it's needed by all of the unary operators
//! \throw std::bad_alloc()
static void VM_to_token(const umaxint& src_int,const size_t base_type_index,POD_pair<char*,lex_flags>& dest)
{
	assert(C_TYPE::INT<=base_type_index && C_TYPE::ULLONG>=base_type_index);
	const char* const suffix = literal_suffix(base_type_index);
	char* buf = _new_buffer_nonNULL_throws<char>((VM_MAX_BIT_PLATFORM/3)+4);
	dest.second = literal_flags(base_type_index);
	dest.second |= C_TESTFLAG_DECIMAL;
	z_ucharint_toa(src_int,buf,10);
	assert(!suffix || 3>=strlen(suffix));
	assert(dest.second);
	if (suffix) strcat(buf,suffix);

	// shrinking realloc should be no-fail
	dest.first = REALLOC(buf,ZAIMONI_LEN_WITH_NULL(strlen(buf)));
}

//! \throw std::bad_alloc()
static void VM_to_literal(parse_tree& dest, const umaxint& src_int,const parse_tree& src,const type_system& types)
{
	POD_pair<char*,lex_flags> new_token;
	VM_to_token(src_int,src.type_code.base_type_index,new_token);
	dest.clear();
	dest.grab_index_token_from<0>(new_token.first,new_token.second);
	dest.grab_index_token_location_from<0,0>(src);
	assert((C_TESTFLAG_CHAR_LITERAL | C_TESTFLAG_STRING_LITERAL | C_TESTFLAG_PP_NUMERAL) & dest.index_tokens[0].flags);
	_label_one_literal(dest,types);
	assert(PARSE_EXPRESSION & dest.flags);
}

static void force_decimal_literal(parse_tree& dest,const char* src,const type_system& types)
{
	assert(src && *src);
	dest.destroy();
	assert(dest.index_tokens[0].src_filename && *dest.index_tokens[0].src_filename);
	dest.index_tokens[0].token.first = src;
	dest.index_tokens[0].token.second = strlen(src);
	dest.index_tokens[0].flags = (C_TESTFLAG_PP_NUMERAL | C_TESTFLAG_INTEGER | C_TESTFLAG_DECIMAL);
	_label_one_literal(dest,types);
}

static parse_tree decimal_literal(const char* src,const parse_tree& loc_src,const type_system& types)
{
	assert(src && *src);
	parse_tree dest;
	dest.clear();
	dest.index_tokens[0].token.first = src;
	dest.index_tokens[0].token.second = strlen(src);
	dest.index_tokens[0].flags = (C_TESTFLAG_PP_NUMERAL | C_TESTFLAG_INTEGER | C_TESTFLAG_DECIMAL);
	dest.grab_index_token_location_from<0,0>(loc_src);
	_label_one_literal(dest,types);
	return dest;
}

//! usually want to use value_copy rather than MoveInto before this (easier to be ACID)
//! \throw std::bad_alloc only if dest.type_code.pointer_power<dest.data<2>()->type_code.pointer_power 
static void force_unary_positive_literal(parse_tree& dest,const parse_tree& src SIG_CONST_TYPES)
{
	assert(0==dest.size<0>());
	assert(0==dest.size<1>());
	assert(1==dest.size<2>());
	assert(!dest.index_tokens[1].token.first);
	dest.grab_index_token_from_str_literal<0>("+",C_TESTFLAG_NONATOMIC_PP_OP_PUNC);
	*dest.c_array<2>() = src;
	dest.core_flag_update();
	dest.flags |= PARSE_STRICT_UNARY_EXPRESSION;
	dest.subtype = C99_UNARY_SUBTYPE_PLUS;
	if (converts_to_arithmeticlike(dest.data<2>()->type_code ARG_TYPES))
		value_copy(dest.type_code,dest.data<2>()->type_code);	//! \bug doesn't work for enumerators
	assert(dest.index_tokens[0].src_filename);
	assert(is_C99_unary_operator_expression<'+'>(dest));
}

static void force_unary_negative_token(parse_tree& dest,parse_tree* src,const parse_tree& loc_src SIG_CONST_TYPES)
{
	assert(src);
	assert(PARSE_EXPRESSION & src->flags);
	dest.clear();
	dest.grab_index_token_from_str_literal<0>("-",C_TESTFLAG_NONATOMIC_PP_OP_PUNC);
	dest.grab_index_token_location_from<0,0>(loc_src);
	dest.fast_set_arg<2>(src);
	dest.core_flag_update();
	dest.flags |= PARSE_STRICT_UNARY_EXPRESSION;
	dest.subtype = C99_UNARY_SUBTYPE_NEG;
	if (converts_to_arithmeticlike(dest.data<2>()->type_code ARG_TYPES))
		value_copy(dest.type_code,dest.data<2>()->type_code);	//! \bug doesn't work for enumerators
	// do not handle type here: C++ operator overloading risk
	assert(dest.index_tokens[0].src_filename);
	assert(is_C99_unary_operator_expression<'-'>(dest));
}

// this one hides a slight inefficiency: negative literals take 2 dynamic memory allocations, positive literals take one
// return code is true for success, false for memory failure
//! \throw std::bad_alloc()
static void VM_to_signed_literal(parse_tree& x,const bool is_negative, const umaxint& src_int,const parse_tree& src,const type_system& types)
{
	if (is_negative)
		{
		parse_tree* tmp = _new_buffer_nonNULL_throws<parse_tree>(1);
		try {
			VM_to_literal(*tmp,src_int,src,types);
			}
		catch(const std::bad_alloc&)
			{
			_flush(tmp);
			throw;
			}
		assert(PARSE_EXPRESSION & tmp->flags);
		force_unary_negative_token(x,tmp,*tmp ARG_TYPES);
		}
	else VM_to_literal(x,src_int,src,types);
}

static bool is_integerlike_literal(const parse_tree& x SIG_CONST_TYPES)
{
	return converts_to_integerlike(x.type_code ARG_TYPES) && (PARSE_PRIMARY_EXPRESSION & x.flags);
}

static bool eval_unary_plus(parse_tree& src, const type_system& types)
{
	assert(is_C99_unary_operator_expression<'+'>(src));
	if (0<src.data<2>()->type_code.pointer_power)
		{	// assume C++98 interpretation, as this is illegal in C99
		//! \test cpp/default/Pass_if_control27.hpp
		if (!(parse_tree::INVALID & src.flags))
			{
			assert(src.type_code==src.data<2>()->type_code);
			src.eval_to_arg<2>(0);
			return true;
			}
		return false;
		};
 	// handle integer-like literals like a real integer literal
	if (is_integerlike_literal(*src.data<2>() ARG_TYPES))
		{
		src.type_code.MoveInto(src.c_array<2>()->type_code);
		src.eval_to_arg<2>(0);
		return true;
		}
	return false;
}

//! \throw std::bad_alloc()
static bool eval_unary_minus(parse_tree& src, const type_system& types,literal_converts_to_bool_func& literal_converts_to_bool,intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	assert(is_C99_unary_operator_expression<'-'>(src));
	bool is_true = false;
	if (literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES) && !is_true && (1==(src.type_code.base_type_index-C_TYPE::INT)%2 || virtual_machine::twos_complement==target_machine->C_signed_int_representation() || bool_options[boolopt::int_traps]))
		{	// -0==0
			// deal with unary - not being allowed to actually return -0 on these machines later
		type_spec tmp;
		src.type_code.OverwriteInto(tmp);
		force_decimal_literal(src,"0",types);
		tmp.MoveInto(src.type_code);
		return true;
		};
	if (is_integerlike_literal(*src.data<2>() ARG_TYPES) && 1==(src.type_code.base_type_index-C_TYPE::INT)%2)
		{	// unsigned...we're fine
		const virtual_machine::std_int_enum machine_type = machine_type_from_type_index(src.type_code.base_type_index);
		umaxint res_int;
		intlike_literal_to_VM(res_int,*src.data<2>() ARG_TYPES);
		target_machine->unsigned_additive_inverse(res_int,machine_type);

		//! \todo flag failures to reduce as RAM-stalled
		POD_pair<char*,lex_flags> new_token;
		VM_to_token(res_int,src.type_code.base_type_index,new_token);
		src.c_array<2>()->grab_index_token_from<0>(new_token.first,new_token.second);
		src.type_code.MoveInto(src.c_array<2>()->type_code);
		src.eval_to_arg<2>(0);
		return true;
		};
	if (converts_to_integerlike(src.data<2>()->type_code ARG_TYPES) && is_C99_unary_operator_expression<'-'>(*src.data<2>()))
		{	// - - __ |-> __, trap-int machines fine as -0=0 for sign/magnitude and one's complement, and the offending literal for two's complement is an unsigned int
		assert(converts_to_integerlike(src.data<2>()->data<2>()->type_code ARG_TYPES));
		parse_tree tmp;
		src.c_array<2>()->c_array<2>()->OverwriteInto(tmp);
		src.type_code.MoveInto(tmp.type_code);
		tmp.MoveInto(src);
		return true;		
		}
	return false;
}

//! \throw std::bad_alloc()
static void C_unary_plusminus_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(C99_UNARY_SUBTYPE_NEG==src.subtype || C99_UNARY_SUBTYPE_PLUS==src.subtype);
	assert((C99_UNARY_SUBTYPE_PLUS==src.subtype) ? is_C99_unary_operator_expression<'+'>(src) : is_C99_unary_operator_expression<'-'>(src));
	// return immediately if applied to a pointer type (C++98 would type here)
	if (0<src.data<2>()->type_code.pointer_power)
		{
		src.type_code.set_type(0);
		simple_error(src,(C99_UNARY_SUBTYPE_PLUS==src.subtype) ? " applies unary + to a pointer (C99 6.5.3.3p1)" : " applies unary - to a pointer (C99 6.5.3.3p1)");
		return;
		}
	// can type if result is a primitive arithmetic type
	if (converts_to_arithmeticlike(src.data<2>()->type_code.base_type_index ARG_TYPES))
		src.type_code.set_type(default_promote_type(src.data<2>()->type_code.base_type_index ARG_TYPES));

	const size_t arg_unary_subtype 	= (is_C99_unary_operator_expression<'-'>(*src.data<2>())) ? C99_UNARY_SUBTYPE_NEG
									: (is_C99_unary_operator_expression<'+'>(*src.data<2>())) ? C99_UNARY_SUBTYPE_PLUS : 0;
	if (!arg_unary_subtype) return;
	// two deep:
	// 2) if inner +/- is applied to a raw pointer, error out and change type to 0
	// 1) if inner +/- is applied to an arithmetic literal, try to crunch it (but handle - signed carefully)
	if (C99_UNARY_SUBTYPE_PLUS==src.subtype)
		{
		if 		(C99_UNARY_SUBTYPE_PLUS==arg_unary_subtype)
			eval_unary_plus(*src.c_array<2>(),types);
		else	// if (C99_UNARY_SUBTYPE_NEG==arg_unary_subtype)
			eval_unary_minus(*src.c_array<2>(),types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM);
		}
	else{	// if (C99_UNARY_SUBTYPE_NEG==src.subtype)
		if 		(C99_UNARY_SUBTYPE_PLUS==arg_unary_subtype)
			eval_unary_plus(*src.c_array<2>(),types);
		else	// if (C99_UNARY_SUBTYPE_NEG==arg_unary_subtype)
			eval_unary_minus(*src.c_array<2>(),types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM);
		}
}

//! \throw std::bad_alloc()
static void CPP_unary_plusminus_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(C99_UNARY_SUBTYPE_NEG==src.subtype || C99_UNARY_SUBTYPE_PLUS==src.subtype);
	assert((C99_UNARY_SUBTYPE_PLUS==src.subtype) ? is_C99_unary_operator_expression<'+'>(src) : is_C99_unary_operator_expression<'-'>(src));
	
	// can type if result is a primitive arithmetic type
	if (converts_to_arithmeticlike(src.data<2>()->type_code ARG_TYPES))
		src.type_code.set_type(default_promote_type(src.data<2>()->type_code.base_type_index ARG_TYPES));

	// two deep:
	// 1) if inner +/- is applied to an arithmetic literal, try to crunch it (but leave - signed alone)
	// 2) if inner +/- is applied to a raw pointer, error out and change type to 0
	if (C99_UNARY_SUBTYPE_PLUS==src.subtype)
		{
		if (0<src.data<2>()->type_code.pointer_power)
			// C++98 5.3.1p6: pointer type allowed for unary +, not for unary - (C99 errors)
			//! \test default/Pass_if_control27.hpp
			value_copy(src.type_code,src.data<2>()->type_code);

		if 		(is_C99_unary_operator_expression<'+'>(*src.data<2>()))
			eval_unary_plus(*src.c_array<2>(),types);
		else if (is_C99_unary_operator_expression<'-'>(*src.data<2>()))
			eval_unary_minus(*src.c_array<2>(),types,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM);
		}
	else{	// if (C99_UNARY_SUBTYPE_NEG==src.subtype)
		// return immediately if result is a pointer type; nested application to a pointer type dies
		if (0<src.data<2>()->type_code.pointer_power)
			{
			src.type_code.set_type(0);
			simple_error(src," applies unary - to a pointer (C++98 5.3.1p7)");
			return;
			}

		const size_t arg_unary_subtype 	= (is_C99_unary_operator_expression<'-'>(*src.data<2>())) ? C99_UNARY_SUBTYPE_NEG
										: (is_C99_unary_operator_expression<'+'>(*src.data<2>())) ? C99_UNARY_SUBTYPE_PLUS : 0;
		if (arg_unary_subtype)
			{
			if 		(C99_UNARY_SUBTYPE_PLUS==arg_unary_subtype)
				eval_unary_plus(*src.c_array<2>(),types);
			else	// if (C99_UNARY_SUBTYPE_NEG==arg_unary_subtype)
				eval_unary_minus(*src.c_array<2>(),types,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM);
			}
		}
}

// this is going to have to be forked eventually; the CPP variant has to be more
// careful with the *& identity as both deference * and reference & are 
// overloadable operators
//! \throw std::bad_alloc()
static void C_deref_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_C99_unary_operator_expression<'*'>(src));
	//! \todo: handle *& identity when we have &
	// multi-dimensional arrays and cv-qualified pointers should be automatically handled
	value_copy(src.type_code,src.data<2>()->type_code);
	// handle lvalueness in indirection type building and/or the dereference stage
	if (!src.type_code.dereference())
		//! \test default/Error_if_control24.hpp, default/Error_if_control24.h
		simple_error(src," is not dereferencing a pointer (C99 6.5.3.2p2; C++98 5.3.1p1)");
}

// no eval_deref because of &* cancellation
// defer syntax check to after resolution of multiply-*, so no C/C++ fork
//! \throw std::bad_alloc()
static bool terse_locate_C99_deref(parse_tree& src, size_t& i,const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	if (token_is_char<'*'>(src.data<0>()[i].index_tokens[0].token))
		{
		assert(1<src.size<0>()-i);	// should be intercepted at context-free check
		inspect_potential_paren_primary_expression(src.c_array<0>()[i+1]);
		if (is_C99_unary_operator_expression<'*'>(src.data<0>()[i+1]))
			C_deref_easy_syntax_check(src.c_array<0>()[i+1],types);
		if (PARSE_CAST_EXPRESSION & src.data<0>()[i+1].flags)
			{
			assemble_unary_postfix_arguments(src,i,C99_UNARY_SUBTYPE_DEREF);
			assert(is_C99_unary_operator_expression<'*'>(src.data<0>()[i]));
			return true;
			};
		}
	return false;
}

//! \throw std::bad_alloc()
static bool terse_locate_CPP_deref(parse_tree& src, size_t& i,const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	if (token_is_char<'*'>(src.data<0>()[i].index_tokens[0].token))
		{
		assert(1<src.size<0>()-i);	// should be intercepted at context-free check
		inspect_potential_paren_primary_expression(src.c_array<0>()[i+1]);
		if (is_C99_unary_operator_expression<'*'>(src.data<0>()[i+1]))
			C_deref_easy_syntax_check(src.c_array<0>()[i+1],types);
		if (PARSE_CAST_EXPRESSION & src.data<0>()[i+1].flags)
			{
			assemble_unary_postfix_arguments(src,i,C99_UNARY_SUBTYPE_DEREF);
			assert(is_C99_unary_operator_expression<'*'>(src.data<0>()[i]));
			return true;
			};
		}
	return false;
}

//! \throw std::bad_alloc()
static bool terse_locate_C_logical_NOT(parse_tree& src, size_t& i,const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	if (token_is_char<'!'>(src.data<0>()[i].index_tokens[0].token))
		{
		assert(1<src.size<0>()-i);	// should be intercepted at context-free check
		inspect_potential_paren_primary_expression(src.c_array<0>()[i+1]);
		if (is_C99_unary_operator_expression<'*'>(src.data<0>()[i+1]))
			C_deref_easy_syntax_check(src.c_array<0>()[i+1],types);
		if (PARSE_CAST_EXPRESSION & src.data<0>()[i+1].flags)
			{
			assemble_unary_postfix_arguments(src,i,C99_UNARY_SUBTYPE_NOT);
			assert(is_C99_unary_operator_expression<'!'>(src.data<0>()[i]));
			return true;
			};
		}
	return false;
}

//! \throw std::bad_alloc()
static bool terse_locate_CPP_logical_NOT(parse_tree& src, size_t& i,const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	if (token_is_char<'!'>(src.data<0>()[i].index_tokens[0].token) || token_is_string<3>(src.data<0>()[i].index_tokens[0].token,"not"))
		{
		assert(1<src.size<0>()-i);	// should be intercepted at context-free check
		inspect_potential_paren_primary_expression(src.c_array<0>()[i+1]);
		if (is_C99_unary_operator_expression<'*'>(src.data<0>()[i+1]))
			C_deref_easy_syntax_check(src.c_array<0>()[i+1],types);
		if (PARSE_CAST_EXPRESSION & src.data<0>()[i+1].flags)
			{
			assemble_unary_postfix_arguments(src,i,C99_UNARY_SUBTYPE_NOT);
			assert(is_CPP_logical_NOT_expression(src.data<0>()[i]));
			return true;
			};
		}
	return false;
}

static bool eval_logical_NOT(parse_tree& src, const type_system& types, func_traits<bool (*)(const parse_tree&)>::function_ref_type is_logical_NOT, literal_converts_to_bool_func& literal_converts_to_bool)
{
	assert(is_logical_NOT(src));
	{	// deal with literals that convert to bool here
	bool is_true = false;
	if (literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES))
		{
		src.destroy();
		src.index_tokens[0].token.first = (is_true) ? "0" : "1";
		src.index_tokens[0].token.second = 1;
		src.index_tokens[0].flags = (C_TESTFLAG_PP_NUMERAL | C_TESTFLAG_INTEGER | C_TESTFLAG_DECIMAL);
		_label_one_literal(src,types);
		return true;
		}
	}
	// logical NOT has period 2, but the first application converts the target to bool; can only cancel 3-deep in general, 2-deep against type bool expressions
	if (is_logical_NOT(*src.data<2>()))
		{
		if (	is_logical_NOT(*src.data<2>()->data<2>())
			||	src.data<2>()->type_code.is_type<C_TYPE::BOOL>())
			{
			parse_tree tmp;
			src.c_array<2>()->c_array<2>()->OverwriteInto(tmp);
			tmp.MoveInto(src);
			return true;
			}
		};
	return false;
}

static void C_logical_NOT_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_C99_unary_operator_expression<'!'>(src));
	src.type_code.set_type(C_TYPE::BOOL);	// technically wrong for C, but the range is restricted to _Bool's range
	if (eval_logical_NOT(src,types,is_C99_unary_operator_expression<'!'>,C99_literal_converts_to_bool)) return;

	if (!converts_to_bool(src.data<2>()->type_code ARG_TYPES))
		{	// can't test this from preprocessor or static assertion
		simple_error(src," applies ! to a nonscalar type (C99 6.5.3.3p1)");
		return;
		}
}

static void CPP_logical_NOT_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_CPP_logical_NOT_expression(src));
	src.type_code.set_type(C_TYPE::BOOL);	// technically wrong for C, but the range is restricted to _Bool's range
	if (eval_logical_NOT(src,types,is_CPP_logical_NOT_expression,CPP_literal_converts_to_bool)) return;

	if (!converts_to_bool(src.data<2>()->type_code ARG_TYPES))
		{	// can't test this from preprocessor or static assertion
		simple_error(src," applies ! to a type not convertible to bool (C++98 5.3.1p8)");
		return;
		}
}

//! \throw std::bad_alloc()
static bool locate_C99_logical_NOT(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	if (terse_locate_C_logical_NOT(src,i,types))
		{
		C_logical_NOT_easy_syntax_check(src.c_array<0>()[i],types);
		return true;
		}
	return false;
}

//! \throw std::bad_alloc()
static bool locate_CPP_logical_NOT(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	if (terse_locate_CPP_logical_NOT(src,i,types))
		{	//! \todo handle operator overloading
		CPP_logical_NOT_easy_syntax_check(src.c_array<0>()[i],types);
		return true;
		}
	return false;
}

static bool int_has_trapped(parse_tree& src,const umaxint& src_int)
{
	assert(C_TYPE::INT<=src.type_code.base_type_index && C_TYPE::INTEGERLIKE>src.type_code.base_type_index);
	// check for trap representation for signed types
	const virtual_machine::std_int_enum machine_type = machine_type_from_type_index(src.type_code.base_type_index);
	if (bool_options[boolopt::int_traps] && 0==(src.type_code.base_type_index-C_TYPE::INT)%2 && target_machine->trap_int(src_int,machine_type))
		{
		if (!no_runtime_errors)
			simple_error(src," generated a trap representation: undefined behavior (C99 6.2.6.1p5)");
		return true;
		}
	return false;
}

//! this doesn't do anything to dest's type
static void force_unary_negative_literal(parse_tree& dest,const parse_tree& src)
{
	assert(0==dest.size<0>());
	assert(0==dest.size<1>());
	assert(1==dest.size<2>());
	assert(!dest.index_tokens[1].token.first);
	dest.grab_index_token_from_str_literal<0>("-",C_TESTFLAG_NONATOMIC_PP_OP_PUNC);
	*dest.c_array<2>() = src;
	dest.core_flag_update();
	dest.flags |= PARSE_STRICT_UNARY_EXPRESSION;
	dest.subtype = C99_UNARY_SUBTYPE_NEG;
	assert(dest.index_tokens[0].src_filename);
	assert(is_C99_unary_operator_expression<'-'>(dest));
}

//! \throw std::bad_alloc()
static bool terse_locate_C99_bitwise_complement(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	if (token_is_char<'~'>(src.data<0>()[i].index_tokens[0].token))
		{
		assert(1<src.size<0>()-i);	// should be intercepted at context-free check
		inspect_potential_paren_primary_expression(src.c_array<0>()[i+1]);
		if (is_C99_unary_operator_expression<'*'>(src.data<0>()[i+1]))
			C_deref_easy_syntax_check(src.c_array<0>()[i+1],types);
		if (PARSE_CAST_EXPRESSION & src.data<0>()[i+1].flags)
			{
			assemble_unary_postfix_arguments(src,i,C99_UNARY_SUBTYPE_COMPL);
			assert(is_C99_unary_operator_expression<'~'>(src.data<0>()[i]));
			return true;
			};
		}
	return false;
}

//! \throw std::bad_alloc()
static bool terse_locate_CPP_bitwise_complement(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	if (token_is_char<'~'>(src.data<0>()[i].index_tokens[0].token) || token_is_string<5>(src.data<0>()[i].index_tokens[0].token,"compl"))
		{
		assert(1<src.size<0>()-i);	// should be intercepted at context-free check
		inspect_potential_paren_primary_expression(src.c_array<0>()[i+1]);
		if (is_C99_unary_operator_expression<'*'>(src.data<0>()[i+1]))
			C_deref_easy_syntax_check(src.c_array<0>()[i+1],types);
		if (PARSE_CAST_EXPRESSION & src.data<0>()[i+1].flags)
			{
			assemble_unary_postfix_arguments(src,i,C99_UNARY_SUBTYPE_COMPL);
			assert(is_CPP_bitwise_complement_expression(src.data<0>()[i]));
			return true;
			};
		}
	return false;
}

// all three users of this want the resulting type to be the same as dest's type
//! \throw std::bad_alloc()
static void construct_twos_complement_int_min(parse_tree& dest, const type_system& types, const virtual_machine::std_int_enum machine_type, const parse_tree& src_loc)
{
	zaimoni::autoval_ptr<parse_tree> tmp2;
	tmp2 = _new_buffer_nonNULL_throws<parse_tree>(1);	// XXX we recycle this variable later
	zaimoni::autoval_ptr<parse_tree> tmp3;
	tmp3 = _new_buffer_nonNULL_throws<parse_tree>(1);	// XXX we recycle this variable later
	{
	umaxint tmp_int(target_machine->signed_max(machine_type));
	zaimoni::autoval_ptr<parse_tree> tmp;
	tmp = _new_buffer_nonNULL_throws<parse_tree>(1);	// XXX we recycle this variable later
	VM_to_literal(*tmp,tmp_int,src_loc,types);

	tmp_int = 1;
	try {
		VM_to_literal(*tmp2,tmp_int,src_loc,types);
		}
	catch(const std::bad_alloc&)
		{
		tmp->destroy();
		throw;
		}

	force_unary_negative_token(*tmp3,tmp.release(),src_loc ARG_TYPES);
	}
	
	parse_tree_class tmp4;
	tmp4.grab_index_token_from_str_literal<0>("-",C_TESTFLAG_NONATOMIC_PP_OP_PUNC);
	tmp4.grab_index_token_location_from<0,0>(src_loc);
	tmp4.fast_set_arg<1>(tmp3.release());
	tmp4.fast_set_arg<2>(tmp2.release());

	tmp4.core_flag_update();
	tmp4.flags |= PARSE_STRICT_ADD_EXPRESSION;
	tmp4.subtype = C99_ADD_SUBTYPE_MINUS;
	assert(is_C99_add_operator_expression<'-'>(tmp4));

	dest.type_code.MoveInto(tmp4.type_code);
	tmp4.MoveInto(dest);
	// do not handle type here: C++ operator overloading risk
}

//! \throw std::bad_alloc()
static bool eval_bitwise_compl(parse_tree& src, const type_system& types,func_traits<bool (*)(const parse_tree&)>::function_ref_type is_bitwise_complement_expression,intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	assert(is_bitwise_complement_expression(src));
	assert(converts_to_integerlike(src.data<2>()->type_code ARG_TYPES));
	umaxint res_int;
	if (intlike_literal_to_VM(res_int,*src.data<2>() ARG_TYPES)) 
		{
		const virtual_machine::std_int_enum machine_type = machine_type_from_type_index(src.type_code.base_type_index);
		res_int.auto_bitwise_complement();
		res_int.mask_to(target_machine->C_bit(machine_type));

		if (int_has_trapped(src,res_int)) return false;

		const bool negative_signed_int = 0==(src.type_code.base_type_index-C_TYPE::INT)%2 && res_int.test(target_machine->C_bit(machine_type)-1);
		if (negative_signed_int) target_machine->signed_additive_inverse(res_int,machine_type);
		if (	virtual_machine::twos_complement==target_machine->C_signed_int_representation()
			&& 	0==(src.type_code.base_type_index-C_TYPE::INT)%2
			&& 	!bool_options[boolopt::int_traps]
			&&	res_int>target_machine->signed_max(machine_type))
			{	// trap representation; need to get it into -INT_MAX-1 form
			construct_twos_complement_int_min(src,types,machine_type,src);
			return true;
			}

		parse_tree tmp;
		VM_to_literal(tmp,res_int,src,types);	// two's-complement non-trapping INT_MIN dies if it gets here

		if (negative_signed_int)
			// convert to parsed - literal
			force_unary_negative_literal(src,tmp);
		else{	// convert to positive literal
			src.type_code.MoveInto(tmp.type_code);
			tmp.MoveInto(src);
			}
		return true;
		};
	if (	is_bitwise_complement_expression(*src.data<2>())
		&&	is_bitwise_complement_expression(*src.data<2>()->data<2>()))
		{	// ~~~__ reduces to ~__ safely
		parse_tree tmp;
		src.c_array<2>()->c_array<2>()->OverwriteInto(tmp);
		tmp.MoveInto(src);
		return true;
		}
	return false;
}

//! \throw std::bad_alloc()
static void C_bitwise_complement_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_C99_unary_operator_expression<'~'>(src));
	const POD_pair<size_t,bool> tmp = default_promotion_is_integerlike(src.data<2>()->type_code ARG_TYPES);
	if (!tmp.second)
		{	//! \test Error_if_control25.h
		src.type_code.set_type(0);
		simple_error(src," applies ~ to a nonintegral type (C99 6.5.3.3p1)");
		return;
		}
	src.type_code.set_type(tmp.first);
	zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
	if (eval_bitwise_compl(src,types,is_C99_unary_operator_expression<'~'>,C99_intlike_literal_to_VM)) return;
}

//! \throw std::bad_alloc()
static void CPP_bitwise_complement_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_CPP_bitwise_complement_expression(src));
	const POD_pair<size_t,bool> tmp = default_promotion_is_integerlike(src.data<2>()->type_code ARG_TYPES);
	if (!tmp.second)
		{
		src.type_code.set_type(0);
		simple_error(src," applies ~ to a nonintegral type (C99 6.5.3.3p1)");
		return;
		}
	src.type_code.set_type(tmp.first);
	zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
	if (eval_bitwise_compl(src,types,is_CPP_bitwise_complement_expression,CPP_intlike_literal_to_VM)) return;
}

//! \throw std::bad_alloc()
static bool locate_C99_bitwise_complement(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());

	if (	!(PARSE_OBVIOUS & src.data<0>()[i].flags)
		&&	src.data<0>()[i].is_atomic()
		&&	terse_locate_C99_bitwise_complement(src,i,types))
		{
		C_bitwise_complement_easy_syntax_check(src.c_array<0>()[i],types);
		return true;
		}
	return false;
}

//! \throw std::bad_alloc()
static bool locate_CPP_bitwise_complement(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());

	if (	!(PARSE_OBVIOUS & src.data<0>()[i].flags)
		&&	src.data<0>()[i].is_atomic()
		&&	terse_locate_CPP_bitwise_complement(src,i,types))
		{	//! \todo handle overloading
		CPP_bitwise_complement_easy_syntax_check(src.c_array<0>()[i],types);
		return true;
		}
	return false;
}

//! \throw std::bad_alloc()
static bool terse_locate_C99_unary_plusminus(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	const size_t unary_subtype 	= (token_is_char<'-'>(src.data<0>()[i].index_tokens[0].token)) ? C99_UNARY_SUBTYPE_NEG
								: (token_is_char<'+'>(src.data<0>()[i].index_tokens[0].token)) ? C99_UNARY_SUBTYPE_PLUS : 0;
	if (unary_subtype)
		{
		assert(1<src.size<0>()-i);	// should be intercepted at context-free check
		inspect_potential_paren_primary_expression(src.c_array<0>()[i+1]);
		if (is_C99_unary_operator_expression<'*'>(src.data<0>()[i+1]))
			C_deref_easy_syntax_check(src.c_array<0>()[i+1],types);
		if (PARSE_CAST_EXPRESSION & src.data<0>()[i+1].flags)
			{
			assemble_unary_postfix_arguments(src,i,unary_subtype);
			src.c_array<0>()[i].type_code.set_type(C_TYPE::NOT_VOID);	// defer to later
			if (   0==i	// unless no predecessor possible
				// operators also work
			    || robust_token_is_char<'~'>(src.data<0>()[i-1].index_tokens[0].token) 
			    || robust_token_is_char<'!'>(src.data<0>()[i-1].index_tokens[0].token) 
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"++") 
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"--") 
			    || robust_token_is_char<'*'>(src.data<0>()[i-1].index_tokens[0].token) 
			    || robust_token_is_char<'/'>(src.data<0>()[i-1].index_tokens[0].token) 
			    || robust_token_is_char<'%'>(src.data<0>()[i-1].index_tokens[0].token) 
			    || robust_token_is_char<'+'>(src.data<0>()[i-1].index_tokens[0].token) 
			    || robust_token_is_char<'-'>(src.data<0>()[i-1].index_tokens[0].token)
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"<<") 
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,">>")
			    || robust_token_is_char<'<'>(src.data<0>()[i-1].index_tokens[0].token) 
			    || robust_token_is_char<'>'>(src.data<0>()[i-1].index_tokens[0].token)
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"<=")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,">=")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"==")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"!=")
			    || robust_token_is_char<'&'>(src.data<0>()[i-1].index_tokens[0].token)
			    || robust_token_is_char<'^'>(src.data<0>()[i-1].index_tokens[0].token)
			    || robust_token_is_char<'|'>(src.data<0>()[i-1].index_tokens[0].token)
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"&&")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"||")
			    || robust_token_is_char<'?'>(src.data<0>()[i-1].index_tokens[0].token)
			    || robust_token_is_char<':'>(src.data<0>()[i-1].index_tokens[0].token)
			    || robust_token_is_char<'='>(src.data<0>()[i-1].index_tokens[0].token)
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"*=")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"/=")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"%=")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"+=")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"-=")
			    || robust_token_is_string<3>(src.data<0>()[i-1].index_tokens[0].token,"<<=")
			    || robust_token_is_string<3>(src.data<0>()[i-1].index_tokens[0].token,">>=")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"&=")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"^=")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"|=")
			    || robust_token_is_char<','>(src.data<0>()[i-1].index_tokens[0].token)) 
				C_unary_plusminus_easy_syntax_check(src.c_array<0>()[i],types);
			assert((C99_UNARY_SUBTYPE_PLUS==unary_subtype) ? is_C99_unary_operator_expression<'+'>(src.data<0>()[i]) : is_C99_unary_operator_expression<'-'>(src.data<0>()[i]));
			return true;
			};
		}
	return false;
}

//! \throw std::bad_alloc()
static bool terse_locate_CPP_unary_plusminus(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	const size_t unary_subtype 	= (token_is_char<'-'>(src.data<0>()[i].index_tokens[0].token)) ? C99_UNARY_SUBTYPE_NEG
								: (token_is_char<'+'>(src.data<0>()[i].index_tokens[0].token)) ? C99_UNARY_SUBTYPE_PLUS : 0;
	if (unary_subtype)
		{
		assert(1<src.size<0>()-i);	// should be intercepted at context-free check
		inspect_potential_paren_primary_expression(src.c_array<0>()[i+1]);
		if (is_C99_unary_operator_expression<'*'>(src.data<0>()[i+1]))
			C_deref_easy_syntax_check(src.c_array<0>()[i+1],types);
		if (PARSE_CAST_EXPRESSION & src.data<0>()[i+1].flags)
			{
			assemble_unary_postfix_arguments(src,i,unary_subtype);
			src.c_array<0>()[i].type_code.set_type(C_TYPE::NOT_VOID);	// defer to later
			if (   0==i	// unless no predecessor possible
				// operators also work
			    || robust_token_is_char<'~'>(src.data<0>()[i-1].index_tokens[0].token) 
			    || robust_token_is_string<5>(src.data<0>()[i-1].index_tokens[0].token,"compl") 
			    || robust_token_is_char<'!'>(src.data<0>()[i-1].index_tokens[0].token) 
			    || robust_token_is_string<3>(src.data<0>()[i-1].index_tokens[0].token,"not") 
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"++") 
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"--") 
			    || robust_token_is_char<'*'>(src.data<0>()[i-1].index_tokens[0].token) 
			    || robust_token_is_char<'/'>(src.data<0>()[i-1].index_tokens[0].token) 
			    || robust_token_is_char<'%'>(src.data<0>()[i-1].index_tokens[0].token) 
			    || robust_token_is_char<'+'>(src.data<0>()[i-1].index_tokens[0].token) 
			    || robust_token_is_char<'-'>(src.data<0>()[i-1].index_tokens[0].token)
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"<<") 
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,">>")
			    || robust_token_is_char<'<'>(src.data<0>()[i-1].index_tokens[0].token) 
			    || robust_token_is_char<'>'>(src.data<0>()[i-1].index_tokens[0].token)
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"<=")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,">=")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"==")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"!=")
			    || robust_token_is_string<6>(src.data<0>()[i-1].index_tokens[0].token,"not_eq")
			    || robust_token_is_char<'&'>(src.data<0>()[i-1].index_tokens[0].token)
			    || robust_token_is_string<6>(src.data<0>()[i-1].index_tokens[0].token,"bitand")
			    || robust_token_is_char<'^'>(src.data<0>()[i-1].index_tokens[0].token)
			    || robust_token_is_string<3>(src.data<0>()[i-1].index_tokens[0].token,"xor")
			    || robust_token_is_char<'|'>(src.data<0>()[i-1].index_tokens[0].token)
			    || robust_token_is_string<5>(src.data<0>()[i-1].index_tokens[0].token,"bitor")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"&&")
			    || robust_token_is_string<3>(src.data<0>()[i-1].index_tokens[0].token,"and")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"||")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"or")
			    || robust_token_is_char<'?'>(src.data<0>()[i-1].index_tokens[0].token)
			    || robust_token_is_char<':'>(src.data<0>()[i-1].index_tokens[0].token)
			    || robust_token_is_char<'='>(src.data<0>()[i-1].index_tokens[0].token)
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"*=")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"/=")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"%=")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"+=")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"-=")
			    || robust_token_is_string<3>(src.data<0>()[i-1].index_tokens[0].token,"<<=")
			    || robust_token_is_string<3>(src.data<0>()[i-1].index_tokens[0].token,">>=")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"&=")
			    || robust_token_is_string<6>(src.data<0>()[i-1].index_tokens[0].token,"and_eq")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"^=")
			    || robust_token_is_string<6>(src.data<0>()[i-1].index_tokens[0].token,"xor_eq")
			    || robust_token_is_string<2>(src.data<0>()[i-1].index_tokens[0].token,"|=")
			    || robust_token_is_string<5>(src.data<0>()[i-1].index_tokens[0].token,"or_eq")
			    || robust_token_is_char<','>(src.data<0>()[i-1].index_tokens[0].token)) 
				CPP_unary_plusminus_easy_syntax_check(src.c_array<0>()[i],types);
			assert((C99_UNARY_SUBTYPE_PLUS==unary_subtype) ? is_C99_unary_operator_expression<'+'>(src.data<0>()[i]) : is_C99_unary_operator_expression<'-'>(src.data<0>()[i]));
			return true;
			};
		}
	return false;
}

//! \throw std::bad_alloc()
static bool locate_C99_unary_plusminus(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return false;

	return terse_locate_C99_unary_plusminus(src,i,types);
}

//! \throw std::bad_alloc()
static bool locate_CPP_unary_plusminus(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return false;

	return terse_locate_CPP_unary_plusminus(src,i,types);
}

/* Scan for unary operators and cast expressions
unary-expression:
	postfix-expression
	++ unary-expression
	-- unary-expression
	unary-operator cast-expression
	sizeof unary-expression
	sizeof ( type-name )
unary-operator: one of
	& * + - ~ !

cast-expression:
	unary-expression
	( type-name ) cast-expression

Note that the binary operators *,+,- are effectively handled by first building the unary operator, then checking whether the left-hand-side qualifies for extension to binary operator
*/
//! \throw std::bad_alloc()
static void locate_C99_unary_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (	(PARSE_OBVIOUS & src.data<0>()[i].flags)
		||	!src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_C99_deref(src,i,types)) return;
	if (locate_C99_logical_NOT(src,i,types)) return;
	if (locate_C99_bitwise_complement(src,i,types)) return;
	if (locate_C99_unary_plusminus(src,i,types)) return;

#if 0
	if (terse_locate_unary_operator(src,i))
		{
		C_unary_op_easy_syntax_check(src.c_array<0>()[i],types);
		return;
		}
	else if (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"++"))
		{
		}
	else if (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"--"))
		{
		}
	else if (   token_is_char<'('>(src.data<0>()[i].index_tokens[0].token)
			 && token_is_char<')'>(src.data<0>()[i].index_tokens[1].token))
		{
		}
#endif
}

/* Scan for unary expressions and cast expressions
unaryexpression:
	postfixexpression
	++ castexpression
	-- castexpression
	unaryoperator castexpression
	sizeof unaryexpression
	sizeof ( typeid )
	newexpression
	deleteexpression

unaryoperator:
	one of * & + - ! ~
	note that compl, not end up here as well.  compl may be used to make psuedo-destructor interpretation illegal
	note that bitand does *not* end up here, so it can prevent inadvertently taking an address

castexpression:
	unaryexpression
	( typeid ) castexpression

deleteexpression:
	::opt delete castexpression
	::opt delete [ ] castexpression

newexpression:
	::opt new newplacementopt newtypeid newinitializeropt
	::opt new newplacementopt ( typeid) newinitializeropt
newplacement:
	( expressionlist )
newtypeid:
	typespecifierseq
	newdeclaratoropt
newdeclarator:
	ptroperator
	newdeclaratoropt
	directnewdeclarator
directnewdeclarator:
	[ expression ]
	directnewdeclarator [ constantexpression ]
newinitializer:
	( expressionlistopt )

Note that the binary operators *,+,- are effectively handled by first building the unary operator, then checking whether the left-hand-side qualifies for extension to binary operator
*/
//! \throw std::bad_alloc()
static void locate_CPP_unary_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (	(PARSE_OBVIOUS & src.data<0>()[i].flags)
		||	!src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_CPP_deref(src,i,types)) return;
	if (locate_CPP_logical_NOT(src,i,types)) return;
	if (locate_CPP_bitwise_complement(src,i,types)) return;
	if (locate_CPP_unary_plusminus(src,i,types)) return;

#if 0
	if (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"++"))
		{
		}
	else if (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"--"))
		{
		}
	else if (   token_is_char<'('>(src.data<0>()[i].index_tokens[0].token)
			 && token_is_char<')'>(src.data<0>()[i].index_tokens[1].token))
		{
		}
	else if (token_is_string<3>(src.data<0>()[i].index_tokens[0].token,"new"))
		{
		}
	else if (token_is_string<6>(src.data<0>()[i].index_tokens[0].token,"delete"))
		{
		}
#endif
}

//! \throw std::bad_alloc()
static void assemble_binary_infix_arguments(parse_tree& src, size_t& i, const lex_flags _flags)
{
	assert(1<=i && 2<=src.size<0>()-i);
	{
	parse_tree* const tmp_c_array = src.c_array<0>()+(i-1);
	zaimoni::autoval_ptr<parse_tree> tmp;
	tmp = repurpose_inner_parentheses(tmp_c_array[0]);	// RAM conservation
	parse_tree* const tmp2 = repurpose_inner_parentheses(tmp_c_array[2]);	// RAM conservation
	tmp_c_array[0].OverwriteInto(*tmp);
	tmp_c_array[2].OverwriteInto(*tmp2);
	tmp_c_array[1].fast_set_arg<1>(tmp.release());
	tmp_c_array[1].fast_set_arg<2>(tmp2);
	tmp_c_array[1].core_flag_update();
	tmp_c_array[1].flags |= _flags;
	}
	src.DeleteIdx<0>(i+1);
	src.DeleteIdx<0>(--i);

	parse_tree& tmp = src.c_array<0>()[i];
	cancel_outermost_parentheses(tmp.c_array<1>()[0]);
	cancel_outermost_parentheses(tmp.c_array<2>()[0]);
}

//! \throw std::bad_alloc()
static void merge_binary_infix_argument(parse_tree& src, size_t& i, const lex_flags _flags)
{
	assert(1<=i);
 	{
	parse_tree* const tmp_c_array = src.c_array<0>()+(i-1);
	parse_tree* const tmp = repurpose_inner_parentheses(tmp_c_array[0]);	// RAM conservation
	tmp_c_array[0].OverwriteInto(*tmp);

	tmp_c_array[1].fast_set_arg<1>(tmp);
	tmp_c_array[1].core_flag_update();
	tmp_c_array[1].flags |= _flags;
	}
	src.DeleteIdx<0>(--i);
	cancel_outermost_parentheses(src.c_array<0>()[i].c_array<1>()[0]);
}

//! \throw std::bad_alloc()
static bool terse_C99_augment_mult_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (is_C99_unary_operator_expression<'*'>(src.data<0>()[i]))
		{
		if (1<=i && (PARSE_MULT_EXPRESSION & src.data<0>()[i-1].flags))
			{	// check for unary +/- with type NOT_VOID
				// if found, split out the +/- into slot i and adjust i up before proceeding
			if (   is_C99_unary_operator_expression<'+'>(src.data<0>()[i-1])
				|| is_C99_unary_operator_expression<'-'>(src.data<0>()[i-1]))
				{
				if (src.data<0>()[i-1].type_code.is_type<C_TYPE::NOT_VOID>())
					{
					src._args[0].insertNSlotsAt(1,i);
					src.c_array<0>()[i].clear();
					src.c_array<0>()[i-1].front<2>().MoveInto(src.c_array<0>()[i]);
					src.c_array<0>()[i-1].DeleteIdx<2>(0);
					src.c_array<0>()[i-1].type_code.base_type_index = 0;
					++i;
					assert(PARSE_MULT_EXPRESSION & src.data<0>()[i-1].flags);
					assert(is_C99_unary_operator_expression<'*'>(src.data<0>()[i]));
					}
				}
			merge_binary_infix_argument(src,i,PARSE_STRICT_MULT_EXPRESSION);
			assert(is_C99_mult_operator_expression(src.data<0>()[i]));
			src.c_array<0>()[i].type_code.set_type(0);	// handle type inference later
			assert(is_C99_mult_operator_expression(src.data<0>()[i]));
			return true;
			};
		// run syntax-checks against unary *
		C_deref_easy_syntax_check(src.c_array<0>()[i],types);
		}
	return false;
}

//! \throw std::bad_alloc()
static bool terse_CPP_augment_mult_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (is_C99_unary_operator_expression<'*'>(src.data<0>()[i]))
		{
		if (1<=i && (inspect_potential_paren_primary_expression(src.c_array<0>()[i-1]),(PARSE_MULT_EXPRESSION & src.data<0>()[i-1].flags)))
			{	// check for unary +/- with type NOT_VOID
				// if found, split out the +/- into slot i and adjust i up before proceeding
			if (   is_C99_unary_operator_expression<'+'>(src.data<0>()[i-1])
				|| is_C99_unary_operator_expression<'-'>(src.data<0>()[i-1]))
				{
				if (src.data<0>()[i-1].type_code.is_type<C_TYPE::NOT_VOID>())
					{
					src._args[0].insertNSlotsAt(1,i);
					src.c_array<0>()[i].clear();
					src.c_array<0>()[i-1].front<2>().MoveInto(src.c_array<0>()[i]);
					src.c_array<0>()[i-1].DeleteIdx<2>(0);
					src.c_array<0>()[i-1].type_code.base_type_index = 0;
					++i;
					assert(PARSE_MULT_EXPRESSION & src.data<0>()[i-1].flags);
					assert(is_C99_unary_operator_expression<'*'>(src.data<0>()[i]));
					}
				}
			merge_binary_infix_argument(src,i,PARSE_STRICT_MULT_EXPRESSION);
			assert(is_C99_mult_operator_expression(src.data<0>()[i]));
			src.c_array<0>()[i].type_code.set_type(0);	// handle type inference later
			assert(is_C99_mult_operator_expression(src.data<0>()[i]));
			return true;
			};
		// run syntax-checks against unary *
		//! \todo handle operator overloading
		C_deref_easy_syntax_check(src.c_array<0>()[i],types);
		}
	return false;
}

//! \throw std::bad_alloc()
static bool terse_locate_mult_expression(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	const size_t mult_subtype 	= (token_is_char<'/'>(src.data<0>()[i].index_tokens[0].token)) ? C99_MULT_SUBTYPE_DIV
								: (token_is_char<'%'>(src.data<0>()[i].index_tokens[0].token)) ? C99_MULT_SUBTYPE_MOD
								: (token_is_char<'*'>(src.data<0>()[i].index_tokens[0].token)) ? C99_MULT_SUBTYPE_MULT : 0;
	if (mult_subtype)
		{
		if (1>i || 2>src.size<0>()-i) return false;
		parse_tree* const tmp_c_array = src.c_array<0>()+(i-1);
		inspect_potential_paren_primary_expression(tmp_c_array[0]);
		inspect_potential_paren_primary_expression(tmp_c_array[2]);
		if ((PARSE_MULT_EXPRESSION & tmp_c_array[0].flags)
			&&	(PARSE_PM_EXPRESSION & tmp_c_array[2].flags))
			{
			assemble_binary_infix_arguments(src,i,PARSE_STRICT_MULT_EXPRESSION);	// tmp_c_array goes invalid here
			assert(is_C99_mult_operator_expression(src.data<0>()[i]));
			{
			parse_tree& tmp = src.c_array<0>()[i];
			tmp.subtype = mult_subtype;
			tmp.type_code.set_type(0);	// handle type inference later
			}
			assert(is_C99_mult_operator_expression(src.data<0>()[i]));
			return true;
			}
		}
	return false;
}

//! \throw std::bad_alloc()
static bool eval_mult_expression(parse_tree& src, const type_system& types, literal_converts_to_bool_func& literal_converts_to_bool,intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	assert(is_C99_mult_operator_expression<'*'>(src));

	bool is_true = false;

	// do this first to avoid unnecessary dynamic memory allocation
	if (	(literal_converts_to_bool(*src.data<1>(),is_true ARG_TYPES) && !is_true)	// 0 * __
		||	(literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES) && !is_true))	// __ * 0
		{
		// construct +0 to defuse 1-0*6
		parse_tree tmp = decimal_literal("0",src,types);
		if (C_TYPE::INTEGERLIKE==src.type_code.base_type_index)
			{
			message_header(src.index_tokens[0]);
			INC_INFORM("invalid ");
			INC_INFORM(src);
			INFORM(" optimized to valid 0");
			tmp.type_code.set_type(C_TYPE::LLONG);	// legalize
			}
		else value_copy(tmp.type_code,src.type_code);
		src.DeleteIdx<1>(0);
		force_unary_positive_literal(src,tmp ARG_TYPES);
		return true;
		};

	umaxint res_int;
	umaxint rhs_int;
	const bool lhs_converted = intlike_literal_to_VM(res_int,*src.data<1>() ARG_TYPES);
	const bool rhs_converted = intlike_literal_to_VM(rhs_int,*src.data<2>() ARG_TYPES);
	if (lhs_converted && 1==res_int)
		{
		src.type_code.MoveInto(src.c_array<2>()->type_code);
		src.eval_to_arg<2>(0);
		return true;
		};
	if (rhs_converted && 1==rhs_int)
		{
		src.type_code.MoveInto(src.c_array<1>()->type_code);
		src.eval_to_arg<1>(0);
		return true;
		};
	if (lhs_converted && rhs_converted)
		{
		const promote_aux old(src.type_code.base_type_index ARG_TYPES);
		const promote_aux lhs(src.data<1>()->type_code.base_type_index ARG_TYPES);
		assert(old.bitcount>=lhs.bitcount);
		const promote_aux rhs(src.data<2>()->type_code.base_type_index ARG_TYPES);
		assert(old.bitcount>=rhs.bitcount);

		// handle sign-extension of lhs, rhs
		const bool lhs_negative = target_machine->C_promote_integer(res_int,lhs,old);
		const bool rhs_negative = target_machine->C_promote_integer(rhs_int,rhs,old);
		if (old.is_signed)
			{	// signed integer result: overflow is undefined
			umaxint lhs_test(res_int);
			umaxint rhs_test(rhs_int);
			umaxint ub(target_machine->signed_max(old.machine_type));
			const bool tweak_ub = rhs_negative!=lhs_negative && virtual_machine::twos_complement==target_machine->C_signed_int_representation() && !bool_options[boolopt::int_traps];
			if (rhs_negative) target_machine->signed_additive_inverse(rhs_test,old.machine_type);
			if (lhs_negative) target_machine->signed_additive_inverse(lhs_test,old.machine_type);
			if (tweak_ub) ub += 1;
			if (ub<lhs_test || ub<rhs_test)
				{
				if (!no_runtime_errors)
					//! \todo catch this in two's-complement specific testing
					simple_error(src," signed * overflow, undefined behavior (C99 6.5p5, C++98 5p5)");
				return false;
				}
			const bool lhs_lt_rhs = lhs_test<rhs_test;
			ub /= (lhs_lt_rhs) ? rhs_test : lhs_test;
			if (ub<(lhs_lt_rhs ? lhs_test : rhs_test))
				{	//! \test if.C99/Pass_conditional_op_noeval.hpp, if.C99/Pass_conditional_op_noeval.h
				if (!no_runtime_errors)
					//! \test default/Error_if_control29.hpp, default/Error_if_control29.h
					simple_error(src," signed * overflow, undefined behavior (C99 6.5p5, C++98 5p5)");
				return false;
				}
			lhs_test *= rhs_test;
			if (rhs_negative!=lhs_negative)
				{	// valid result, but not representable: do not reduce (errors out spuriously)
				if (tweak_ub && target_machine->signed_max(old.machine_type)<lhs_test) return false;

				target_machine->signed_additive_inverse(lhs_test,old.machine_type);
				// convert to parsed - literal
				parse_tree tmp;
				VM_to_literal(tmp,lhs_test,src,types);

				src.DeleteIdx<1>(0);
				force_unary_negative_literal(src,tmp);
				return true;
				}
			res_int = lhs_test;
			}
		else
			res_int *= rhs_int;

		// convert to parsed + literal
		parse_tree tmp;
		VM_to_literal(tmp,res_int,src,types);
		value_copy(tmp.type_code,src.type_code);
		src.DeleteIdx<1>(0);
		force_unary_positive_literal(src,tmp ARG_TYPES);
		return true;
		}
	return false;
}

//! \throw std::bad_alloc()
static bool eval_div_expression(parse_tree& src, const type_system& types, literal_converts_to_bool_func& literal_converts_to_bool,intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	assert(is_C99_mult_operator_expression<'/'>(src));

	bool is_true = false;
	if (converts_to_integerlike(src.type_code ARG_TYPES))
		{
		if 		(literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES) && !is_true)
			{	//! \test if.C99/Pass_conditional_op_noeval.hpp, if.C99/Pass_conditional_op_noeval.h
			if (!no_runtime_errors)
				//! \test default/Error_if_control30.hpp, default/Error_if_control30.h
				simple_error(src," division by zero, undefined behavior (C99 6.5.5p5, C++98 5.6p4)");
			return false;
			}
		/*! \todo would like a simple comparison of absolute values to auto-detect zero, possibly after mainline code */
		else if (literal_converts_to_bool(*src.data<1>(),is_true ARG_TYPES) && !is_true)
			{
			// construct +0 to defuse 1-0/6
			parse_tree tmp = decimal_literal("0",src,types);
			if (C_TYPE::INTEGERLIKE==src.type_code.base_type_index)
				{
				message_header(src.index_tokens[0]);
				INC_INFORM("invalid ");
				INC_INFORM(src);
				INFORM(" optimized to valid 0");
				tmp.type_code.set_type(C_TYPE::LLONG);	// legalize
				}
			else value_copy(tmp.type_code,src.type_code);
			src.DeleteIdx<1>(0);
			force_unary_positive_literal(src,tmp ARG_TYPES);
			return true;
			}
		//! \todo change target for formal verification; would like to inject a constraint against div-by-integer-zero here
		};

	umaxint res_int;
	umaxint rhs_int;
	const bool lhs_converted = intlike_literal_to_VM(res_int,*src.data<1>() ARG_TYPES);
	const bool rhs_converted = intlike_literal_to_VM(rhs_int,*src.data<2>() ARG_TYPES);
	if (rhs_converted && rhs_int==1)
		{	// __/1 |-> __
		src.type_code.MoveInto(src.c_array<1>()->type_code);
		src.eval_to_arg<1>(0);
		return true;
		};
	// two's complement can overflow: INT_MIN/-1
	// implementation-defined whether negative results round away or to zero (standard prefers to zero, so default to that)
	if (lhs_converted && rhs_converted)
		{
		const promote_aux old(src.type_code.base_type_index ARG_TYPES);
		const promote_aux lhs(src.data<1>()->type_code.base_type_index ARG_TYPES);
		assert(old.bitcount>=lhs.bitcount);
		const promote_aux rhs(src.data<2>()->type_code.base_type_index ARG_TYPES);
		assert(old.bitcount>=rhs.bitcount);

		// handle sign-extension of lhs, rhs
		const bool lhs_negative = target_machine->C_promote_integer(res_int,lhs,old);
		const bool rhs_negative = target_machine->C_promote_integer(rhs_int,rhs,old);
		if (old.is_signed)
			{	// signed integer result
			umaxint lhs_test(res_int);
			umaxint rhs_test(rhs_int);
			umaxint ub(target_machine->signed_max(old.machine_type));
			if (rhs_negative) target_machine->signed_additive_inverse(rhs_test,old.machine_type);
			if (lhs_negative) target_machine->signed_additive_inverse(lhs_test,old.machine_type);
			if (rhs_negative!=lhs_negative && virtual_machine::twos_complement==target_machine->C_signed_int_representation()) ub += 1;
			if (lhs_test<rhs_test)
				{
				const bool want_zero = rhs_negative==lhs_negative || !bool_options[boolopt::int_neg_div_rounds_away_from_zero];
				parse_tree tmp = decimal_literal(want_zero ? "0" : "1",src,types);
				src.DeleteIdx<1>(0);
				if (want_zero)
					{
					value_copy(tmp.type_code,src.type_code);
					force_unary_positive_literal(src,tmp ARG_TYPES); // +0
					}
				else	
					force_unary_negative_literal(src,tmp); // -1
				return true;
				}

			bool round_away = false;
			if (rhs_negative!=lhs_negative && bool_options[boolopt::int_neg_div_rounds_away_from_zero])
				{
				umaxint lhs_mod_test(lhs_test);
				lhs_mod_test %= rhs_test;
				round_away = 0!=lhs_mod_test;
				}
			lhs_test /= rhs_test;
			if (rhs_negative!=lhs_negative)
				{
				if (round_away) lhs_test += 1;
				target_machine->signed_additive_inverse(lhs_test,old.machine_type);
				// convert to parsed - literal
				parse_tree tmp;
				VM_to_literal(tmp,lhs_test,src,types);

				src.DeleteIdx<1>(0);
				force_unary_negative_literal(src,tmp);
				return true;
				}
			if (ub<lhs_test)
				{	//! \todo test this in two's complement code
				assert(virtual_machine::twos_complement==target_machine->C_signed_int_representation());
				if (!no_runtime_errors)
					simple_error(src," signed / overflow, undefined behavior (C99 6.5p5, C++98 5p5)");
				return false;
				}

			res_int = lhs_test;
			}
		else
			res_int /= rhs_int;

		// convert to parsed + literal
		parse_tree tmp;
		VM_to_literal(tmp,res_int,src,types);

		value_copy(tmp.type_code,src.type_code);
		src.DeleteIdx<1>(0);
		force_unary_positive_literal(src,tmp ARG_TYPES);
		return true;
		}
	return false;
}

//! \throw std::bad_alloc()
static bool eval_mod_expression(parse_tree& src, const type_system& types, literal_converts_to_bool_func& literal_converts_to_bool,intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	assert(is_C99_mult_operator_expression<'%'>(src));

	bool is_true = false;
	if (converts_to_integerlike(src.type_code ARG_TYPES))
		{
		if 		(literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES) && !is_true)
			{	//! \test if.C99/Pass_conditional_op_noeval.hpp, if.C99/Pass_conditional_op_noeval.h
			if (!no_runtime_errors)
				//! \test default/Error_if_control31.hpp, Error_if_control31.h
				simple_error(src," modulo by zero, undefined behavior (C99 6.5.5p5, C++98 5.6p4)");
			return false;
			}
		/*! \todo would like a simple comparison of absolute values to auto-detect zero, possibly after mainline code */
		else if (literal_converts_to_bool(*src.data<1>(),is_true ARG_TYPES) && !is_true)
			{
			// construct +0 to defuse 1-0%6
			parse_tree tmp = decimal_literal("0",src,types);
			if (C_TYPE::INTEGERLIKE==src.type_code.base_type_index)
				{
				message_header(src.index_tokens[0]);
				INC_INFORM("invalid ");
				INC_INFORM(src);
				INFORM(" optimized to valid 0");
				tmp.type_code.set_type(C_TYPE::LLONG);	// legalize
				}
			else value_copy(tmp.type_code,src.type_code);
			src.DeleteIdx<1>(0);
			force_unary_positive_literal(src,tmp ARG_TYPES);
			return true;
			}
		//! \todo change target for formal verification; would like to inject a constraint against div-by-integer-zero here
		};

	umaxint res_int;
	umaxint rhs_int;
	const bool lhs_converted = intlike_literal_to_VM(res_int,*src.data<1>() ARG_TYPES);
	const bool rhs_converted = intlike_literal_to_VM(rhs_int,*src.data<2>() ARG_TYPES);
	if (rhs_converted && rhs_int==1)
		{	// __%1 |-> +0
		parse_tree tmp = decimal_literal("0",src,types);
		if (C_TYPE::INTEGERLIKE!=src.type_code.base_type_index)
			value_copy(tmp.type_code,src.type_code);
		else
			tmp.type_code.set_type(C_TYPE::LLONG);	// legalize
		src.DeleteIdx<1>(0);
		force_unary_positive_literal(src,tmp ARG_TYPES);
		return true;
		};
	if (lhs_converted && rhs_converted)
		{
		const promote_aux old(src.type_code.base_type_index ARG_TYPES);
		const promote_aux lhs(src.data<1>()->type_code.base_type_index ARG_TYPES);
		assert(old.bitcount>=lhs.bitcount);
		const promote_aux rhs(src.data<2>()->type_code.base_type_index ARG_TYPES);
		assert(old.bitcount>=rhs.bitcount);

		// handle sign-extension of lhs, rhs
		const bool lhs_negative = target_machine->C_promote_integer(res_int,lhs,old);
		const bool rhs_negative = target_machine->C_promote_integer(rhs_int,rhs,old);
		if (old.is_signed)
			{	// signed integer result
			umaxint lhs_test(res_int);
			umaxint rhs_test(rhs_int);
			umaxint ub(target_machine->signed_max(old.machine_type));
			if (rhs_negative) target_machine->signed_additive_inverse(rhs_test,old.machine_type);
			if (lhs_negative) target_machine->signed_additive_inverse(lhs_test,old.machine_type);
			if (rhs_negative!=lhs_negative && virtual_machine::twos_complement==target_machine->C_signed_int_representation()) ub += 1;

			lhs_test %= rhs_test;
			if (0!=lhs_test && rhs_negative!=lhs_negative)
				{
				if (!bool_options[boolopt::int_neg_div_rounds_away_from_zero])
					{	// convert to parsed - literal
					parse_tree tmp;
					VM_to_literal(tmp,lhs_test,src,types);

					src.DeleteIdx<1>(0);
					force_unary_negative_literal(src,tmp);
					return true;
					};
				rhs_test -= lhs_test;
				lhs_test = rhs_test;
				};

			res_int = lhs_test;
			}
		else	// unsigned integer result: C99 6.3.1.3p2 dictates modulo conversion to unsigned
			res_int %= rhs_int;

		// convert to parsed + literal
		parse_tree tmp;
		VM_to_literal(tmp,res_int,src,types);

		value_copy(tmp.type_code,src.type_code);
		src.DeleteIdx<1>(0);
		force_unary_positive_literal(src,tmp ARG_TYPES);
		return true;
		}
	return false;
}

BOOST_STATIC_ASSERT(1==C99_MULT_SUBTYPE_MOD-C99_MULT_SUBTYPE_DIV);
BOOST_STATIC_ASSERT(1==C99_MULT_SUBTYPE_MULT-C99_MULT_SUBTYPE_MOD);

static bool _mod_expression_typecheck(parse_tree& src SIG_CONST_TYPES)
{
	assert(C99_MULT_SUBTYPE_MOD==src.subtype && is_C99_mult_operator_expression<'%'>(src));
	POD_pair<size_t,bool> lhs = default_promotion_is_integerlike(src.data<1>()->type_code ARG_TYPES);
	POD_pair<size_t,bool> rhs = default_promotion_is_integerlike(src.data<2>()->type_code ARG_TYPES);
	if (!lhs.second)
		{	//! \test default/Error_if_control33.hpp, default/Error_if_control33.h
			//! \test default/Error_if_control34.hpp, default/Error_if_control34.h
		simple_error(src,rhs.second ? " has nonintegral LHS (C99 6.5.5p2, C++98 5.6p2)" : " has nonintegral LHS and RHS (C99 6.5.5p2, C++98 5.6p2)");
		return false;
		}
	else if (!rhs.second)
		{	//! \test default/Error_if_control32.hpp, default/Error_if_control32.h
		simple_error(src," has nonintegral RHS (C99 6.5.5p2, C++98 5.6p2)");
		return false;
		};
	src.type_code.set_type(arithmetic_reconcile(lhs.first,rhs.first ARG_TYPES));
	return true;
}

static bool _mult_div_expression_typecheck(parse_tree& src SIG_CONST_TYPES)
{
	assert(C99_MULT_SUBTYPE_DIV==src.subtype || C99_MULT_SUBTYPE_MULT==src.subtype);
	assert((C99_MULT_SUBTYPE_DIV==src.subtype) ? is_C99_mult_operator_expression<'/'>(src) : is_C99_mult_operator_expression<'*'>(src));
	POD_pair<size_t,bool> lhs = default_promotion_is_arithmeticlike(src.data<1>()->type_code ARG_TYPES);
	POD_pair<size_t,bool> rhs = default_promotion_is_arithmeticlike(src.data<2>()->type_code ARG_TYPES);
	if (!lhs.second)
		{	//! \test default/Error_if_control36.hpp, default/Error_if_control36.h
			//! \test default/Error_if_control37.hpp, default/Error_if_control37.h
			//! \test default/Error_if_control39.hpp, default/Error_if_control39.h
			//! \test default/Error_if_control40.hpp, default/Error_if_control40.h
		simple_error(src,rhs.second ? " has nonarithmetic LHS (C99 6.5.5p2, C++98 5.6p2)" : " has nonarithmetic LHS and RHS (C99 6.5.5p2, C++98 5.6p2)");
		return false;
		}
	else if (!rhs.second)
		{	//! \test default/Error_if_control35.hpp, default/Error_if_control35.h
			//! \test default/Error_if_control38.hpp, default/Error_if_control38.h
		simple_error(src," has nonarithmetic RHS (C99 6.5.5p2, C++98 5.6p2)");
		return false;
		};

	src.type_code.set_type(arithmetic_reconcile(lhs.first,rhs.first ARG_TYPES));
	return true;
}

//! \throw std::bad_alloc()
static void C_mult_expression_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(C99_MULT_SUBTYPE_DIV<=src.subtype && C99_MULT_SUBTYPE_MULT>=src.subtype);
	assert((C99_MULT_SUBTYPE_DIV==src.subtype) ? is_C99_mult_operator_expression<'/'>(src) : (C99_MULT_SUBTYPE_MULT==src.subtype) ? is_C99_mult_operator_expression<'*'>(src) : is_C99_mult_operator_expression<'%'>(src));
	// note that 0*integerlike and so on are invalid, but do optimize to valid (but this is probably worth a separate execution path)
	if (C99_MULT_SUBTYPE_MOD==src.subtype)
		{	// require integral type
		if (!_mod_expression_typecheck(src ARG_TYPES)) return;
		zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
		eval_mod_expression(src,types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM);
		}
	else{	// require arithmetic type
		if (!_mult_div_expression_typecheck(src ARG_TYPES)) return;
		zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
		if (C99_MULT_SUBTYPE_MULT==src.subtype)
			eval_mult_expression(src,types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM);
		else
			eval_div_expression(src,types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM);			
		}
}

//! \throw std::bad_alloc()
static void CPP_mult_expression_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(C99_MULT_SUBTYPE_DIV<=src.subtype && C99_MULT_SUBTYPE_MULT>=src.subtype);
	assert((C99_MULT_SUBTYPE_DIV==src.subtype) ? is_C99_mult_operator_expression<'/'>(src) : (C99_MULT_SUBTYPE_MULT==src.subtype) ? is_C99_mult_operator_expression<'*'>(src) : is_C99_mult_operator_expression<'%'>(src));

	if (C99_MULT_SUBTYPE_MOD==src.subtype)
		{	// require integral type
		if (!_mod_expression_typecheck(src ARG_TYPES)) return;
		zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
		eval_mod_expression(src,types,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM);
		}
	else{	// require arithmetic type
		if (!_mult_div_expression_typecheck(src ARG_TYPES)) return;
		zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
		if (C99_MULT_SUBTYPE_MULT==src.subtype)
			eval_mult_expression(src,types,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM);
		else
			eval_div_expression(src,types,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM);
		}
}

/*
multiplicative-expression:
	cast-expression
	multiplicative-expression * cast-expression
	multiplicative-expression / cast-expression
	multiplicative-expression % cast-expression
*/
//! \throw std::bad_alloc()
static void locate_C99_mult_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());

	if (terse_C99_augment_mult_expression(src,i,types))
		{
		C_mult_expression_easy_syntax_check(src.c_array<0>()[i],types);
		// reconstitute raw +- from terse_C99_augment_mult_expression into unary +-
		if (0<i)
			{	// synchronize with locate_C99_unary_plusminus
			if (const size_t unary_subtype 	= token_is_char<'-'>(src.data<0>()[i-1].index_tokens[0].token) ? C99_UNARY_SUBTYPE_NEG
									: token_is_char<'+'>(src.data<0>()[i-1].index_tokens[0].token) ? C99_UNARY_SUBTYPE_PLUS : 0)
				{
				assemble_unary_postfix_arguments(src,i-1,unary_subtype);	// doesn't work: reference to temporary
				src.c_array<0>()[i-1].type_code.set_type(C_TYPE::NOT_VOID);	// defer to later
				assert((C99_UNARY_SUBTYPE_PLUS==unary_subtype) ? is_C99_unary_operator_expression<'+'>(src.data<0>()[i-1]) : is_C99_unary_operator_expression<'-'>(src.data<0>()[i-1]));
				};
			}
		return;
		}

	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_mult_expression(src,i)) C_mult_expression_easy_syntax_check(src.c_array<0>()[i],types);
}

/*
multexpression:
	pmexpression
	multexpression * pmexpression
	multexpression / pmexpression
	multexpression % pmexpression
*/
//! \throw std::bad_alloc()
static void locate_CPP_mult_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());

	if (terse_CPP_augment_mult_expression(src,i,types))
		{	//! \todo handle operator overloading
		CPP_mult_expression_easy_syntax_check(src.c_array<0>()[i],types);
		// reconstitute raw +- from terse_CPP_augment_mult_expression into unary +-
		if (0<i)
			{	// synchronize with locate_CPP_unary_plusminus
			if (const size_t unary_subtype 	= token_is_char<'-'>(src.data<0>()[i-1].index_tokens[0].token) ? C99_UNARY_SUBTYPE_NEG
									: token_is_char<'+'>(src.data<0>()[i-1].index_tokens[0].token) ? C99_UNARY_SUBTYPE_PLUS : 0)
				{
				assemble_unary_postfix_arguments(src,i-1,unary_subtype);
				src.c_array<0>()[i-1].type_code.set_type(C_TYPE::NOT_VOID);	// defer to later
				assert((C99_UNARY_SUBTYPE_PLUS==unary_subtype) ? is_C99_unary_operator_expression<'+'>(src.data<0>()[i-1]) : is_C99_unary_operator_expression<'-'>(src.data<0>()[i-1]));
				};
			}
		return;
		}

	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_mult_expression(src,i))
		//! \todo handle operator overloading
		CPP_mult_expression_easy_syntax_check(src.c_array<0>()[i],types);
}

// Law of Demeter conversion is object-size neutral [Dec. 9 2009], so don't do it
static bool C_string_literal_equal_content(const parse_tree& lhs, const parse_tree& rhs,bool& is_equal)
{
	if (C_TESTFLAG_STRING_LITERAL==lhs.index_tokens[0].flags && C_TESTFLAG_STRING_LITERAL==rhs.index_tokens[0].flags)
		{
		const size_t lhs_len = LengthOfCStringLiteral(lhs.index_tokens[0].token.first,lhs.index_tokens[0].token.second);
		if (LengthOfCStringLiteral(rhs.index_tokens[0].token.first,rhs.index_tokens[0].token.second)!=lhs_len)
			{	// string literals of different length are necessarily different decayed pointers even if they overlap
			is_equal = false;
			return true;
			};
		if (('L'==*lhs.index_tokens[0].token.first)!=('L'==*rhs.index_tokens[0].token.first))
			{	// wide string literals never overlap with narrow string literals with the same character values
				//! \todo check language standards: is it implementation-defined whether a wide-string character literal 
				//! can overlap a narrow-string character literal with a suitable placement of NUL bytes?
			is_equal = false;
			return true;
			};

		size_t i = 0;
		while(i<lhs_len-1)
			{
			char* const lhs_lit = GetCCharacterLiteralAt(lhs.index_tokens[0].token.first,lhs.index_tokens[0].token.second,i);
			char* const rhs_lit = GetCCharacterLiteralAt(rhs.index_tokens[0].token.first,rhs.index_tokens[0].token.second,i);
			const uintmax_t lhs_val = EvalCharacterLiteral(lhs_lit,strlen(lhs_lit));
			const uintmax_t rhs_val = EvalCharacterLiteral(rhs_lit,strlen(rhs_lit));
			free(lhs_lit);
			free(rhs_lit);
			if (lhs_val!=rhs_val)
				{	// different at this place, so different
				is_equal = false;
				return true;
				}
			++i;
			}
		// assume hyper-optimizing linker; the string literals overlap
		is_equal = true;
		return true;
		}
	return false;
}

//! \throw std::bad_alloc()
static bool terse_C99_augment_add_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (is_C99_unary_operator_expression<'+'>(src.data<0>()[i]) || is_C99_unary_operator_expression<'-'>(src.data<0>()[i]))
		{
		if (1<=i && (inspect_potential_paren_primary_expression(src.c_array<0>()[i-1]),(PARSE_ADD_EXPRESSION & src.data<0>()[i-1].flags)))
			{
			merge_binary_infix_argument(src,i,PARSE_STRICT_ADD_EXPRESSION);
			assert(is_C99_add_operator_expression(src.data<0>()[i]));
			src.c_array<0>()[i].type_code.set_type(0);	// handle type inference later
			assert(is_C99_add_operator_expression(src.data<0>()[i]));
			return true;
			};
		// run syntax-checks against unary + or unary -
		C_unary_plusminus_easy_syntax_check(src.c_array<0>()[i],types);
		}
	return false;
}

//! \throw std::bad_alloc()
static bool terse_CPP_augment_add_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (is_C99_unary_operator_expression<'+'>(src.data<0>()[i]) || is_C99_unary_operator_expression<'-'>(src.data<0>()[i]))
		{
		if (1<=i && (inspect_potential_paren_primary_expression(src.c_array<0>()[i-1]),(PARSE_ADD_EXPRESSION & src.data<0>()[i-1].flags)))
			{
			merge_binary_infix_argument(src,i,PARSE_STRICT_ADD_EXPRESSION);
			assert(is_C99_add_operator_expression(src.data<0>()[i]));
			src.c_array<0>()[i].type_code.set_type(0);	// handle type inference later
			assert(is_C99_add_operator_expression(src.data<0>()[i]));
			return true;
			};
		// run syntax-checks against unary + or unary -
		CPP_unary_plusminus_easy_syntax_check(src.c_array<0>()[i],types);
		}
	return false;
}

//! \throw std::bad_alloc()
static bool terse_locate_add_expression(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	const size_t add_subtype 	= (token_is_char<'+'>(src.data<0>()[i].index_tokens[0].token)) ? C99_ADD_SUBTYPE_PLUS
								: (token_is_char<'-'>(src.data<0>()[i].index_tokens[0].token)) ? C99_ADD_SUBTYPE_MINUS : 0;
	if (add_subtype)
		{
		if (1>i || 2>src.size<0>()-i) return false;
		parse_tree* const tmp_c_array = src.c_array<0>()+(i-1);
		inspect_potential_paren_primary_expression(tmp_c_array[0]);
		inspect_potential_paren_primary_expression(tmp_c_array[2]);
		if (	(PARSE_ADD_EXPRESSION & tmp_c_array[0].flags)
			&&	(PARSE_MULT_EXPRESSION & tmp_c_array[2].flags))
			{
			assemble_binary_infix_arguments(src,i,PARSE_STRICT_ADD_EXPRESSION);	// tmp_c_array goes invalid here
			assert(is_C99_add_operator_expression(src.data<0>()[i]));
			parse_tree& tmp = src.c_array<0>()[i];
			tmp.subtype = add_subtype;
			tmp.type_code.set_type(0);	// handle type inference later
			assert(is_C99_add_operator_expression(src.data<0>()[i]));
			return true;
			}
		}
	return false;
}

//! \throw std::bad_alloc()
static bool eval_add_expression(parse_tree& src, const type_system& types, literal_converts_to_bool_func& literal_converts_to_bool,intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	assert(is_C99_add_operator_expression<'+'>(src));

	const size_t lhs_pointer = src.data<1>()->type_code.pointer_power;
	const size_t rhs_pointer = src.data<2>()->type_code.pointer_power;	
	// void pointers should have been intercepted by now
	assert(1!=lhs_pointer || C_TYPE::VOID!=src.data<1>()->type_code.base_type_index);
	assert(1!=rhs_pointer || C_TYPE::VOID!=src.data<2>()->type_code.base_type_index);

	switch((0<lhs_pointer)+2*(0<rhs_pointer))
	{
#ifndef NDEBUG
	default: FATAL_CODE("hardware/compiler error: invalid linear combination in eval_add_expression",3);
#endif
	case 0:	{
			assert(converts_to_arithmeticlike(src.data<1>()->type_code.base_type_index ARG_TYPES));
			assert(converts_to_arithmeticlike(src.data<2>()->type_code.base_type_index ARG_TYPES));
			bool is_true = false;
			if 		(literal_converts_to_bool(*src.data<1>(),is_true ARG_TYPES) && !is_true)
				{	// 0 + __ |-> __
				src.type_code.MoveInto(src.c_array<2>()->type_code);
				src.eval_to_arg<2>(0);
				return true;
				}
			else if (literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES) && !is_true)
				{	// __ + 0 |-> __
				src.type_code.MoveInto(src.c_array<1>()->type_code);
				src.eval_to_arg<1>(0);
				return true;
				};
			umaxint res_int;
			umaxint rhs_int;
			const promote_aux old(src.type_code.base_type_index ARG_TYPES);
			const promote_aux lhs(src.data<1>()->type_code.base_type_index ARG_TYPES);
			assert(old.bitcount>=lhs.bitcount);
			const promote_aux rhs(src.data<2>()->type_code.base_type_index ARG_TYPES);
			assert(old.bitcount>=rhs.bitcount);
			const bool lhs_converted = intlike_literal_to_VM(res_int,*src.data<1>() ARG_TYPES);
			const bool rhs_converted = intlike_literal_to_VM(rhs_int,*src.data<2>() ARG_TYPES);
			const bool lhs_negative = lhs_converted && target_machine->C_promote_integer(res_int,lhs,old);
			const bool rhs_negative = rhs_converted && target_machine->C_promote_integer(rhs_int,rhs,old);
			if (lhs_converted && rhs_converted)
				{
				if (old.is_signed)
					{	// signed integer result
					umaxint lhs_test(res_int);
					umaxint rhs_test(rhs_int);
					umaxint ub(target_machine->signed_max(old.machine_type));
					bool result_is_negative = false;
					if (rhs_negative) target_machine->signed_additive_inverse(rhs_test,old.machine_type);
					if (lhs_negative) target_machine->signed_additive_inverse(lhs_test,old.machine_type);
					if (rhs_negative!=lhs_negative)
						{	// cancellation...safe
						switch(cmp(lhs_test,rhs_test))
						{
						case -1:
							result_is_negative = rhs_negative;
							rhs_test -= lhs_test;
							lhs_test = rhs_test;
							break;
						case 0:
							lhs_test.clear();
							break;
						case 1:
							result_is_negative = lhs_negative;
							lhs_test -= rhs_test;
							break;
						};
						}
					else{	// augmentation: bounds-check
						result_is_negative = lhs_negative;
						const bool tweak_ub = result_is_negative && virtual_machine::twos_complement==target_machine->C_signed_int_representation() && !bool_options[boolopt::int_traps];
						if (tweak_ub) ub += 1;
						if (ub<lhs_test || ub<rhs_test || (ub -= lhs_test)<rhs_test)
							{	//! \test if.C99/Pass_conditional_op_noeval.hpp, if.C99/Pass_conditional_op_noeval.h
							if (!no_runtime_errors)
								//! \test default/Error_if_control41.hpp, default/Error_if_control41.h
								simple_error(src," signed + overflow, undefined behavior (C99 6.5p5, C++98 5p5)");
							return false;
							};
						lhs_test += rhs_test;
						// if we can't render it, do not reduce
						if (tweak_ub && target_machine->signed_max(old.machine_type)<lhs_test) return false;
						}

					if (result_is_negative)
						{	// convert to parsed - literal
						parse_tree tmp;
						VM_to_literal(tmp,lhs_test,src,types);

						src.DeleteIdx<1>(0);
						force_unary_negative_literal(src,tmp);
						return true;
						};
					res_int = lhs_test;
					}
				else
					res_int += rhs_int;

				// convert to parsed + literal
				parse_tree tmp;
				VM_to_literal(tmp,res_int,src,types);
				value_copy(tmp.type_code,src.type_code);
				src.DeleteIdx<1>(0);
				force_unary_positive_literal(src,tmp ARG_TYPES);
				return true;
				}
			break;
			}
	case 1:	{
			assert(converts_to_integerlike(src.data<2>()->type_code.base_type_index ARG_TYPES));
			bool is_true = false;
			if (literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES) && !is_true)
				{	// __ + 0 |-> __
				src.type_code.MoveInto(src.c_array<1>()->type_code);
				src.eval_to_arg<1>(0);
				return true;
				}
			break;
			}
	case 2:	{
			assert(converts_to_integerlike(src.data<1>()->type_code.base_type_index ARG_TYPES));
			bool is_true = false;
			if (literal_converts_to_bool(*src.data<1>(),is_true ARG_TYPES) && !is_true)
				{	// 0 + __ |-> __
				src.type_code.MoveInto(src.c_array<2>()->type_code);
				src.eval_to_arg<2>(0);
				return true;
				}
			break;
			}
#ifndef NDEBUG
	case 3:	FATAL_CODE("invalid expression not flagged as invalid expression",3);
#endif
	}
	return false;
}

//! \throw std::bad_alloc()
static bool eval_sub_expression(parse_tree& src, const type_system& types, literal_converts_to_bool_func& literal_converts_to_bool,intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	assert(is_C99_add_operator_expression<'-'>(src));
	const size_t lhs_pointer = src.data<1>()->type_code.pointer_power;
	const size_t rhs_pointer = src.data<2>()->type_code.pointer_power;	
	// void pointers should have been intercepted by now
	assert(1!=lhs_pointer || C_TYPE::VOID!=src.data<1>()->type_code.base_type_index);
	assert(1!=rhs_pointer || C_TYPE::VOID!=src.data<2>()->type_code.base_type_index);

	switch((0<lhs_pointer)+2*(0<rhs_pointer))
	{
#ifndef NDEBUG
	default: FATAL_CODE("hardware/compiler error: invalid linear combination in eval_sub_expression",3);
#endif
	case 0:	{
			assert(converts_to_arithmeticlike(src.data<1>()->type_code.base_type_index ARG_TYPES));
			assert(converts_to_arithmeticlike(src.data<2>()->type_code.base_type_index ARG_TYPES));
			bool is_true = false;
			if 		(literal_converts_to_bool(*src.data<1>(),is_true ARG_TYPES) && !is_true)
				{	// 0 - __ |-> - __
				src.DeleteIdx<1>(0);
				src.core_flag_update();
				src.flags |= PARSE_STRICT_UNARY_EXPRESSION;
				src.subtype = C99_UNARY_SUBTYPE_NEG;
				assert(is_C99_unary_operator_expression<'-'>(src));
				return true;
				}
			else if (literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES) && !is_true)
				{	// __ - 0 |-> __
				src.type_code.MoveInto(src.c_array<1>()->type_code);
				src.eval_to_arg<1>(0);
				return true;
				}
			umaxint res_int;
			umaxint rhs_int;
			const bool lhs_converted = intlike_literal_to_VM(res_int,*src.data<1>() ARG_TYPES);
			const bool rhs_converted = intlike_literal_to_VM(rhs_int,*src.data<2>() ARG_TYPES);
			if (lhs_converted && rhs_converted)
				{	//! \todo deal with signed integer arithmetic
				const promote_aux old(src.type_code.base_type_index ARG_TYPES);
				const promote_aux lhs(src.data<1>()->type_code.base_type_index ARG_TYPES);
				assert(old.bitcount>=lhs.bitcount);
				const promote_aux rhs(src.data<2>()->type_code.base_type_index ARG_TYPES);
				assert(old.bitcount>=rhs.bitcount);

				// handle sign-extension of lhs, rhs
				const bool lhs_negative = target_machine->C_promote_integer(res_int,lhs,old);
				const bool rhs_negative = target_machine->C_promote_integer(rhs_int,rhs,old);
				if (old.is_signed)
					{	// signed integer result
					umaxint lhs_test(res_int);
					umaxint rhs_test(rhs_int);
					umaxint ub(target_machine->signed_max(old.machine_type));
					bool result_is_negative = false;
					if (rhs_negative) target_machine->signed_additive_inverse(rhs_test,old.machine_type);
					if (lhs_negative) target_machine->signed_additive_inverse(lhs_test,old.machine_type);
					if (rhs_negative==lhs_negative)
						{	// cancellation: safe
						switch(cmp(lhs_test,rhs_test))
						{
						case -1:
							result_is_negative = !lhs_negative;
							rhs_test -= lhs_test;
							lhs_test = rhs_test;
							break;
						case 0:
							lhs_test.clear();
							break;
						case 1:
							result_is_negative = lhs_negative;
							lhs_test -= rhs_test;
							break;
						};
						}
					else{	// augmentation: need bounds check
						result_is_negative = lhs_negative;
						const bool tweak_ub = result_is_negative && virtual_machine::twos_complement==target_machine->C_signed_int_representation() && !bool_options[boolopt::int_traps];
						if (tweak_ub) ub += 1;
						if (ub<lhs_test || ub<rhs_test || (ub -= lhs_test)<rhs_test)
							{	//! \test if.C99/Pass_conditional_op_noeval.hpp, if.C99/Pass_conditional_op_noeval.h
							if (!no_runtime_errors)
								//! \test default/Error_if_control42.hpp, default/Error_if_control42.h
								simple_error(src," signed - overflow, undefined behavior (C99 6.5p5, C++98 5p5)");
							return false;
							};
						lhs_test += rhs_test;
						// if we can't render it, do not reduce
						if (tweak_ub && target_machine->signed_max(old.machine_type)<lhs_test) return false;
						}

					if (result_is_negative)
						{	// convert to parsed - literal
						parse_tree tmp;
						VM_to_literal(tmp,lhs_test,src,types);
						src.DeleteIdx<1>(0);
						force_unary_negative_literal(src,tmp);
						return true;
						};
					res_int = lhs_test;
					}
				else
					res_int -= rhs_int;

				// convert to parsed + literal
				parse_tree tmp;
				VM_to_literal(tmp,res_int,src,types);
				value_copy(tmp.type_code,src.type_code);
				src.DeleteIdx<1>(0);
				force_unary_positive_literal(src,tmp ARG_TYPES);
				return true;
				}
			break;
			}
	case 1:	{
			assert(converts_to_integerlike(src.data<2>()->type_code.base_type_index ARG_TYPES));
			bool is_true = false;
			if (literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES) && !is_true)
				{	// __ - 0 |-> __
				src.type_code.MoveInto(src.c_array<1>()->type_code);
				src.eval_to_arg<1>(0);
				return true;
				}
			break;
			}
#ifndef NDEBUG
	case 2:	FATAL_CODE("invalid expression not flagged as invalid expression",3);
#endif
	case 3:	{	// hyper-optimizing linker: two string literals decay to equal pointers iff they are equal under strcmp
				// use this to short-circuit to 0; remember to adjust the preprocessor hacks as well
			bool is_equal = false;
			if (C_string_literal_equal_content(*src.data<1>(),*src.data<2>(),is_equal) && is_equal)
				{	//! \test default/Pass_if_zero.hpp, default/Pass_if_zero.h
				type_spec old_type;
				src.type_code.OverwriteInto(old_type);
				force_decimal_literal(src,"0",types);
				old_type.MoveInto(src.type_code);
				return true;
				}
			break;
			}
	}
	return false;
}

// +: either both are arithmetic, or one is raw pointer and one is integer
// -: either both are arithmetic, or both are compatible raw pointer, or left is raw pointer and right is integer
//! \throw std::bad_alloc()
static void C_CPP_add_expression_easy_syntax_check(parse_tree& src,const type_system& types,literal_converts_to_bool_func& literal_converts_to_bool,intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	assert((C99_ADD_SUBTYPE_PLUS==src.subtype && is_C99_add_operator_expression<'+'>(src)) || (C99_ADD_SUBTYPE_MINUS==src.subtype && is_C99_add_operator_expression<'-'>(src)));
	BOOST_STATIC_ASSERT(1==C99_ADD_SUBTYPE_MINUS-C99_ADD_SUBTYPE_PLUS);
	const size_t lhs_pointer = src.data<1>()->type_code.pointer_power;
	const size_t rhs_pointer = src.data<2>()->type_code.pointer_power;	

	// pointers to void are disallowed; not testable from preprocessor
	const bool exact_rhs_voidptr = 1==rhs_pointer && C_TYPE::VOID==src.data<2>()->type_code.base_type_index;
	if (1==lhs_pointer && C_TYPE::VOID==src.data<1>()->type_code.base_type_index)
		{
		simple_error(src,exact_rhs_voidptr ? " uses void* arguments (C99 6.5.6p2,3; C++98 5.7p1,2)" : " uses void* left-hand argument (C99 6.5.6p2,3; C++98 5.7p1,2)");
		return;
		}
	else if (exact_rhs_voidptr)
		{
		simple_error(src," uses void* right-hand argument (C99 6.5.6p2,3; C++98 5.7p1,2)");
		return;
		}

	switch((0<lhs_pointer)+2*(0<rhs_pointer)+4*(src.subtype-C99_ADD_SUBTYPE_PLUS))
	{
#ifndef NDEBUG
	default: FATAL_CODE("hardware/compiler error: invalid linear combination in C_add_expression_easy_syntax_check",3);
#endif
	case 0:	{	// cannot test errors from preprocessor
			POD_pair<size_t,bool> lhs = default_promotion_is_arithmeticlike(src.data<1>()->type_code ARG_TYPES);
			POD_pair<size_t,bool> rhs = default_promotion_is_arithmeticlike(src.data<2>()->type_code ARG_TYPES);
			if (!lhs.second)
				{
				simple_error(src,rhs.second ? " has non-arithmetic non-pointer right argument (C99 6.5.6p2; C++98 5.7p1)" : " has non-arithmetic non-pointer arguments (C99 6.5.6p2; C++98 5.7p1)");
				return;
				}
			else if (!rhs.second)
				{
				simple_error(src," has non-arithmetic non-pointer left argument (C99 6.5.6p2; C++98 5.7p1)");
				return;
				}

			src.type_code.set_type(arithmetic_reconcile(lhs.first,rhs.first ARG_TYPES));
			zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
			eval_add_expression(src,types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM);
			break;
			}
	case 1:	{	// ptr + integer, hopefully
				// requires floating-point literals to test errors from preprocessor
			value_copy(src.type_code,src.data<1>()->type_code);
			if (!converts_to_integerlike(src.data<2>()->type_code.base_type_index ARG_TYPES))
				{
				simple_error(src," adds pointer to non-integer (C99 6.5.6p2; C++98 5.7p1)");
				return;
				}
			zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
			eval_add_expression(src,types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM);
			break;
			}
	case 2:	{	// integer + ptr, hopefully
				// requires floating-point literals to test errors from preprocessor
			value_copy(src.type_code,src.data<2>()->type_code);
			if (!converts_to_integerlike(src.data<1>()->type_code.base_type_index ARG_TYPES))
				{
				simple_error(src," adds pointer to non-integer (C99 6.5.6p2; C++98 5.7p1)");
				return;
				}
			zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
			eval_add_expression(src,types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM);
			break;
			}
	case 3:	{	//	ptr + ptr dies
				//! \test default/Error_if_control43.hpp, default/Error_if_control43.h
			simple_error(src," adds two pointers (C99 6.5.6p2; C++98 5.7p1)");
			return;
			}
	case 4:	{	// cannot test errors from preprocessor
			POD_pair<size_t,bool> lhs = default_promotion_is_arithmeticlike(src.data<1>()->type_code ARG_TYPES);
			POD_pair<size_t,bool> rhs = default_promotion_is_arithmeticlike(src.data<2>()->type_code ARG_TYPES);
			if (!lhs.second)
				{
				simple_error(src,rhs.second ? " has non-arithmetic non-pointer right argument (C99 6.5.6p3; C++98 5.7p2)" : " has non-arithmetic non-pointer arguments (C99 6.5.6p3; C++98 5.7p2)");
				return;
				}
			else if (!rhs.second)
				{
				simple_error(src," has non-arithmetic non-pointer left argument (C99 6.5.6p3; C++98 5.7p2)");
				return;
				}

			src.type_code.set_type(arithmetic_reconcile(lhs.first,rhs.first ARG_TYPES));
			zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
			eval_sub_expression(src,types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM);
			break;
			}
	case 5:	{	// ptr - integer, hopefully; requires floating-point literal to test from preprocessor
			value_copy(src.type_code,src.data<1>()->type_code);
			if (!converts_to_integerlike(src.data<2>()->type_code.base_type_index ARG_TYPES))
				{
				simple_error(src," subtracts non-integer from pointer (C99 6.5.6p3; C++98 5.7p2)");
				return;
				}
			zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
			eval_sub_expression(src,types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM);
			break;
			}
	case 6:	{	// non-ptr - ptr dies
				//! \test default/Error_if_control44.hpp, default/Error_if_control44.h
			simple_error(src," subtracts a non-pointer from a pointer (C99 6.5.6p3; C++98 5.7p2)");
			return;
			}
	case 7:	{	// ptr - ptr should be compatible
				// type is ptrdiff_t
			const virtual_machine::std_int_enum tmp = target_machine->ptrdiff_t_type();
			assert(tmp);
			src.type_code.set_type((virtual_machine::std_int_char==tmp ? C_TYPE::CHAR
							:	virtual_machine::std_int_short==tmp ? C_TYPE::SHRT
							:	virtual_machine::std_int_int==tmp ? C_TYPE::INT
							:	virtual_machine::std_int_long==tmp ? C_TYPE::LONG
							:	virtual_machine::std_int_long_long==tmp ? C_TYPE::LLONG : 0));
			assert(0!=src.type_code.base_type_index);
			zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
			eval_sub_expression(src,types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM);
			break;
			}
	}
}

/*
additive-expression:
	multiplicative-expression
	additive-expression + multiplicative-expression
	additive-expression - multiplicative-expression
*/
static void locate_C99_add_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());

	if (terse_C99_augment_add_expression(src,i,types))
		{
		C_CPP_add_expression_easy_syntax_check(src.c_array<0>()[i],types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM);
		return;
		}

	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_add_expression(src,i))
		C_CPP_add_expression_easy_syntax_check(src.c_array<0>()[i],types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM);
}

/*
additive-expression:
	multiplicative-expression
	additive-expression + multiplicative-expression
	additive-expression - multiplicative-expression
*/
static void locate_CPP_add_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());

	if (terse_CPP_augment_add_expression(src,i,types))
		{	//! \todo handle operator overloading
		C_CPP_add_expression_easy_syntax_check(src.c_array<0>()[i],types,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM);
		return;
		}

	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_add_expression(src,i))
		//! \todo handle operator overloading
		C_CPP_add_expression_easy_syntax_check(src.c_array<0>()[i],types,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM);
}

static bool binary_infix_failed_integer_arguments(parse_tree& src, const char* standard SIG_CONST_TYPES)
{
	assert(NULL!=standard);
	if (parse_tree::INVALID & src.flags)	// already invalid, don't make noise
		return !converts_to_integerlike(src.data<1>()->type_code ARG_TYPES) || !converts_to_integerlike(src.data<2>()->type_code ARG_TYPES);

	// hmm... 45-47, 48-50, 51-53, 54-56, 57-59
	//! \todo need tests for float literal in place of int literal: << >> & ^ |
	const bool rhs_integerlike = converts_to_integerlike(src.data<2>()->type_code ARG_TYPES);
	if (!converts_to_integerlike(src.data<1>()->type_code ARG_TYPES))
		{	// tests for string literal in place of integer literal
			//! \test default/Error_if_control46.hpp, default/Error_if_control46.h
			//! \test default/Error_if_control47.hpp, default/Error_if_control47.h
			//! \test default/Error_if_control49.hpp, default/Error_if_control49.h
			//! \test default/Error_if_control50.hpp, default/Error_if_control50.h
			//! \test default/Error_if_control52.hpp, default/Error_if_control52.h
			//! \test default/Error_if_control53.hpp, default/Error_if_control53.h
			//! \test default/Error_if_control55.hpp, default/Error_if_control55.h
			//! \test default/Error_if_control56.hpp, default/Error_if_control56.h
			//! \test default/Error_if_control58.hpp, default/Error_if_control58.h
			//! \test default/Error_if_control59.hpp, default/Error_if_control59.h
		src.flags |= parse_tree::INVALID;
		message_header(src.index_tokens[0]);
		INC_INFORM(ERR_STR);
		INC_INFORM(src);
		INC_INFORM(rhs_integerlike ? " has nonintegral LHS " : " has nonintegral LHS and RHS ");
		INFORM(standard);
		zcc_errors.inc_error();
		return true;
		}
	else if (!rhs_integerlike)
		{	// tests for string literal in place of integer literal
			//! \test default/Error_if_control45.hpp, default/Error_if_control45.h
			//! \test default/Error_if_control48.hpp, default/Error_if_control48.h
			//! \test default/Error_if_control51.hpp, default/Error_if_control51.h
			//! \test default/Error_if_control54.hpp, default/Error_if_control54.h
			//! \test default/Error_if_control57.hpp, default/Error_if_control57.h
		src.flags |= parse_tree::INVALID;
		message_header(src.index_tokens[0]);
		INC_INFORM(ERR_STR);
		INC_INFORM(src);
		INC_INFORM(" has nonintegral RHS ");
		INFORM(standard);
		zcc_errors.inc_error();
		return true;
		}
	return false;
}

static bool terse_locate_shift_expression(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	const size_t shift_subtype 	= (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"<<")) ? C99_SHIFT_SUBTYPE_LEFT
								: (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,">>")) ? C99_SHIFT_SUBTYPE_RIGHT : 0;
	if (shift_subtype)
		{
		if (1>i || 2>src.size<0>()-i) return false;
		parse_tree* const tmp_c_array = src.c_array<0>()+(i-1);
		inspect_potential_paren_primary_expression(tmp_c_array[0]);
		inspect_potential_paren_primary_expression(tmp_c_array[2]);
		if (	(PARSE_SHIFT_EXPRESSION & tmp_c_array[0].flags)
			&&	(PARSE_ADD_EXPRESSION & tmp_c_array[2].flags))
			{
			assemble_binary_infix_arguments(src,i,PARSE_STRICT_SHIFT_EXPRESSION);	// tmp_c_array goes invalid here
			assert(is_C99_shift_expression(src.data<0>()[i]));
			parse_tree& tmp = src.c_array<0>()[i];
			tmp.subtype = shift_subtype;
			tmp.type_code.set_type(0);	// handle type inference later
			assert(is_C99_shift_expression(src.data<0>()[i]));
			return true;
			}
		}
	return false;
}

//! \throw std::bad_alloc()
static bool eval_shift(parse_tree& src, const type_system& types, literal_converts_to_bool_func& literal_converts_to_bool,intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	assert(converts_to_integerlike(src.data<1>()->type_code ARG_TYPES));
	assert(converts_to_integerlike(src.data<2>()->type_code ARG_TYPES));
	assert(C99_SHIFT_SUBTYPE_LEFT<=src.subtype && C99_SHIFT_SUBTYPE_RIGHT>=src.subtype);
	BOOST_STATIC_ASSERT(1==C99_SHIFT_SUBTYPE_RIGHT-C99_SHIFT_SUBTYPE_LEFT);
	// handle:
	// 0 << __ |-> 0
	// __ << 0 |-> __
	// 0 >> __ |-> 0
	// __ >> 0 |-> __
	// two integer literals
	// error if RHS is literal "out of bounds"
	bool is_true = false;
	if (literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES) && !is_true)
		{
		if (!is_true)
			{	// __ << 0 or __ >> 0: lift
			src.type_code.MoveInto(src.c_array<1>()->type_code);
			src.eval_to_arg<1>(0);
			return true;
			}
		};

	umaxint rhs_int;
	if (intlike_literal_to_VM(rhs_int,*src.data<2>() ARG_TYPES))
		{
		const virtual_machine::std_int_enum machine_type = machine_type_from_type_index(src.type_code.base_type_index);
		const bool undefined_behavior = target_machine->C_bit(machine_type)<=rhs_int;

		//! \todo can't test with static test case (need to use bitcount of uintmax_t/intmax_t)
		if (undefined_behavior)
			simple_error(src," : RHS is at least as large as bits of LHS; undefined behavior (C99 6.5.7p3/C++98 5.8p1)");

		if (literal_converts_to_bool(*src.data<1>(),is_true ARG_TYPES))
			{
			if (!is_true)
				{	// 0 << __ or 0 >> __: zero out (note that we can do this even if we invoked undefined behavior)
				type_spec tmp;
				src.type_code.OverwriteInto(tmp);
				force_decimal_literal(src,"0",types);
				tmp.MoveInto(src.type_code);
				return true;
				}
			};
		if (undefined_behavior) return false;

		umaxint res_int;
		if (intlike_literal_to_VM(res_int,*src.data<1>() ARG_TYPES))
			{
			// note that incoming negative signed integers are not handled by this code path
			if (C99_SHIFT_SUBTYPE_LEFT==src.subtype)
				{	//! \todo but signed integers do go undefined in C if left-shifted too much; C++ accepts
#if 0
				if (0==(src.type_code.base_type_index-C_TYPE::INT)%2 && target_machine->C_bit(machine_type)<=rhs_int.to_uint()+lhs_int.int_log2()+1)
					simple_error(src," : result does not fit in LHS type; undefined behavior (C99 6.5.7p3)");
#endif
				res_int <<= rhs_int.to_uint();
				if (int_has_trapped(src,res_int)) return false;
				}
			else	// if (C99_SHIFT_SUBTYPE_RIGHT==src.subtype)
				res_int >>= rhs_int.to_uint();

			const virtual_machine::std_int_enum machine_type = machine_type_from_type_index(src.type_code.base_type_index);
			const bool negative_signed_int = 0==(src.type_code.base_type_index-C_TYPE::INT)%2 && res_int.test(target_machine->C_bit(machine_type)-1);
			if (negative_signed_int) target_machine->signed_additive_inverse(res_int,machine_type);
			parse_tree tmp;
			VM_to_literal(tmp,res_int,src,types);

			if (negative_signed_int)
				{	// convert to parsed - literal
				src.DeleteIdx<1>(0);
				force_unary_negative_literal(src,tmp);
				}
			else{	// convert to positive literal
				src.type_code.MoveInto(tmp.type_code);
				tmp.MoveInto(src);
				}
			return true;
			}
		}
	return false;
}

//! \throw std::bad_alloc()
static void C_shift_expression_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_C99_shift_expression(src));
	// C99 6.5.7p2: requires being an integer type
	if (binary_infix_failed_integer_arguments(src,"(C99 6.5.7p2)" ARG_TYPES)) return;
	src.type_code.base_type_index = default_promote_type(src.data<1>()->type_code.base_type_index ARG_TYPES);
	assert(converts_to_integerlike(src.type_code.base_type_index ARG_TYPES));
	zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
	if (eval_shift(src,types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM)) return;
}

//! \throw std::bad_alloc()
static void CPP_shift_expression_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_C99_shift_expression(src));
	// C++98 5.8p1: requires being an integer or enumeration type
	if (binary_infix_failed_integer_arguments(src,"(C++98 5.8p1)" ARG_TYPES)) return;
	src.type_code.base_type_index = default_promote_type(src.data<1>()->type_code.base_type_index ARG_TYPES);
	assert(converts_to_integerlike(src.type_code.base_type_index ARG_TYPES));
	zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
	if (eval_shift(src,types,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM)) return;
}

/*
shift-expression:
	additive-expression
	shift-expression << additive-expression
	shift-expression >> additive-expression
*/
//! \throw std::bad_alloc()
static void locate_C99_shift_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_shift_expression(src,i))
		C_shift_expression_easy_syntax_check(src.c_array<0>()[i],types);
}

/*
shift-expression:
	additive-expression
	shift-expression << additive-expression
	shift-expression >> additive-expression
*/
//! \throw std::bad_alloc()
static void locate_CPP_shift_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_shift_expression(src,i))
		//! \todo handle overloading
		CPP_shift_expression_easy_syntax_check(src.c_array<0>()[i],types);
}

//! \throw std::bad_alloc
static bool terse_locate_relation_expression(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	const size_t rel_subtype 	= (token_is_char<'<'>(src.data<0>()[i].index_tokens[0].token)) ? C99_RELATION_SUBTYPE_LT
								: (token_is_char<'>'>(src.data<0>()[i].index_tokens[0].token)) ? C99_RELATION_SUBTYPE_GT
								: (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"<=")) ? C99_RELATION_SUBTYPE_LTE
								: (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,">=")) ? C99_RELATION_SUBTYPE_GTE : 0;
	if (rel_subtype)
		{
		if (1>i || 2>src.size<0>()-i) return false;
		parse_tree* const tmp_c_array = src.c_array<0>()+(i-1);
		inspect_potential_paren_primary_expression(tmp_c_array[0]);
		inspect_potential_paren_primary_expression(tmp_c_array[2]);
		if (	(PARSE_SHIFT_EXPRESSION & tmp_c_array[0].flags)
			&&	(PARSE_ADD_EXPRESSION & tmp_c_array[2].flags))
			{
			assemble_binary_infix_arguments(src,i,PARSE_STRICT_RELATIONAL_EXPRESSION);	// tmp_c_array goes invalid here
			assert(is_C99_relation_expression(src.data<0>()[i]));
			parse_tree& tmp = src.c_array<0>()[i];
			tmp.subtype = rel_subtype;
			tmp.type_code.set_type(C_TYPE::BOOL);
			assert(is_C99_relation_expression(src.data<0>()[i]));
			return true;
			}
		}
	return false;
}

static bool eval_relation_expression(parse_tree& src, const type_system& types,intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	BOOST_STATIC_ASSERT(1==C99_RELATION_SUBTYPE_GT-C99_RELATION_SUBTYPE_LT);
	BOOST_STATIC_ASSERT(1==C99_RELATION_SUBTYPE_LTE-C99_RELATION_SUBTYPE_GT);
	BOOST_STATIC_ASSERT(1==C99_RELATION_SUBTYPE_GTE-C99_RELATION_SUBTYPE_LTE);
	assert(C99_RELATION_SUBTYPE_LT<=src.subtype && C99_RELATION_SUBTYPE_GTE>=src.subtype);
	umaxint lhs_int;
	umaxint rhs_int;

	const bool lhs_converted = intlike_literal_to_VM(lhs_int,*src.data<1>() ARG_TYPES);
	const bool rhs_converted = intlike_literal_to_VM(rhs_int,*src.data<2>() ARG_TYPES);
	if (lhs_converted && rhs_converted)
		{
		const char* result 	= NULL;
		const promote_aux targ(arithmetic_reconcile(src.data<1>()->type_code.base_type_index, src.data<2>()->type_code.base_type_index ARG_TYPES) ARG_TYPES);
		const promote_aux lhs(src.data<1>()->type_code.base_type_index ARG_TYPES);
		assert(targ.bitcount>=lhs.bitcount);
		const promote_aux rhs(src.data<2>()->type_code.base_type_index ARG_TYPES);
		assert(targ.bitcount>=rhs.bitcount);

		// handle sign-extension of lhs, rhs
		bool use_unsigned_compare = true;
		const bool lhs_negative = target_machine->C_promote_integer(lhs_int,lhs,targ);
		const bool rhs_negative = target_machine->C_promote_integer(rhs_int,rhs,targ);
		if (rhs_negative) target_machine->signed_additive_inverse(rhs_int,targ.machine_type);
		if (lhs_negative) target_machine->signed_additive_inverse(lhs_int,targ.machine_type);

		//! \todo --do-what-i-mean redefines < > operators to handle cross-domain
		if (targ.is_signed && (lhs_negative || rhs_negative))
			{	// repair switch to handle signed numerals
			if (lhs_negative && rhs_negative)
				std::swap(lhs_int,rhs_int);
			else{
				const bool lhs_zero = target_machine->is_zero(lhs_int.data(),lhs_int.size(),targ);
				const bool rhs_zero = target_machine->is_zero(rhs_int.data(),rhs_int.size(),targ);
				const bool op_uses_less_than = (src.subtype%2);	// low-level, check with static assertions
				// is above correct?
				BOOST_STATIC_ASSERT(C99_RELATION_SUBTYPE_LT%2);
				BOOST_STATIC_ASSERT(C99_RELATION_SUBTYPE_LTE%2);
				BOOST_STATIC_ASSERT(!(C99_RELATION_SUBTYPE_GT%2));
				BOOST_STATIC_ASSERT(!(C99_RELATION_SUBTYPE_GTE%2));
				use_unsigned_compare = false;
				if (!lhs_zero)
					result = lhs_negative ? (op_uses_less_than ? "1" : "0") : (op_uses_less_than ? "0" : "1");
				else if (!rhs_zero)
					result = rhs_negative ? (op_uses_less_than ? "0" : "1") : (op_uses_less_than ? "1" : "0");
				else{	// is below correct?
					BOOST_STATIC_ASSERT(C99_RELATION_SUBTYPE_LTE<=C99_RELATION_SUBTYPE_GTE);
					BOOST_STATIC_ASSERT(C99_RELATION_SUBTYPE_LT<C99_RELATION_SUBTYPE_LTE);
					BOOST_STATIC_ASSERT(C99_RELATION_SUBTYPE_GT<C99_RELATION_SUBTYPE_LTE);
					result = (C99_RELATION_SUBTYPE_LTE<=src.subtype) ? "1" : "0"; 	// low-level, check with static assertions					
					}				
				}
			};
		if (use_unsigned_compare)
			switch(src.subtype)
			{
			case C99_RELATION_SUBTYPE_LT:
				result = (lhs_int<rhs_int) ? "1" : "0";
				break;
			case C99_RELATION_SUBTYPE_GT:
				result = (lhs_int>rhs_int) ? "1" : "0";
				break;
			case C99_RELATION_SUBTYPE_LTE:
				result = (lhs_int<=rhs_int) ? "1" : "0";
				break;
			case C99_RELATION_SUBTYPE_GTE:
				result = (lhs_int>=rhs_int) ? "1" : "0";
				break;
			};
		force_decimal_literal(src,result,types);
		return true;
		};
	//! \bug: string literals can't handle GTE, LTE
	//! \bug: string literals handle GT, LT only with respect to zero (NULL constant)?
	return false;
}

static bool C_CPP_relation_expression_core_syntax_ok(parse_tree& src,const type_system& types)
{
	const unsigned int ptr_case = (0<src.data<1>()->type_code.pointer_power)+2*(0<src.data<2>()->type_code.pointer_power);
	switch(ptr_case)
	{
	case 0:	// can't test from preprocessor
		if (!converts_to_reallike(src.data<1>()->type_code.base_type_index ARG_TYPES) || !converts_to_reallike(src.data<2>()->type_code.base_type_index ARG_TYPES))
			{
			simple_error(src," compares non-real type(s) (C99 6.5.8p2/C++98 5.9p2)");
			return false;
			}
		break;
	case 1:	//! \todo need floating-point literal to test first half
			//! \todo figure out how to test second half
		if (!converts_to_integer(src.data<2>()->type_code ARG_TYPES) || !(PARSE_PRIMARY_EXPRESSION & src.data<2>()->flags))
			{	// oops
			simple_error(src," compares pointer to something not an integer literal or pointer (C99 6.5.8p2/C++98 4.10p1,5.9p2)");
			return false;
			}
		break;
	case 2:	//! \todo need floating-point literal to test first half
			//! \todo figure out how to test second half
		if (!converts_to_integer(src.data<1>()->type_code ARG_TYPES) || !(PARSE_PRIMARY_EXPRESSION & src.data<1>()->flags))
			{	// oops
			simple_error(src," compares pointer to something not an integer literal or pointer (C99 6.5.8p2/C++98 4.10p1,5.9p2)");
			return false;
			}
		break;
	}
	return true;
}

static void C_relation_expression_easy_syntax_check(parse_tree& src,const type_system& types)
{
	if (!C_CPP_relation_expression_core_syntax_ok(src,types))
		eval_relation_expression(src,types,C99_intlike_literal_to_VM);
}

static void CPP_relation_expression_easy_syntax_check(parse_tree& src,const type_system& types)
{	//! \todo handle operator overloading
	if (!C_CPP_relation_expression_core_syntax_ok(src,types))
		eval_relation_expression(src,types,CPP_intlike_literal_to_VM);
}

/*
relational-expression:
	shift-expression
	relational-expression < shift-expression
	relational-expression > shift-expression
	relational-expression <= shift-expression
	relational-expression >= shift-expression
*/
//! \throw std::bad_alloc
static void locate_C99_relation_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_relation_expression(src,i))
		C_relation_expression_easy_syntax_check(src.c_array<0>()[i],types);
}

/*
relational-expression:
	shift-expression
	relational-expression < shift-expression
	relational-expression > shift-expression
	relational-expression <= shift-expression
	relational-expression >= shift-expression
*/
//! \throw std::bad_alloc
static void locate_CPP_relation_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_relation_expression(src,i))
		//! \todo handle overloading
		CPP_relation_expression_easy_syntax_check(src.c_array<0>()[i],types);
}

//! \throw std::bad_alloc
static bool terse_locate_C99_equality_expression(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	const size_t eq_subtype = (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"==")) ? C99_EQUALITY_SUBTYPE_EQ
							: (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"!=")) ? C99_EQUALITY_SUBTYPE_NEQ : 0;
	if (eq_subtype)
		{
		if (1>i || 2>src.size<0>()-i) return false;
		parse_tree* const tmp_c_array = src.c_array<0>()+(i-1);
		inspect_potential_paren_primary_expression(tmp_c_array[0]);
		inspect_potential_paren_primary_expression(tmp_c_array[2]);
		if (	(PARSE_EQUALITY_EXPRESSION & tmp_c_array[0].flags)
			&&	(PARSE_RELATIONAL_EXPRESSION & tmp_c_array[2].flags))
			{
			assemble_binary_infix_arguments(src,i,PARSE_STRICT_EQUALITY_EXPRESSION);	// tmp_c_array becomes invalid here
			assert(is_C99_equality_expression(src.data<0>()[i]));
			parse_tree& tmp = src.c_array<0>()[i];
			tmp.subtype = eq_subtype;
			tmp.type_code.set_type(C_TYPE::BOOL);
			assert(is_C99_equality_expression(src.data<0>()[i]));
			return true;
			}
		}
	return false;
}

//! \throw std::bad_alloc
static bool terse_locate_CPP_equality_expression(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	const size_t eq_subtype = (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"==")) ? C99_EQUALITY_SUBTYPE_EQ
							: (		token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"!=")
							   ||	token_is_string<6>(src.data<0>()[i].index_tokens[0].token,"not_eq")) ? C99_EQUALITY_SUBTYPE_NEQ : 0;
	if (eq_subtype)
		{
		if (1>i || 2>src.size<0>()-i) return false;
		parse_tree* const tmp_c_array = src.c_array<0>()+(i-1);
		inspect_potential_paren_primary_expression(tmp_c_array[0]);
		inspect_potential_paren_primary_expression(tmp_c_array[2]);
		if (	(PARSE_EQUALITY_EXPRESSION & tmp_c_array[0].flags)
			&&	(PARSE_RELATIONAL_EXPRESSION & tmp_c_array[2].flags))
			{
			assemble_binary_infix_arguments(src,i,PARSE_STRICT_EQUALITY_EXPRESSION);	// tmp_c_array becomes invalid here
			assert(is_CPP_equality_expression(src.data<0>()[i]));
			parse_tree& tmp = src.c_array<0>()[i];
			tmp.subtype = eq_subtype;
			tmp.type_code.set_type(C_TYPE::BOOL);
			assert(is_CPP_equality_expression(src.data<0>()[i]));
			return true;
			}
		}
	return false;
}

static bool eval_equality_expression(parse_tree& src, const type_system& types, literal_converts_to_bool_func& literal_converts_to_bool,intlike_literal_to_VM_func& intlike_literal_to_VM)
{	
	BOOST_STATIC_ASSERT(1==C99_EQUALITY_SUBTYPE_NEQ-C99_EQUALITY_SUBTYPE_EQ);
	assert(C99_EQUALITY_SUBTYPE_EQ<=src.subtype && C99_EQUALITY_SUBTYPE_NEQ>=src.subtype);
	umaxint lhs_int;
	umaxint rhs_int;
	const unsigned int integer_literal_case = 	  converts_to_integer(src.data<1>()->type_code ARG_TYPES)
											+	2*converts_to_integer(src.data<2>()->type_code ARG_TYPES);
	const bool is_equal_op = src.subtype==C99_EQUALITY_SUBTYPE_EQ;
	bool is_true = false;
	switch(integer_literal_case)
	{
	case 0:	{	// string literal == string literal (assume hyper-optimizing linker, this should be true iff the string literals are equal as static arrays of char)
				//! \test default/Pass_if_nonzero.hpp, default/Pass_if_nonzero.h, 
				//! \test default/Pass_if_zero.hpp, default/Pass_if_zero.h, 
			bool is_equal = false;
			if (   C_string_literal_equal_content(*src.data<1>(),*src.data<2>(),is_equal)
				)
				{
				force_decimal_literal(src,is_equal_op==is_equal ? "1" : "0",types);
				return true;
				};
			break;
			}
	case 1:	{
			if (0<src.data<2>()->type_code.pointer_power && literal_converts_to_bool(*src.data<1>(),is_true ARG_TYPES)) 
				{
				if (!is_true)
					{	
					if (src.data<2>()->type_code.decays_to_nonnull_pointer())
						{	// string literal != NULL, etc.
						//! \test default/Pass_if_nonzero.hpp, default/Pass_if_nonzero.h, 
						//! \test default/Pass_if_zero.hpp, default/Pass_if_zero.h, 
						force_decimal_literal(src,is_equal_op ? "0" : "1",types);
						return true;
						}
					}
				else if (!(parse_tree::INVALID & src.flags))
					//! \test default/Error_if_control60.hpp, default/Error_if_control60.h 
					//! \test default/Error_if_control61.hpp, default/Error_if_control61.h
					simple_error(src," compares pointer to integer that is not a null pointer constant (C99 6.5.9p5/C++98 4.10p1,5.10p1)");
				return false;
				}
			break;
			}
	case 2:	{
			if (0<src.data<1>()->type_code.pointer_power && literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES)) 
				{
				if (!is_true)
					{
					if (src.data<1>()->type_code.decays_to_nonnull_pointer())
						{	// string literal != NULL
						//! \test default/Pass_if_nonzero.hpp, default/Pass_if_nonzero.h, 
						//! \test default/Pass_if_zero.hpp, default/Pass_if_zero.h, 
						force_decimal_literal(src,is_equal_op ? "0" : "1",types);
						return true;
						}
					}
				else if (!(parse_tree::INVALID & src.flags))
					//! \test default/Error_if_control62.hpp, default/Error_if_control62.h 
					//! \test default/Error_if_control63.hpp, default/Error_if_control63.h
					simple_error(src," compares pointer to integer that is not a null pointer constant (C99 6.5.9p5/C++98 4.10p1,5.10p1)");
				return false;
				}
			break;
			}
	case 3:	{	// integer literal == integer literal
			const promote_aux lhs(src.data<1>()->type_code.base_type_index ARG_TYPES);
			const promote_aux rhs(src.data<2>()->type_code.base_type_index ARG_TYPES);
			const promote_aux old(arithmetic_reconcile(src.data<1>()->type_code.base_type_index,src.data<2>()->type_code.base_type_index ARG_TYPES) ARG_TYPES);
			assert(old.bitcount>=lhs.bitcount);
			assert(old.bitcount>=rhs.bitcount);
			const bool lhs_converted = intlike_literal_to_VM(lhs_int,*src.data<1>() ARG_TYPES);
			const bool rhs_converted = intlike_literal_to_VM(rhs_int,*src.data<2>() ARG_TYPES);
			// general case here in case we try to do with converted/not converted mixed cases
//			if (lhs_converted) target_machine->C_promote_integer(lhs_int,lhs,old);
//			if (rhs_converted) target_machine->C_promote_integer(rhs_int,rhs,old);
			if (lhs_converted && rhs_converted)
				{
				target_machine->C_promote_integer(lhs_int,lhs,old);
				target_machine->C_promote_integer(rhs_int,rhs,old);
				force_decimal_literal(src,(lhs_int==rhs_int)==is_equal_op ? "1" : "0",types);
				return true;
				};
//			break;
			}
	};
	
	return false;
}

static bool C_CPP_equality_expression_syntax_ok_core(parse_tree& src,const type_system& types)
{	// admit legality of:
	// numeric == numeric
	// string literal == string literal
	// string literal == integer literal zero
	// deny legality of : string literal == integer/float
	// more to come later
	const unsigned int ptr_case = (0<src.data<1>()->type_code.pointer_power)+2*(0<src.data<2>()->type_code.pointer_power);
	switch(ptr_case)
	{
	case 0:	// can't test from preprocessor
		if (C_TYPE::VOID>=src.data<1>()->type_code.base_type_index || C_TYPE::VOID>=src.data<2>()->type_code.base_type_index)
			{
			simple_error(src," can't use a void or indeterminately typed argument");
			return false;
			}
		break;
	case 1:	// need floating-point literal to test from preprocessor
		if (!converts_to_integer(src.data<2>()->type_code ARG_TYPES) || !(PARSE_PRIMARY_EXPRESSION & src.data<2>()->flags))
			{	// oops
			simple_error(src," compares pointer to something not an integer literal or pointer (C99 6.5.9p5/C++98 4.10p1,5.10p1)");
			return false;
			}
		break;
	case 2:	// need floating-point literal to test from preprocessor
		if (!converts_to_integer(src.data<1>()->type_code ARG_TYPES) || !(PARSE_PRIMARY_EXPRESSION & src.data<1>()->flags))
			{	// oops
			simple_error(src," compares pointer to something not an integer literal or pointer (C99 6.5.9p5/C++98 4.10p1,5.10p1)");
			return false;
			}
		break;
	}
	return true;
}

static void C_equality_expression_easy_syntax_check(parse_tree& src,const type_system& types)
{
	if (!C_CPP_equality_expression_syntax_ok_core(src,types)) return;
	eval_equality_expression(src,types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM);
}

static void CPP_equality_expression_easy_syntax_check(parse_tree& src,const type_system& types)
{	//! \todo check for operator overloading
	if (!C_CPP_equality_expression_syntax_ok_core(src,types)) return;
	eval_equality_expression(src,types,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM);
}

/*
equality-expression:
	relational-expression
	equality-expression == relational-expression
	equality-expression != relational-expression
*/
//! \throw std::bad_alloc
static void locate_C99_equality_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_C99_equality_expression(src,i))
		C_equality_expression_easy_syntax_check(src.c_array<0>()[i],types);
}

/*
equality-expression:
	relational-expression
	equality-expression == relational-expression
	equality-expression != relational-expression
*/
//! \throw std::bad_alloc
static void locate_CPP_equality_expression(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_CPP_equality_expression(src,i))
		//! \todo handle operator overloading
		CPP_equality_expression_easy_syntax_check(src.c_array<0>()[i],types);
}

//! \throw std::bad_alloc
static bool terse_locate_C99_bitwise_AND(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	//! \todo deal with unary & parses
	parse_tree* const tmp_c_array = src.c_array<0>()+(i-1);
	if (token_is_char<'&'>(tmp_c_array[1].index_tokens[0].token))
		{
		if (1>i || 2>src.size<0>()-i) return false;
		inspect_potential_paren_primary_expression(tmp_c_array[0]);
		inspect_potential_paren_primary_expression(tmp_c_array[2]);
		if (	(PARSE_BITAND_EXPRESSION & tmp_c_array[0].flags)
			&&	(PARSE_EQUALITY_EXPRESSION & tmp_c_array[2].flags))
			{
			assemble_binary_infix_arguments(src,i,PARSE_STRICT_BITAND_EXPRESSION);	// tmp_c_array becomes invalid here
			assert(is_C99_bitwise_AND_expression(src.data<0>()[i]));
			src.c_array<0>()[i].type_code.set_type(0);	// handle type inference later
			assert(is_C99_bitwise_AND_expression(src.data<0>()[i]));
			return true;
			}
		}
	return false;
}

static bool terse_locate_CPP_bitwise_AND(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	if (token_is_char<'&'>(src.data<0>()[i].index_tokens[0].token) || token_is_string<6>(src.data<0>()[i].index_tokens[0].token,"bitand"))
		{
		if (1>i || 2>src.size<0>()-i) return false;
		inspect_potential_paren_primary_expression(src.c_array<0>()[i-1]);
		inspect_potential_paren_primary_expression(src.c_array<0>()[i+1]);
		if (	(PARSE_BITAND_EXPRESSION & src.data<0>()[i-1].flags)
			&&	(PARSE_EQUALITY_EXPRESSION & src.data<0>()[i+1].flags))
			{
			assemble_binary_infix_arguments(src,i,PARSE_STRICT_BITAND_EXPRESSION);
			assert(is_CPP_bitwise_AND_expression(src.data<0>()[i]));
			src.c_array<0>()[i].type_code.set_type(0);	// handle type inference later
			assert(is_CPP_bitwise_AND_expression(src.data<0>()[i]));
			return true;
			}
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_bitwise_AND(parse_tree& src, const type_system& types, literal_converts_to_bool_func& literal_converts_to_bool,intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	assert(converts_to_integerlike(src.data<1>()->type_code ARG_TYPES));
	assert(converts_to_integerlike(src.data<2>()->type_code ARG_TYPES));
	// handle following:
	// __ & 0 |-> 0
	// 0 & __ |-> 0
	// int-literal | int-literal |-> int-literal *if* both fit
	// unary - gives us problems (result is target-specific, could generate a trap representation)
	bool is_true = false;
	if (	(literal_converts_to_bool(*src.data<1>(),is_true ARG_TYPES) && !is_true)	// 0 & __
		||	(literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES) && !is_true))	// __ & 0
		{
		if (C_TYPE::INTEGERLIKE==src.type_code.base_type_index)
			{
			message_header(src.index_tokens[0]);
			INC_INFORM("invalid ");
			INC_INFORM(src);
			INFORM(" optimized to valid 0");
			force_decimal_literal(src,"0",types);
			src.type_code.set_type(C_TYPE::LLONG);	// legalize
			}
		else{
			type_spec tmp;
			src.type_code.OverwriteInto(tmp);
			force_decimal_literal(src,"0",types);
			tmp.MoveInto(src.type_code);
			}
		return true;
		};

	umaxint lhs_int;
	umaxint rhs_int;
	if (intlike_literal_to_VM(lhs_int,*src.data<1>() ARG_TYPES) && intlike_literal_to_VM(rhs_int,*src.data<2>() ARG_TYPES))
		{
		const promote_aux old(src.type_code.base_type_index ARG_TYPES);
		umaxint res_int(lhs_int);
		res_int &= rhs_int;

		// check for trap representation for signed types
		if (int_has_trapped(src,res_int)) return false;

		if 		(res_int==lhs_int)
			{	// lhs & rhs = lhs; conserve type
			src.type_code.MoveInto(src.c_array<1>()->type_code);
			src.eval_to_arg<1>(0);
			}
		else if (res_int==rhs_int)
			{	// lhs & rhs = rhs; conserve type
			src.type_code.MoveInto(src.c_array<2>()->type_code);
			src.eval_to_arg<2>(0);
			}
		else{
			const bool negative_signed_int = old.is_signed && res_int.test(old.bitcount-1);
			if (negative_signed_int) target_machine->signed_additive_inverse(res_int,old.machine_type);
			if (	virtual_machine::twos_complement==target_machine->C_signed_int_representation()
				&& 	old.is_signed
				&& 	!bool_options[boolopt::int_traps]
				&&	res_int>target_machine->signed_max(old.machine_type))
				{	// trap representation; need to get it into -INT_MAX-1 form
				construct_twos_complement_int_min(src,types,old.machine_type,src);
				return true;
				}

			parse_tree tmp;
			VM_to_signed_literal(tmp,negative_signed_int,res_int,src,types);
			src.type_code.MoveInto(tmp.type_code);
			tmp.MoveInto(src);
			}
		return true;
		}
	return false;
}

//! \throw std::bad_alloc
static void C_bitwise_AND_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_C99_bitwise_AND_expression(src));
	// C99 6.5.10p2: requires being an integer type
	if (binary_infix_failed_integer_arguments(src,"(C99 6.5.10p2)" ARG_TYPES)) return;
	src.type_code.base_type_index = default_promote_type(arithmetic_reconcile(src.data<1>()->type_code.base_type_index,src.data<2>()->type_code.base_type_index ARG_TYPES) ARG_TYPES);
	assert(converts_to_integerlike(src.type_code.base_type_index ARG_TYPES));
	zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
	if (eval_bitwise_AND(src,types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM)) return;
}

//! \throw std::bad_alloc
static void CPP_bitwise_AND_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_CPP_bitwise_AND_expression(src));
	// C++98 5.11p1: requires being an integer or enumeration type
	if (binary_infix_failed_integer_arguments(src,"(C++98 5.11p1)" ARG_TYPES)) return;
	src.type_code.base_type_index = default_promote_type(arithmetic_reconcile(src.data<1>()->type_code.base_type_index,src.data<2>()->type_code.base_type_index ARG_TYPES) ARG_TYPES);
	assert(converts_to_integerlike(src.type_code.base_type_index ARG_TYPES));
	zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
	if (eval_bitwise_AND(src,types,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM)) return;
}

/*
AND-expression:
	equality-expression
	AND-expression & equality-expression
*/
//! \throw std::bad_alloc
static void locate_C99_bitwise_AND(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_C99_bitwise_AND(src,i))
		C_bitwise_AND_easy_syntax_check(src.c_array<0>()[i],types);
}

/*
AND-expression:
	equality-expression
	AND-expression & equality-expression
*/
//! \throw std::bad_alloc
static void locate_CPP_bitwise_AND(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_CPP_bitwise_AND(src,i))
		//! \todo handle overloading
		CPP_bitwise_AND_easy_syntax_check(src.c_array<0>()[i],types);
}

static bool terse_locate_C99_bitwise_XOR(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	parse_tree* const tmp_c_array = src.c_array<0>()+(i-1);
	if (token_is_char<'^'>(tmp_c_array[1].index_tokens[0].token))
		{
		if (1>i || 2>src.size<0>()-i) return false;
		inspect_potential_paren_primary_expression(tmp_c_array[0]);
		inspect_potential_paren_primary_expression(tmp_c_array[2]);
		if (	(PARSE_BITXOR_EXPRESSION & tmp_c_array[0].flags)
			&&	(PARSE_BITAND_EXPRESSION & tmp_c_array[2].flags))
			{
			assemble_binary_infix_arguments(src,i,PARSE_STRICT_BITXOR_EXPRESSION);	// tmp_c_array becomes invalid here
			assert(is_C99_bitwise_XOR_expression(src.data<0>()[i]));
			src.c_array<0>()[i].type_code.set_type(0);	// handle type inference later
			assert(is_C99_bitwise_XOR_expression(src.data<0>()[i]));
			return true;
			}
		}
	return false;
}

static bool terse_locate_CPP_bitwise_XOR(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	if (token_is_char<'^'>(src.data<0>()[i].index_tokens[0].token) || token_is_string<3>(src.data<0>()[i].index_tokens[0].token,"xor"))
		{
		if (1>i || 2>src.size<0>()-i) return false;
		inspect_potential_paren_primary_expression(src.c_array<0>()[i-1]);
		inspect_potential_paren_primary_expression(src.c_array<0>()[i+1]);
		if (	(PARSE_BITXOR_EXPRESSION & src.data<0>()[i-1].flags)
			&&	(PARSE_BITAND_EXPRESSION & src.data<0>()[i+1].flags))
			{
			assemble_binary_infix_arguments(src,i,PARSE_STRICT_BITXOR_EXPRESSION);
			assert(is_CPP_bitwise_XOR_expression(src.data<0>()[i]));
			src.c_array<0>()[i].type_code.set_type(0);	// handle type inference later
			assert(is_CPP_bitwise_XOR_expression(src.data<0>()[i]));
			return true;
			}
		}
	return false;
}

// throws std::bad_alloc
static bool eval_bitwise_XOR(parse_tree& src, const type_system& types, literal_converts_to_bool_func& literal_converts_to_bool,intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	assert(converts_to_integerlike(src.data<1>()->type_code ARG_TYPES));
	assert(converts_to_integerlike(src.data<2>()->type_code ARG_TYPES));
	// handle following
	// x ^ x |-> 0 [later, need sensible detection of "equal" expressions first]
	// 0 ^ __ |-> __
	// __ ^ 0 |-> __
	// also handle double-literal case
	bool is_true = false;
	if (literal_converts_to_bool(*src.data<1>(),is_true ARG_TYPES))
		{
		if (!is_true)
			{	// 0 ^ __
			src.eval_to_arg<2>(0);
			//! \bug convert char literal to appropriate integer
			return true;
			}
		};
	if (literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES))
		{
		if (!is_true)
			{	// __ ^ 0
			src.eval_to_arg<1>(0);
			//! \bug convert char literal to appropriate integer
			return true;
			}
		};

	umaxint lhs_int;
	umaxint rhs_int;
	if (intlike_literal_to_VM(lhs_int,*src.data<1>() ARG_TYPES) && intlike_literal_to_VM(rhs_int,*src.data<2>() ARG_TYPES))
		{
		const promote_aux old(src.type_code.base_type_index ARG_TYPES);
		umaxint res_int(lhs_int);
		res_int ^= rhs_int;
//		res_int.mask_to(target_machine->C_bit(machine_type));	// shouldn't need this

		if (int_has_trapped(src,res_int)) return false;

		const bool negative_signed_int = old.is_signed && res_int.test(old.bitcount-1);
		if (negative_signed_int) target_machine->signed_additive_inverse(res_int,old.machine_type);
		if (	virtual_machine::twos_complement==target_machine->C_signed_int_representation()
			&& 	old.is_signed
			&& 	!bool_options[boolopt::int_traps]
			&&	res_int>target_machine->signed_max(old.machine_type))
			{	// trap representation; need to get it into -INT_MAX-1 form
			construct_twos_complement_int_min(src,types,old.machine_type,src);
			return true;
			}

		parse_tree tmp;
		VM_to_signed_literal(tmp,negative_signed_int,res_int,src,types);
		src.type_code.MoveInto(tmp.type_code);
		tmp.MoveInto(src);
		return true;
		}
	return false;
}

// throws std::bad_alloc
static void C_bitwise_XOR_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_C99_bitwise_XOR_expression(src));
	// C99 6.5.11p2: requires being an integer type
	if (binary_infix_failed_integer_arguments(src,"(C99 6.5.11p2)" ARG_TYPES)) return;
	src.type_code.base_type_index = default_promote_type(arithmetic_reconcile(src.data<1>()->type_code.base_type_index,src.data<2>()->type_code.base_type_index ARG_TYPES) ARG_TYPES);
	assert(converts_to_integerlike(src.type_code.base_type_index ARG_TYPES));
	zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
	if (eval_bitwise_XOR(src,types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM)) return;
}

// throws std::bad_alloc
static void CPP_bitwise_XOR_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_CPP_bitwise_XOR_expression(src));
	// C++98 5.12p1: requires being an integer or enumeration type
	if (binary_infix_failed_integer_arguments(src,"(C++98 5.12p1)" ARG_TYPES)) return;
	src.type_code.base_type_index = default_promote_type(arithmetic_reconcile(src.data<1>()->type_code.base_type_index,src.data<2>()->type_code.base_type_index ARG_TYPES) ARG_TYPES);
	assert(converts_to_integerlike(src.type_code.base_type_index ARG_TYPES));
	zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
	if (eval_bitwise_XOR(src,types,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM)) return;
}

/*
exclusive-OR-expression:
	AND-expression
	exclusive-OR-expression ^ AND-expression
*/
// throws std::bad_alloc
static void locate_C99_bitwise_XOR(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_C99_bitwise_XOR(src,i)) C_bitwise_XOR_easy_syntax_check(src.c_array<0>()[i],types);
}

/*
exclusive-OR-expression:
	AND-expression
	exclusive-OR-expression ^ AND-expression
*/
// throws std::bad_alloc
static void locate_CPP_bitwise_XOR(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_CPP_bitwise_XOR(src,i))
		//! \todo handle operator overloading
		CPP_bitwise_XOR_easy_syntax_check(src.c_array<0>()[i],types);
}

static bool terse_locate_C99_bitwise_OR(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	parse_tree* const tmp_c_array = src.c_array<0>()+(i-1);
	if (token_is_char<'|'>(tmp_c_array[1].index_tokens[0].token))
		{
		if (1>i || 2>src.size<0>()-i) return false;
		inspect_potential_paren_primary_expression(tmp_c_array[0]);
		inspect_potential_paren_primary_expression(tmp_c_array[2]);
		if (	(PARSE_BITOR_EXPRESSION & tmp_c_array[0].flags)
			&&	(PARSE_BITXOR_EXPRESSION & tmp_c_array[2].flags))
			{
			assemble_binary_infix_arguments(src,i,PARSE_STRICT_BITOR_EXPRESSION);	// tmp_c_array becomes invalid here
			assert(is_C99_bitwise_OR_expression(src.data<0>()[i]));
			src.c_array<0>()[i].type_code.set_type(0);	// handle type inference later
			assert(is_C99_bitwise_OR_expression(src.data<0>()[i]));
			return true;
			}
		}
	return false;
}

static bool terse_locate_CPP_bitwise_OR(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	if (token_is_char<'|'>(src.data<0>()[i].index_tokens[0].token) || token_is_string<5>(src.data<0>()[i].index_tokens[0].token,"bitor"))
		{
		if (1>i || 2>src.size<0>()-i) return false;
		inspect_potential_paren_primary_expression(src.c_array<0>()[i-1]);
		inspect_potential_paren_primary_expression(src.c_array<0>()[i+1]);
		if (	(PARSE_BITOR_EXPRESSION & src.data<0>()[i-1].flags)
			&&	(PARSE_BITXOR_EXPRESSION & src.data<0>()[i+1].flags))
			{
			assemble_binary_infix_arguments(src,i,PARSE_STRICT_BITOR_EXPRESSION);
			assert(is_CPP_bitwise_OR_expression(src.data<0>()[i]));
			src.c_array<0>()[i].type_code.set_type(0);	// handle type inference later
			assert(is_CPP_bitwise_OR_expression(src.data<0>()[i]));
			return true;
			}
		}
	return false;
}

//! \throw std::bad_alloc()
static bool eval_bitwise_OR(parse_tree& src, const type_system& types, literal_converts_to_bool_func& literal_converts_to_bool,intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	assert(converts_to_integerlike(src.data<1>()->type_code ARG_TYPES));
	assert(converts_to_integerlike(src.data<2>()->type_code ARG_TYPES));
	// handle following:
	// __ | 0 |-> __
	// 0 | __ |-> __
	// int-literal | int-literal |-> int-literal *if* both fit
	// unary - gives us problems (result is target-specific, could generate a trap representation)
	bool is_true = false;
	if (literal_converts_to_bool(*src.data<1>(),is_true ARG_TYPES))
		{
		if (!is_true)
			{	// 0 | __
			src.eval_to_arg<2>(0);
			//! \bug convert char literal to appropriate integer
			return true;
			}
		};
	if (literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES))
		{
		if (!is_true)
			{	// __ | 0
			src.eval_to_arg<1>(0);
			//! \bug convert char literal to appropriate integer
			return true;
			}
		};

	umaxint lhs_int;
	umaxint rhs_int;
	if (intlike_literal_to_VM(lhs_int,*src.data<1>() ARG_TYPES) && intlike_literal_to_VM(rhs_int,*src.data<2>() ARG_TYPES))
		{
		umaxint res_int(lhs_int);

		res_int |= rhs_int;
//		res_int.mask_to(target_machine->C_bit(machine_type));	// shouldn't need this
		if 		(res_int==lhs_int)
			{	// lhs | rhs = lhs; conserve type
			src.type_code.MoveInto(src.c_array<1>()->type_code);
			src.eval_to_arg<1>(0);
			}
		else if (res_int==rhs_int)
			{	// lhs | rhs = rhs; conserve type
			src.type_code.MoveInto(src.c_array<2>()->type_code);
			src.eval_to_arg<2>(0);
			}
		else{
			if (int_has_trapped(src,res_int)) return false;

			const virtual_machine::std_int_enum machine_type = (virtual_machine::std_int_enum)((src.type_code.base_type_index-C_TYPE::INT)/2+virtual_machine::std_int_int);
			const bool negative_signed_int = 0==(src.type_code.base_type_index-C_TYPE::INT)%2 && res_int.test(target_machine->C_bit(machine_type)-1);
			if (negative_signed_int) target_machine->signed_additive_inverse(res_int,machine_type);
			parse_tree tmp;
			VM_to_literal(tmp,res_int,src,types);

			if (negative_signed_int)
				{	// convert to parsed - literal
				src.DeleteIdx<1>(0);
				force_unary_negative_literal(src,tmp);
				}
			else{	// convert to positive literal
				src.type_code.MoveInto(tmp.type_code);
				tmp.MoveInto(src);
				}
			}
		return true;
		}
	return false;
}

//! \throw std::bad_alloc()
static void C_bitwise_OR_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_C99_bitwise_OR_expression(src));
	// C99 6.5.12p2: requires being an integer type
	if (binary_infix_failed_integer_arguments(src,"(C99 6.5.12p2)" ARG_TYPES)) return;
	src.type_code.base_type_index = arithmetic_reconcile(src.data<1>()->type_code.base_type_index,src.data<2>()->type_code.base_type_index ARG_TYPES);
	assert(converts_to_integerlike(src.type_code.base_type_index ARG_TYPES));
	zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
	if (eval_bitwise_OR(src,types,C99_literal_converts_to_bool,C99_intlike_literal_to_VM)) return;
}

//! \throw std::bad_alloc()
static void CPP_bitwise_OR_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_CPP_bitwise_OR_expression(src));
	// C++98 5.13p1: requires being an integer or enumeration type
	if (binary_infix_failed_integer_arguments(src,"(C++98 5.13p1)" ARG_TYPES)) return;
	src.type_code.base_type_index = arithmetic_reconcile(src.data<1>()->type_code.base_type_index,src.data<2>()->type_code.base_type_index ARG_TYPES);
	assert(converts_to_integerlike(src.type_code.base_type_index ARG_TYPES));
	zaimoni::simple_lock<unsigned int> lock(no_runtime_errors);
	if (eval_bitwise_OR(src,types,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM)) return;
}

/*
inclusive-OR-expression:
	exclusive-OR-expression
	inclusive-OR-expression | exclusive-OR-expression
*/
//! \throw std::bad_alloc()
static void locate_C99_bitwise_OR(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_C99_bitwise_OR(src,i))
		C_bitwise_OR_easy_syntax_check(src.c_array<0>()[i],types);
}

/*
inclusive-OR-expression:
	exclusive-OR-expression
	inclusive-OR-expression | exclusive-OR-expression
*/
//! \throw std::bad_alloc()
static void locate_CPP_bitwise_OR(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_CPP_bitwise_OR(src,i))
		//! \todo handle overloading
		CPP_bitwise_OR_easy_syntax_check(src.c_array<0>()[i],types);
}

static bool binary_infix_failed_boolean_arguments(parse_tree& src, const char* standard SIG_CONST_TYPES)
{	//! \todo so the error message isn't technically right...convertible to bool in C++ is morally equivalent to scalar in C
	// cannot test this within preprocessor
	assert(standard && *standard);

	const bool rhs_converts_to_bool =  converts_to_bool(src.data<2>()->type_code ARG_TYPES);
	if (!converts_to_bool(src.data<1>()->type_code ARG_TYPES))
		{
		simple_error(src,rhs_converts_to_bool ? " has nonscalar LHS" : " has nonscalar LHS and RHS");
		INFORM(standard);
		return true;
		}
	else if (!rhs_converts_to_bool)
		{
		simple_error(src," has nonscalar RHS");
		INFORM(standard);
		return true;
		}
	return false;
}

static bool terse_locate_C99_logical_AND(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	parse_tree* const tmp_c_array = src.c_array<0>()+(i-1);
	if (token_is_string<2>(tmp_c_array[1].index_tokens[0].token,"&&"))
		{
		if (1>i || 2>src.size<0>()-i) return false;
		inspect_potential_paren_primary_expression(tmp_c_array[0]);
		inspect_potential_paren_primary_expression(tmp_c_array[2]);
		if (	(PARSE_LOGICAND_EXPRESSION & tmp_c_array[0].flags)
			&&	(PARSE_BITOR_EXPRESSION & tmp_c_array[2].flags))
			{
			assemble_binary_infix_arguments(src,i,PARSE_STRICT_LOGICAND_EXPRESSION);	// tmp_c_array becomes invalid here
			assert(is_C99_logical_AND_expression(src.data<0>()[i]));
			src.c_array<0>()[i].type_code.set_type(C_TYPE::BOOL);	// technically wrong, but range is correct
			assert(is_C99_logical_AND_expression(src.data<0>()[i]));
			return true;
			}
		}
	return false;
}

static bool terse_locate_CPP_logical_AND(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	if (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"&&") || token_is_string<3>(src.data<0>()[i].index_tokens[0].token,"and"))
		{
		if (1>i || 2>src.size<0>()-i) return false;
		inspect_potential_paren_primary_expression(src.c_array<0>()[i-1]);
		inspect_potential_paren_primary_expression(src.c_array<0>()[i+1]);
		if (	(PARSE_LOGICAND_EXPRESSION & src.data<0>()[i-1].flags)
			&&	(PARSE_BITOR_EXPRESSION & src.data<0>()[i+1].flags))
			{
			assemble_binary_infix_arguments(src,i,PARSE_STRICT_LOGICAND_EXPRESSION);
			assert(is_CPP_logical_AND_expression(src.data<0>()[i]));
			//! \todo handle overloading
			src.c_array<0>()[i].type_code.set_type(C_TYPE::BOOL);
			assert(is_CPP_logical_AND_expression(src.data<0>()[i]));
			return true;
			}
		}
	return false;
}

static bool eval_logical_AND(parse_tree& src, const type_system& types, literal_converts_to_bool_func& literal_converts_to_bool)
{
	// deal with literals here.  && short-circuit evaluates.
	// 1 && __ |-> 0!=__
	// 0 && __ |-> 0
	// __ && 1 |-> 0!=__
	// __ && 0 |-> __ && 0 (__ may have side effects...), *but* does "stop the buck" so
	// (__ && 1) && __ |-> __ && 1

	bool is_true = false;
	if (literal_converts_to_bool(*src.data<1>(),is_true ARG_TYPES))
		{	// one of 0 && __ or 1 && __
		if (!is_true)
			{	// 0 && __
			if (src.flags & parse_tree::INVALID)
				{
				message_header(src.index_tokens[0]);
				INC_INFORM("invalid ");
				INC_INFORM(src);
				INFORM(" optimized to valid 0");
				};
			force_decimal_literal(src,"0",types);
			return true;
			}
		else if (literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES))
			{	// 1 && 1 or 1 && 0
			force_decimal_literal(src,is_true ? "1" : "0",types);
			return true;
			}
#if 0
		//! \todo fixup 1 && __ when we have != and are working on C/C++ proper; anything already type bool could be reduced now
		else{
			}
#endif
		};
#if 0
		//! \todo fixup (__ && 1) && __ when we are working on C/C++ proper
#endif
	return false;
}

static void C_logical_AND_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_C99_logical_AND_expression(src));
	if (binary_infix_failed_boolean_arguments(src,"(C99 6.5.13p2)" ARG_TYPES)) return;

	if (eval_logical_AND(src,types,C99_literal_converts_to_bool)) return;
}

static void CPP_logical_AND_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_CPP_logical_AND_expression(src));
	if (binary_infix_failed_boolean_arguments(src,"(C++98 5.14p1)" ARG_TYPES)) return;

	if (eval_logical_AND(src,types,CPP_literal_converts_to_bool)) return;
}

/*
logical-AND-expression:
	inclusive-OR-expression
	logical-AND-expression && inclusive-OR-expression
*/
static void locate_C99_logical_AND(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_C99_logical_AND(src,i))
		C_logical_AND_easy_syntax_check(src.c_array<0>()[i],types);
}

/*
logical-AND-expression:
	inclusive-OR-expression
	logical-AND-expression && inclusive-OR-expression
*/
static void locate_CPP_logical_AND(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_CPP_logical_AND(src,i))
		//! \todo check for operator overloading
		CPP_logical_AND_easy_syntax_check(src.c_array<0>()[i],types);
}

static bool terse_locate_C99_logical_OR(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	parse_tree* const tmp_c_array = src.c_array<0>()+(i-1);
	if (token_is_string<2>(tmp_c_array[1].index_tokens[0].token,"||"))
		{
		if (1>i || 2>src.size<0>()-i) return false;
		inspect_potential_paren_primary_expression(tmp_c_array[0]);
		inspect_potential_paren_primary_expression(tmp_c_array[2]);
		if (	(PARSE_LOGICOR_EXPRESSION & tmp_c_array[0].flags)
			&&	(PARSE_LOGICAND_EXPRESSION & tmp_c_array[2].flags))
			{
			assemble_binary_infix_arguments(src,i,PARSE_STRICT_LOGICOR_EXPRESSION);	// tmp_c_array becomes invalid here
			assert(is_C99_logical_OR_expression(src.data<0>()[i]));
			src.c_array<0>()[i].type_code.set_type(C_TYPE::BOOL);	// technically wrong, but range is correct
			assert(is_C99_logical_OR_expression(src.data<0>()[i]));
			return true;
			}
		}
	return false;
}

static bool terse_locate_CPP_logical_OR(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	if (token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"||") || token_is_string<2>(src.data<0>()[i].index_tokens[0].token,"or"))
		{
		if (1>i || 2>src.size<0>()-i) return false;
		inspect_potential_paren_primary_expression(src.c_array<0>()[i-1]);
		inspect_potential_paren_primary_expression(src.c_array<0>()[i+1]);
		if (	(PARSE_LOGICOR_EXPRESSION & src.data<0>()[i-1].flags)
			&&	(PARSE_LOGICAND_EXPRESSION & src.data<0>()[i+1].flags))
			{
			assemble_binary_infix_arguments(src,i,PARSE_STRICT_LOGICOR_EXPRESSION);
			assert(is_CPP_logical_OR_expression(src.data<0>()[i]));
			src.c_array<0>()[i].type_code.set_type(C_TYPE::BOOL);
			assert(is_CPP_logical_OR_expression(src.data<0>()[i]));
			return true;
			}
		}
	return false;
}

static bool eval_logical_OR(parse_tree& src, const type_system& types, literal_converts_to_bool_func& literal_converts_to_bool)
{
	// deal with literals here.  || short-circuit evaluates.
	// 0 || __ |-> 0!=__
	// 1 || __ |-> 1
	// __ || 0 |-> 0!=__
	// __ || 1 |-> __ || 1 (__ may have side effects...), *but* does "stop the buck" so
	// (__ || 1) || __ |-> __ || 1

	bool is_true = false;
	if (literal_converts_to_bool(*src.data<1>(),is_true ARG_TYPES))
		{	// one of 0 || __ or 1 || __
		if (is_true)
			{	// 1 || __
			if (src.flags & parse_tree::INVALID)
				{
				message_header(src.index_tokens[0]);
				INC_INFORM("invalid ");
				INC_INFORM(src);
				INFORM(" optimized to valid 1");
				};
			force_decimal_literal(src,"1",types);
			return true;
			}
		else if (literal_converts_to_bool(*src.data<2>(),is_true ARG_TYPES))
			{	// 0 || 1 or 0 || 0
			force_decimal_literal(src,is_true ? "1" : "0",types);
			return true;
			}
#if 0
		//! \todo fixup 0 || __ when we have != and are working on C/C++ proper; anything already type bool could be reduced now
		else{
			}
#endif
		};
#if 0
		//! \todo fixup (__ || 1) || __ when we are working on C/C++ proper
#endif
	return false;
}

static void C_logical_OR_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_C99_logical_OR_expression(src));
	if (binary_infix_failed_boolean_arguments(src,"(C99 6.5.14p2)" ARG_TYPES)) return;

	if (eval_logical_OR(src,types,C99_literal_converts_to_bool)) return;
}

static void CPP_logical_OR_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_CPP_logical_OR_expression(src));
	if (binary_infix_failed_boolean_arguments(src,"(C++98 5.15p1)" ARG_TYPES)) return;

	if (eval_logical_OR(src,types,CPP_literal_converts_to_bool)) return;
}

/*
logical-OR-expression:
	logical-AND-expression
	logical-OR-expression || logical-AND-expression
*/
static void locate_C99_logical_OR(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_C99_logical_OR(src,i))
		C_logical_OR_easy_syntax_check(src.c_array<0>()[i],types);
}

/*
logical-OR-expression:
	logical-AND-expression
	logical-OR-expression || logical-AND-expression
*/
static void locate_CPP_logical_OR(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_CPP_logical_OR(src,i))
		//! \todo check for operator overloading
		CPP_logical_OR_easy_syntax_check(src.c_array<0>()[i],types);
}

//! \throw std::bad_alloc
static bool terse_locate_conditional_op(parse_tree& src, size_t& i)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	assert(!(PARSE_OBVIOUS & src.data<0>()[i].flags));
	assert(src.data<0>()[i].is_atomic());

	if (token_is_char<'?'>(src.data<0>()[i].index_tokens[0].token))
		{
		// ? as first might be space deficiency (check uniqueness of construction)
		if (1>i || 3>src.size<0>()-i) return false;
		parse_tree* const tmp_c_array = src.c_array<0>()+(i-1);
		if (	tmp_c_array[3].is_atomic()
			&&	token_is_char<':'>(tmp_c_array[3].index_tokens[0].token))
			{
			inspect_potential_paren_primary_expression(tmp_c_array[0]);
			inspect_potential_paren_primary_expression(tmp_c_array[2]);
			inspect_potential_paren_primary_expression(tmp_c_array[4]);
			if (	(PARSE_LOGICOR_EXPRESSION & src.data<0>()[i-1].flags)
				&&	(PARSE_EXPRESSION & src.data<0>()[i+1].flags)
				&&	(PARSE_CONDITIONAL_EXPRESSION & src.data<0>()[i+3].flags))
				{
				zaimoni::autoval_ptr<parse_tree> tmp;
				zaimoni::autoval_ptr<parse_tree> tmp2;
				tmp = repurpose_inner_parentheses(tmp_c_array[0]);	// RAM conservation
				tmp2 = repurpose_inner_parentheses(tmp_c_array[2]);	// RAM conservation
				parse_tree* const tmp3 = repurpose_inner_parentheses(tmp_c_array[4]);	// RAM conservation
				tmp_c_array[0].OverwriteInto(*tmp);
				tmp_c_array[2].OverwriteInto(*tmp2);
				tmp_c_array[4].OverwriteInto(*tmp3);
				tmp_c_array[1].grab_index_token_from<1,0>(tmp_c_array[3]);
				tmp_c_array[1].grab_index_token_location_from<1,0>(tmp_c_array[3]);
				tmp_c_array[1].fast_set_arg<0>(tmp2.release());
				tmp_c_array[1].fast_set_arg<1>(tmp.release());
				tmp_c_array[1].fast_set_arg<2>(tmp3);
				tmp_c_array[1].core_flag_update();
				tmp_c_array[1].flags |= PARSE_STRICT_CONDITIONAL_EXPRESSION;
				src.DeleteNSlotsAt<0>(3,i+1);	// tmp_c_array becomes invalid here
				src.DeleteIdx<0>(--i);
				assert(is_C99_conditional_operator_expression_strict(src.data<0>()[i]));
				parse_tree& tmp4 = src.c_array<0>()[i];
				cancel_outermost_parentheses(tmp4.front<0>());
				cancel_outermost_parentheses(tmp4.front<1>());
				cancel_outermost_parentheses(tmp4.front<2>());
				assert(is_C99_conditional_operator_expression(src.data<0>()[i]));
				return true;
				}
			}
		}
	return false;
}

static bool eval_conditional_op(parse_tree& src, literal_converts_to_bool_func& literal_converts_to_bool SIG_CONST_TYPES)
{
	bool is_true = false;
	if (literal_converts_to_bool(*src.c_array<1>(),is_true ARG_TYPES))
		{
		const bool was_invalid = src.flags & parse_tree::INVALID;
		if (is_true)
			{	// it's the infix arg
			src.type_code.MoveInto(src.c_array<0>()->type_code);
			src.eval_to_arg<0>(0);
			}
		else{	// it's the postfix arg
			src.type_code.MoveInto(src.c_array<2>()->type_code);
			src.eval_to_arg<2>(0);
			};
		if (was_invalid && !(src.flags & parse_tree::INVALID))
			{
			message_header(src.index_tokens[0]);
			INC_INFORM("invalid ? : operator optimized to valid ");
			INFORM(src);
			}
		return true;
		}
	return false;
}

//! \throws std::bad_alloc
static void C_conditional_op_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_C99_conditional_operator_expression(src));
	// \todo 1) infix, postfix args have common non-void type (controls whether expression has valid type)
	// \todo change target for multidimensional arrays
	// \todo change target for const/volatile/restricted pointers
	// NOTE: result is always an rvalue in C (C99 6.5.15p4)
	switch(cmp(src.data<0>()->type_code.pointer_power,src.data<2>()->type_code.pointer_power))
	{
	case 1:	{	// LHS has more guaranteed indirectability than RHS
			if (C_TYPE::NOT_VOID==src.data<2>()->type_code.base_type_index)
				{	// recoverable
				src.type_code.set_type(C_TYPE::NOT_VOID);
				src.type_code.set_pointer_power(src.data<0>()->type_code.pointer_power);
				}
			else if (is_null_pointer_constant(*src.data<2>(),C99_intlike_literal_to_VM ARG_TYPES))
				// (...) ? string : 0 -- do *not* error (null pointer); check true/false status
				//! \test default/Pass_if_zero.h, default/Pass_if_zero.hpp 
				// actually, could be either 1 (positively is null pointer constant) or -1 (could be).  We do the same thing in either case.
				value_copy(src.type_code,src.data<0>()->type_code);
			else{
				src.type_code.set_type(0);	// incoherent type
				// (...) ? string : int -- error
				//! \test default/Error_if_control64.h 
				simple_error(src," has incoherent type");
				}
			break;
			}
	case -1:{	// LHS has less guaranteed indirectability than RHS
			if (C_TYPE::NOT_VOID==src.data<0>()->type_code.base_type_index)
				{	// recoverable
				src.type_code.set_type(C_TYPE::NOT_VOID);
				src.type_code.set_pointer_power(src.data<2>()->type_code.pointer_power);
				}
			else if (is_null_pointer_constant(*src.data<0>(),C99_intlike_literal_to_VM ARG_TYPES))
				// (...) ? 0 : string -- do *not* error (null pointer); check true/false status
				//! \test default/Pass_if_zero.h, default/Pass_if_zero.hpp 
				// actually, could be either 1 (positively is null pointer constant) or -1 (could be).  We do the same thing in either case.
				value_copy(src.type_code,src.data<2>()->type_code);
			else{
				src.type_code.set_type(0);	// incoherent type
				// (...) ? int : string -- error
				//! \test default/Error_if_control65.h
				simple_error(src," has incoherent type");
				}
			break;
			}
	case 0:	{
			if (src.data<0>()->type_code.base_type_index==src.data<2>()->type_code.base_type_index)
				{
				src.type_code.set_type(src.data<0>()->type_code.base_type_index);
				src.type_code.set_pointer_power(src.data<0>()->type_code.pointer_power);
				}
			else if (0==src.data<0>()->type_code.pointer_power && (C_TYPE::VOID>=src.data<0>()->type_code.base_type_index || C_TYPE::VOID>=src.data<2>()->type_code.base_type_index))
				{	// can't test this from preprocessor
				src.type_code.set_type(0);	// incoherent type
				simple_error(src," has incoherent type");
				}
			//! \todo test cases at preprocessor level
			else if (0==src.data<0>()->type_code.pointer_power && is_innate_definite_type(src.data<0>()->type_code.base_type_index) && is_innate_definite_type(src.data<2>()->type_code.base_type_index))
				// standard arithmetic conversions
				src.type_code.set_type(arithmetic_reconcile(src.data<0>()->type_code.base_type_index,src.data<2>()->type_code.base_type_index ARG_TYPES));
			//! \todo --do-what-i-mean can handle elementary integer types with same indirection as well
			else if (C_TYPE::NOT_VOID==src.data<0>()->type_code.base_type_index || C_TYPE::NOT_VOID==src.data<2>()->type_code.base_type_index)
				{
				src.type_code.set_type(C_TYPE::NOT_VOID);
				src.type_code.set_pointer_power(src.data<0>()->type_code.pointer_power);
				}
			else{	// can't test this from preprocessor
				src.type_code.set_type(0);	// incoherent type
				simple_error(src," has incoherent type");
				}
			break;
			}
	}

	// 2) prefix arg type convertible to _Bool (control whether expression is evaluatable at all)
	if (!converts_to_bool(src.data<1>()->type_code ARG_TYPES))
		{	// can't test this from preprocessor
		simple_error(src," has nonscalar control expression");
		return;
		}
	// 3) RAM conservation: if we have a suitable literal Do It Now
	// \todo disable this at O0?
	if (eval_conditional_op(src,C99_literal_converts_to_bool ARG_TYPES)) return;
}

//! \throws std::bad_alloc
static void CPP_conditional_op_easy_syntax_check(parse_tree& src,const type_system& types)
{
	assert(is_C99_conditional_operator_expression(src));
	// C++98/C++0x 13.3.1.2p1 mentions that overloading ? : is prohibited
	// \todo 1) infix, postfix args have common non-void type (controls whether expression has valid type); watch out for throw expressions
	// \todo change target for throw expressions
	// \todo change target for multidimensional arrays
	// \todo change target for const/volatile/restricted pointers
	// NOTE: result is an lvalue if both are lvalues of identical type (C++98 5.16p4)
	// NOTE: throw expressions play nice (they always have the type of the other half)
	switch(cmp(src.data<0>()->type_code.pointer_power,src.data<2>()->type_code.pointer_power))
	{
	case 1:	{	// LHS has more guaranteed indirectability than RHS
			if (C_TYPE::NOT_VOID==src.data<2>()->type_code.base_type_index)
				{	// recoverable
				src.type_code.set_type(C_TYPE::NOT_VOID);
				src.type_code.set_pointer_power(src.data<0>()->type_code.pointer_power);
				}
			else if (is_null_pointer_constant(*src.data<2>(),CPP_intlike_literal_to_VM ARG_TYPES))
				// (...) ? string : 0 -- do *not* error (null pointer); check true/false status
				//! \test default/Pass_if_zero.h, default/Pass_if_zero.hpp 
				// actually, could be either 1 (positively is null pointer constant) or -1 (could be).  We do the same thing in either case.
				value_copy(src.type_code,src.data<0>()->type_code);
			else{
				src.type_code.set_type(0);	// incoherent type
				// (...) ? string : int -- error
				//! \test default/Error_if_control64.hpp
				simple_error(src," has incoherent type");
				}
			break;
			}
	case -1:{	// LHS has less guaranteed indirectability than RHS
			if (C_TYPE::NOT_VOID==src.data<0>()->type_code.base_type_index)
				{	// recoverable
				src.type_code.set_type(C_TYPE::NOT_VOID);
				src.type_code.set_pointer_power(src.data<2>()->type_code.pointer_power);
				}
			else if (is_null_pointer_constant(*src.data<0>(),CPP_intlike_literal_to_VM ARG_TYPES))
				// (...) ? 0 : string -- do *not* error (null pointer); check true/false status
				//! \test default/Pass_if_zero.h, default/Pass_if_zero.hpp 
				// actually, could be either 1 (positively is null pointer constant) or -1 (could be).  We do the same thing in either case.
				value_copy(src.type_code,src.data<2>()->type_code);
			else{
				src.type_code.set_type(0);	// incoherent type
				// (...) ? int : string -- error
				//! \test default/Error_if_control65.hpp
				simple_error(src," has incoherent type");
				}
			break;
			}
	case 0:	{
			if (src.data<0>()->type_code.base_type_index==src.data<2>()->type_code.base_type_index)
				{
				src.type_code.set_type(src.data<0>()->type_code.base_type_index);
				src.type_code.set_pointer_power(src.data<0>()->type_code.pointer_power);
				}
			else if (0==src.data<0>()->type_code.pointer_power && (C_TYPE::VOID>=src.data<0>()->type_code.base_type_index || C_TYPE::VOID>=src.data<2>()->type_code.base_type_index))
				{	// can't test this from preprocessor
				src.type_code.set_type(0);	// incoherent type
				simple_error(src," has incoherent type");
				}
			else if (0==src.data<0>()->type_code.pointer_power && is_innate_definite_type(src.data<0>()->type_code.base_type_index) && is_innate_definite_type(src.data<2>()->type_code.base_type_index))
				// standard arithmetic conversions
				src.type_code.set_type(arithmetic_reconcile(src.data<0>()->type_code.base_type_index,src.data<2>()->type_code.base_type_index ARG_TYPES));
			//! \todo --do-what-i-mean can handle elementary integer types with same indirection as well
			else if (C_TYPE::NOT_VOID==src.data<0>()->type_code.base_type_index || C_TYPE::NOT_VOID==src.data<2>()->type_code.base_type_index)
				{
				src.type_code.set_type(C_TYPE::NOT_VOID);
				src.type_code.set_pointer_power(src.data<0>()->type_code.pointer_power);
				}
			else{	// can't test this from preprocessor
				src.type_code.set_type(0);	// incoherent type
				simple_error(src," has incoherent type");
				}
			break;
			}
	}

	// 2) prefix arg type convertible to bool (control whether expression is evaluatable at all)
	if (!converts_to_bool(src.data<1>()->type_code ARG_TYPES))
		{	// can't test this from preprocessor
		simple_error(src," has control expression unconvertible to bool");
		return;
		}
	// 3) RAM conservation: if we have a suitable literal Do It Now
	// \todo disable this at O0?
	if (eval_conditional_op(src,CPP_literal_converts_to_bool ARG_TYPES)) return;
}

//! \throws std::bad_alloc
static void locate_C99_conditional_op(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_conditional_op(src,i))
		C_conditional_op_easy_syntax_check(src.c_array<0>()[i],types);
}

//! \throws std::bad_alloc
static void locate_CPP_conditional_op(parse_tree& src, size_t& i, const type_system& types)
{
	assert(!src.empty<0>());
	assert(i<src.size<0>());
	if (   (PARSE_OBVIOUS & src.data<0>()[i].flags)
		|| !src.data<0>()[i].is_atomic())
		return;

	if (terse_locate_conditional_op(src,i))
		CPP_conditional_op_easy_syntax_check(src.c_array<0>()[i],types);
}

template<class T>
static void parse_forward(parse_tree& src,const type_system& types, T parse_handler)
{
	assert(!src.empty<0>());
	if (0<src.size<0>())
		{
		size_t i = 0;
		do	{
			if (parse_tree::INVALID & src.data<0>()[i].flags) continue;
			parse_handler(src,i,types);
			}
		while(src.size<0>()>++i);
		};
}

template<class T>
static void parse_backward(parse_tree& src,const type_system& types, T parse_handler)
{
	assert(!src.empty<0>());
	if (0<src.size<0>())
		{
		size_t i = src.size<0>();
		do	{
			if (parse_tree::INVALID & src.data<0>()[--i].flags) continue;
			parse_handler(src,i,types);
			}
		while(0<i);
		};
}

// top-level has SIZE_MAX for parent_identifier_count
//! \throws std::bad_alloc
static void C99_locate_expressions(parse_tree& src,const size_t parent_identifier_count,const type_system& types)
{
	if (PARSE_OBVIOUS & src.flags) return;
	size_t identifier_count = (0==parent_identifier_count) ? 0 : _count_identifiers(src);
	{
	size_t i[3] = {src.size<0>(), src.size<1>(), src.size<2>()};
	size_t initial_i[3];
full_restart:
	memmove(initial_i,i,3*sizeof(size_t));
	size_t stalled[3] = {SIZE_MAX,SIZE_MAX,SIZE_MAX};
	try {
		while(0<i[0]) C99_locate_expressions(src.c_array<0>()[--i[0]],identifier_count,types);
		}
	catch(std::bad_alloc&)
		{
		stalled[0] = i[0]++;
		goto restart_1;
		}
restart_1:
	try {
		while(0<i[1]) C99_locate_expressions(src.c_array<1>()[--i[1]],identifier_count,types);
		}
	catch(std::bad_alloc&)
		{
		stalled[1] = i[1]++;
		goto restart_2;
		}
restart_2:
	try {
		while(0<i[2]) C99_locate_expressions(src.c_array<2>()[--i[2]],identifier_count,types);
		}
	catch(std::bad_alloc&)
		{
		stalled[2] = i[2]++;
		goto restart_3;
		}
restart_3:
	if (SIZE_MAX>stalled[0] || SIZE_MAX>stalled[1] || SIZE_MAX>stalled[2])
		{	// had a memory management problem
		if (i[0]<initial_i[0] || i[1]<initial_i[1] || i[2]<initial_i[2])
			// if we made some progress, restart
			goto full_restart;
		// otherwise give up
		throw std::bad_alloc();
		}
	}

	const bool top_level = SIZE_MAX==parent_identifier_count;
	const bool parens_are_expressions = 0==parent_identifier_count	// no identifiers from outside
									|| (top_level && 0==identifier_count);	// top-level, no identifiers

	if (parens_are_expressions || top_level || parent_identifier_count==identifier_count)
		if (inspect_potential_paren_primary_expression(src))
			{
			if (top_level && 1==src.size<0>() && is_naked_parentheses_pair(src))
				src.eval_to_arg<0>(0);
			return;
			}

	// top-level [ ] and { } die regardless of contents
	// note that top-level [ ] should be asphyxiating now
	if (top_level && suppress_naked_brackets_and_braces(src,"top-level",sizeof("top-level")-1))
		return;

	if (!src.empty<0>())
		{
		suppress_naked_brackets_and_braces(*src.c_array<0>(),"top-level",sizeof("top-level")-1);
		parse_forward(src,types,locate_C99_postfix_expression);
		parse_backward(src,types,locate_C99_unary_expression);
		parse_forward(src,types,locate_C99_mult_expression);
		parse_forward(src,types,locate_C99_add_expression);
		parse_forward(src,types,locate_C99_shift_expression);
		parse_forward(src,types,locate_C99_relation_expression);
		parse_forward(src,types,locate_C99_equality_expression);
		parse_forward(src,types,locate_C99_bitwise_AND);
		parse_forward(src,types,locate_C99_bitwise_XOR);
		parse_forward(src,types,locate_C99_bitwise_OR);
		parse_forward(src,types,locate_C99_logical_AND);
		parse_forward(src,types,locate_C99_logical_OR);
		parse_backward(src,types,locate_C99_conditional_op);
/*
assignment-expression:
	conditional-expression
	unary-expression assignment-operator assignment-expression

assignment-operator: one of
	= *= /= %= += -= <<= >>= &= ^= |=
*/
#if 0
		parse_backward(src,types,...);
#endif
/*
expression:
	assignment-expression
	expression , assignment-expression
*/
#if 0
		parse_forward(src,types,...);
#endif
		};
}

// top-level has SIZE_MAX for parent_identifier_count
//! \throws std::bad_alloc
static void CPP_locate_expressions(parse_tree& src,const size_t parent_identifier_count,const type_system& types)
{
	if (PARSE_OBVIOUS & src.flags) return;
	const size_t identifier_count = (0==parent_identifier_count) ? 0 : _count_identifiers(src);
	{
	size_t i[3] = {src.size<0>(), src.size<1>(), src.size<2>()};
	size_t initial_i[3];
full_restart:
	memmove(initial_i,i,3*sizeof(size_t));
	size_t stalled[3] = {SIZE_MAX,SIZE_MAX,SIZE_MAX};
	try {
		while(0<i[0]) CPP_locate_expressions(src.c_array<0>()[--i[0]],identifier_count,types);
		}
	catch(std::bad_alloc&)
		{
		stalled[0] = i[0]++;
		goto restart_1;
		}
restart_1:
	try {
		while(0<i[1]) CPP_locate_expressions(src.c_array<1>()[--i[1]],identifier_count,types);
		}
	catch(std::bad_alloc&)
		{
		stalled[1] = i[1]++;
		goto restart_2;
		}
restart_2:
	try {
		while(0<i[2]) CPP_locate_expressions(src.c_array<2>()[--i[2]],identifier_count,types);
		}
	catch(std::bad_alloc&)
		{
		stalled[2] = i[2]++;
		goto restart_3;
		}
restart_3:
	if (SIZE_MAX>stalled[0] || SIZE_MAX>stalled[1] || SIZE_MAX>stalled[2])
		{	// had a memory management problem
		if (i[0]<initial_i[0] || i[1]<initial_i[1] || i[2]<initial_i[2])
			// if we made some progress, restart
			goto full_restart;
		// otherwise give up
		throw std::bad_alloc();
		}
	}
	
	const bool top_level = SIZE_MAX==parent_identifier_count;
	const bool parens_are_expressions = 0==parent_identifier_count	// no identifiers from outside
									|| (top_level && 0==identifier_count);	// top-level, no identifiers

	// try for ( expression )
	if (parens_are_expressions || top_level || parent_identifier_count==identifier_count)
		if (inspect_potential_paren_primary_expression(src))
			{
			if (top_level && 1==src.size<0>() && is_naked_parentheses_pair(src))
				src.eval_to_arg<0>(0);
			return;
			}

	// top-level [ ] and { } die regardless of contents
	if (top_level && suppress_naked_brackets_and_braces(src,"top-level",sizeof("top-level")-1))
		return;

	if (!src.empty<0>())
		{
		suppress_naked_brackets_and_braces(*src.c_array<0>(),"top-level",sizeof("top-level")-1);
		parse_forward(src,types,locate_CPP_postfix_expression);
		parse_backward(src,types,locate_CPP_unary_expression);
#if 0
/*
pmexpression:
	castexpression
	pmexpression .* castexpression
	pmexpression ->* castexpression
*/
		parse_forward(src,types,...);
#endif
		parse_forward(src,types,locate_CPP_mult_expression);
		parse_forward(src,types,locate_CPP_add_expression);
		parse_forward(src,types,locate_CPP_shift_expression);
		parse_forward(src,types,locate_CPP_relation_expression);
		parse_forward(src,types,locate_CPP_equality_expression);
		parse_forward(src,types,locate_CPP_bitwise_AND);
		parse_forward(src,types,locate_CPP_bitwise_XOR);
		parse_forward(src,types,locate_CPP_bitwise_OR);
		parse_forward(src,types,locate_CPP_logical_AND);
		parse_forward(src,types,locate_CPP_logical_OR);
		parse_backward(src,types,locate_CPP_conditional_op);
/*
assignment-expression:
	conditional-expression
	unary-expression assignment-operator assignment-expression

assignment-operator: one of
	= *= /= %= += -= <<= >>= &= ^= |=
*/
#if 0
		parse_backward(src,types,...);
#endif
/*
expression:
	assignment-expression
	expression , assignment-expression
*/
#if 0
		parse_forward(src,types,...);
#endif
		};
}

static void _label_CPP_literal(parse_tree& src)
{
	if (src.is_atomic() && C_TESTFLAG_IDENTIFIER==src.index_tokens[0].flags) 
		{
		if 		(token_is_string<4>(src.index_tokens[0].token,"true"))
			{
			src.flags |= (PARSE_PRIMARY_EXPRESSION | parse_tree::CONSTANT_EXPRESSION);
			src.type_code.set_type(C_TYPE::BOOL);
			}
		else if (token_is_string<4>(src.index_tokens[0].token,"this"))
			{
			src.flags |= PARSE_PRIMARY_EXPRESSION;
			src.type_code.set_type(C_TYPE::NOT_VOID);
			src.type_code.set_pointer_power(1);
			}
		else if (token_is_string<5>(src.index_tokens[0].token,"false"))
			{
			src.flags |= (PARSE_PRIMARY_EXPRESSION | parse_tree::CONSTANT_EXPRESSION);
			src.type_code.set_type(C_TYPE::BOOL);
			}
		}
}

//! \throw std::bad_alloc
static bool C99_CondenseParseTree(parse_tree& src,const type_system& types)
{
	assert(src.is_raw_list());
	assert(1<src.size<0>());
	const size_t starting_errors = zcc_errors.err_count();
	_label_literals(src,types);
	if (!_match_pairs(src)) return false;
	try {
		C99_locate_expressions(src,SIZE_MAX,types);
		}
	catch(std::bad_alloc&)
		{	// error count change is already false
		if (starting_errors<zcc_errors.err_count()) return false;
		throw;
		}
	if (starting_errors<zcc_errors.err_count()) return false;
	while(src.is_raw_list() && 1==src.size<0>()) src.eval_to_arg<0>(0);
	return true;
}

//! \throw std::bad_alloc
static bool CPP_CondenseParseTree(parse_tree& src,const type_system& types)
{
	assert(src.is_raw_list());
	assert(1<src.size<0>());
	const size_t starting_errors = zcc_errors.err_count();
	_label_literals(src,types);
	std::for_each(src.begin<0>(),src.end<0>(),_label_CPP_literal);	// intercepts: true, false, this
	if (!_match_pairs(src)) return false;
	try {
		CPP_locate_expressions(src,SIZE_MAX,types);
		}
	catch(std::bad_alloc&)
		{	// error count change is already false
		if (starting_errors<zcc_errors.err_count()) return false;
		throw;
		}
	if (starting_errors<zcc_errors.err_count()) return false;
	while(src.is_raw_list() && 1==src.size<0>()) src.eval_to_arg<0>(0);
	return true;
}


//! \test if.C99/Pass_zero.hpp, if.C99/Pass_zero.h
bool C99_integer_literal_is_zero(const char* const x,const size_t x_len,const lex_flags flags)
{
	assert(x && *x);
	assert(0<x_len);
	assert(C_TESTFLAG_PP_NUMERAL & flags);
	assert(!(C_TESTFLAG_FLOAT & flags));
	C_REALITY_CHECK_PP_NUMERAL_FLAGS(flags);
	//! \todo need some way to signal legality for integer literals
	switch(C_EXTRACT_BASE_CODE(flags))
	{
#ifndef NDEBUG
	default: FATAL_CODE("unclassified integer literal",3);
#endif
	case C_BASE_OCTAL:
		{	// all-zeros is zero, ok with leading 0 prefix
		C_PPOctalInteger test_oct;
		ZAIMONI_PASSTHROUGH_ASSERT(C_PPOctalInteger::is(x,x_len,test_oct));
		return strspn(test_oct.ptr,"0") == test_oct.digit_span;
		};
	case C_BASE_DECIMAL:
		{	// decimal is easy
		C_PPDecimalInteger test_dec;
		ZAIMONI_PASSTHROUGH_ASSERT(C_PPDecimalInteger::is(x,x_len,test_dec));
		return 1==test_dec.digit_span && '0'==test_dec.ptr[0];
		};
	case C_BASE_HEXADECIMAL:
		{	// all-zeros is zero, but ignore the leading 0x prefix
		C_PPHexInteger test_hex;
		ZAIMONI_PASSTHROUGH_ASSERT(C_PPHexInteger::is(x,x_len,test_hex));
		return strspn(test_hex.ptr+2,"0")+2 == test_hex.digit_span;
		};
	}
#ifdef NDEBUG
	return false;
#endif
}

static void eval_string_literal_deref(parse_tree& src,const type_system& types,const POD_pair<const char*,size_t>& str_lit,const umaxint& tmp,bool is_negative,bool index_src_is_char)
{
	const size_t strict_ub = LengthOfCStringLiteral(str_lit.first,str_lit.second);
	// C99 6.2.6.2p3 -0 is not actually allowed to generate the bitpattern -0, so no trapping
	if (is_negative && tmp==0) is_negative = false;
	if (is_negative)
		{	//! \test default/Error_if_control66.hpp, default/Error_if_control66.h
			//! \test default/Error_if_control67.hpp, default/Error_if_control67.h
		if (!(src.flags & parse_tree::INVALID))
			{
			message_header(src.index_tokens[0]);
			INC_INFORM(ERR_STR);
			INC_INFORM("undefined behavior: ");
			INC_INFORM(src);
			INFORM(" dereferences string literal with negative index");
			if (index_src_is_char)
				INFORM("(does this source code want char to act like unsigned char?)");
			src.flags |= parse_tree::INVALID;
			zcc_errors.inc_error();
			}
		return;
		}
	else if (strict_ub <= tmp)
		{	//! \test default/Error_if_control68.hpp, default/Error_if_control68.h
			//! \test default/Error_if_control69.hpp, default/Error_if_control69.h
		if (!(src.flags & parse_tree::INVALID))
			{
			message_header(src.index_tokens[0]);
			INC_INFORM(ERR_STR);
			INC_INFORM("undefined behavior: ");
			INC_INFORM(src);
			INFORM(" dereferences string literal past its end");
			if (index_src_is_char && target_machine->signed_max<virtual_machine::std_int_char>()<tmp)
				{
				if (tmp.to_uint()-1==target_machine->signed_max<virtual_machine::std_int_char>())
					INFORM("(does this source code want char to act like signed char, with integer representation sign-and-magnitude?)");
				else if (tmp==target_machine->unsigned_max<virtual_machine::std_int_char>())
					INFORM("(does this source code want char to act like signed char, with integer representation one's complement?)");
				}
			src.flags |= parse_tree::INVALID;
			zcc_errors.inc_error();
			}
		return;
		};
	assert(tmp.representable_as_uint());
	char* tmp2 = GetCCharacterLiteralAt(str_lit.first,str_lit.second,tmp.to_uint());
	assert(tmp2);
	src.destroy();	// str_lit goes invalid here, don't use again
	src.grab_index_token_from<0>(tmp2,C_TESTFLAG_CHAR_LITERAL);
	_label_one_literal(src,types);
}

#define ZCC_EVALPARSETREE_PAIR_EVAL(A,B)	\
	{	\
	bool RAM_err = false;	\
	try {	\
		EvalParseTree(*src.c_array<A>(),types);	\
		}	\
	catch(std::bad_alloc&)	\
		{	\
		RAM_err = true;	\
		goto restart_1;	\
		}	\
restart_1:	\
	/* can't recover locally if this throws std::bad_alloc */	\
	EvalParseTree(*src.c_array<B>(),types);	\
	if (RAM_err) EvalParseTree(*src.c_array<A>(),types);	\
	}


//! \throw std::bad_alloc
static bool
eval_array_deref(parse_tree& src,const type_system& types,
				 func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
				 func_traits<bool (*)(const parse_tree&)>::function_ref_type literal_converts_to_integer,
				 intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	if (!is_array_deref(src)) return false;
	// crunch __[...]
	// canonical definition: *((__)+(...))
	ZCC_EVALPARSETREE_PAIR_EVAL(0,1);
	if (parse_tree::CONSTANT_EXPRESSION & src.flags)
		{
		const unsigned int str_index = 	(C_TESTFLAG_STRING_LITERAL==src.data<0>()->index_tokens[0].flags) ? 0 :
										(C_TESTFLAG_STRING_LITERAL==src.data<1>()->index_tokens[0].flags) ? 1 : UINT_MAX;
		if (UINT_MAX>str_index)
			{
			umaxint tmp; 
			if (!intlike_literal_to_VM(tmp,*src.data(1-str_index) ARG_TYPES)) return false;
			const size_t promoted_type = default_promote_type(src.type_code.base_type_index ARG_TYPES);
			const virtual_machine::std_int_enum machine_type = (virtual_machine::std_int_enum)((promoted_type-C_TYPE::INT)/2+virtual_machine::std_int_int);
			eval_string_literal_deref(src,types,src.data(str_index)->index_tokens[0].token,tmp,tmp.test(target_machine->C_bit(machine_type)-1),C_TESTFLAG_CHAR_LITERAL==src.data(1-str_index)->index_tokens[0].flags);
			return true;
			}
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_deref(	parse_tree& src, const type_system& types,
						func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree)
{
	//! \todo handle operator overloading (fork to handle C/C++?)
	//! \todo catch *& cancellation
	if (is_C99_unary_operator_expression<'*'>(src))
		{
		EvalParseTree(*src.c_array<2>(),types);
		if (C_TESTFLAG_STRING_LITERAL==src.data<2>()->index_tokens[0].flags)
			{
			//! \test default/Pass_if_zero.hpp
			//! \test default/Pass_if_zero.h
			//! \test default/Pass_if_nonzero.hpp
			//! \test default/Pass_if_nonzero.h
			eval_string_literal_deref(src,types,src.data<2>()->index_tokens[0].token,umaxint(0),false,false);
			return true;
			}
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_logical_NOT(parse_tree& src, const type_system& types,
							 func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
							 func_traits<bool (*)(const parse_tree&)>::function_ref_type is_logical_NOT_expression,
							 literal_converts_to_bool_func& literal_converts_to_bool)
{
	if (is_logical_NOT_expression(src))
		{
		EvalParseTree(*src.c_array<2>(),types);
		if (eval_logical_NOT(src,types,is_logical_NOT_expression,literal_converts_to_bool)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_bitwise_compl(	parse_tree& src, const type_system& types,
								func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
								func_traits<bool (*)(const parse_tree&)>::function_ref_type is_bitwise_complement_expression,
								intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	if (is_bitwise_complement_expression(src))
		{
		EvalParseTree(*src.c_array<2>(),types);
		if (eval_bitwise_compl(src,types,is_bitwise_complement_expression,intlike_literal_to_VM)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_unary_plus(parse_tree& src, const type_system& types,
							func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree)
{
	if (is_C99_unary_operator_expression<'+'>(src))
		{
		EvalParseTree(*src.c_array<2>(),types);
		if (eval_unary_plus(src,types)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_unary_minus(parse_tree& src, const type_system& types,
							 func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
							 literal_converts_to_bool_func& literal_converts_to_bool,
							 intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	if (is_C99_unary_operator_expression<'-'>(src))
		{
		EvalParseTree(*src.c_array<2>(),types);
		if (eval_unary_minus(src,types,literal_converts_to_bool,intlike_literal_to_VM)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_mult_expression(parse_tree& src,const type_system& types,
								func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
								literal_converts_to_bool_func& literal_converts_to_bool,
								intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	if (is_C99_mult_operator_expression<'*'>(src))
		{
		ZCC_EVALPARSETREE_PAIR_EVAL(1,2);
		if (eval_mult_expression(src,types,literal_converts_to_bool,intlike_literal_to_VM)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_div_expression(parse_tree& src,const type_system& types,
								func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
								literal_converts_to_bool_func& literal_converts_to_bool,
								intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	if (is_C99_mult_operator_expression<'/'>(src))
		{
		ZCC_EVALPARSETREE_PAIR_EVAL(1,2);
		if (eval_div_expression(src,types,literal_converts_to_bool,intlike_literal_to_VM)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_mod_expression(parse_tree& src,const type_system& types,
								func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
								literal_converts_to_bool_func& literal_converts_to_bool,
								intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	if (is_C99_mult_operator_expression<'%'>(src))
		{
		ZCC_EVALPARSETREE_PAIR_EVAL(1,2);
		if (eval_mod_expression(src,types,literal_converts_to_bool,intlike_literal_to_VM)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_add_expression(parse_tree& src,const type_system& types,
							func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
							literal_converts_to_bool_func& literal_converts_to_bool,
							intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	if (is_C99_add_operator_expression<'+'>(src))
		{
		ZCC_EVALPARSETREE_PAIR_EVAL(1,2);
		if (eval_add_expression(src,types,literal_converts_to_bool,intlike_literal_to_VM)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_sub_expression(parse_tree& src,const type_system& types,
							func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
							literal_converts_to_bool_func& literal_converts_to_bool,
							intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	if (is_C99_add_operator_expression<'-'>(src))
		{
		ZCC_EVALPARSETREE_PAIR_EVAL(1,2);
		if (eval_sub_expression(src,types,literal_converts_to_bool,intlike_literal_to_VM)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_shift(parse_tree& src,const type_system& types,
							func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
							literal_converts_to_bool_func& literal_converts_to_bool,
							intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	if (is_C99_shift_expression(src))
		{
		ZCC_EVALPARSETREE_PAIR_EVAL(1,2);
		if (eval_shift(src,types,literal_converts_to_bool,intlike_literal_to_VM)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_relation_expression(parse_tree& src,const type_system& types,
							func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
							intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	if (is_C99_relation_expression(src))
		{
		ZCC_EVALPARSETREE_PAIR_EVAL(1,2);
		if (eval_relation_expression(src,types,intlike_literal_to_VM)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_equality_expression(parse_tree& src,const type_system& types,
							func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
							func_traits<bool (*)(const parse_tree&)>::function_ref_type is_equality_expression,
							literal_converts_to_bool_func& literal_converts_to_bool,
							intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	if (is_equality_expression(src))
		{
		ZCC_EVALPARSETREE_PAIR_EVAL(1,2);
		if (eval_equality_expression(src,types,literal_converts_to_bool,intlike_literal_to_VM)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_bitwise_AND(parse_tree& src,const type_system& types,
							func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
							func_traits<bool (*)(const parse_tree&)>::function_ref_type is_bitwise_AND_expression,
							literal_converts_to_bool_func& literal_converts_to_bool,
							intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	if (is_bitwise_AND_expression(src))
		{
		ZCC_EVALPARSETREE_PAIR_EVAL(1,2);
		if (eval_bitwise_AND(src,types,literal_converts_to_bool,intlike_literal_to_VM)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_bitwise_XOR(parse_tree& src,const type_system& types,
							func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
							func_traits<bool (*)(const parse_tree&)>::function_ref_type is_bitwise_XOR_expression,
							literal_converts_to_bool_func& literal_converts_to_bool,
							intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	if (is_bitwise_XOR_expression(src))
		{
		ZCC_EVALPARSETREE_PAIR_EVAL(1,2);
		if (eval_bitwise_XOR(src,types,literal_converts_to_bool,intlike_literal_to_VM)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_bitwise_OR(parse_tree& src,const type_system& types,
							func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
							func_traits<bool (*)(const parse_tree&)>::function_ref_type is_bitwise_OR_expression,
							literal_converts_to_bool_func& literal_converts_to_bool,
							intlike_literal_to_VM_func& intlike_literal_to_VM)
{
	if (is_bitwise_OR_expression(src))
		{
		ZCC_EVALPARSETREE_PAIR_EVAL(1,2);
		if (eval_bitwise_OR(src,types,literal_converts_to_bool,intlike_literal_to_VM)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_logical_AND(parse_tree& src,const type_system& types,
							func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
							func_traits<bool (*)(const parse_tree&)>::function_ref_type is_logical_AND_expression,
							literal_converts_to_bool_func& literal_converts_to_bool)
{
	if (is_logical_AND_expression(src))
		{
		ZCC_EVALPARSETREE_PAIR_EVAL(1,2);
		if (eval_logical_AND(src,types,literal_converts_to_bool)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_logical_OR(parse_tree& src,const type_system& types,
							func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
							func_traits<bool (*)(const parse_tree&)>::function_ref_type is_logical_OR_expression,
							literal_converts_to_bool_func& literal_converts_to_bool)
{
	if (is_logical_OR_expression(src))
		{
		ZCC_EVALPARSETREE_PAIR_EVAL(1,2);
		if (eval_logical_OR(src,types,literal_converts_to_bool)) return true;
		}
	return false;
}

//! \throw std::bad_alloc
static bool eval_conditional_operator(parse_tree& src,const type_system& types,
									  func_traits<bool (*)(parse_tree&,const type_system&)>::function_ref_type EvalParseTree,
									  literal_converts_to_bool_func& literal_converts_to_bool)
{
	if (is_C99_conditional_operator_expression(src))
		{	// prefix operator is boolean
		EvalParseTree(*src.c_array<1>(),types);
		if (eval_conditional_op(src,literal_converts_to_bool ARG_TYPES)) return true;
		}
	return false;
}

#if 0
static bool cancel_addressof_deref_operators(parse_tree& src)
{
	assert(is_C99_unary_operator_expression(src));
	if ('&'==*src.index_tokens[0].token.first)
		{	// strip off &*, and remove lvalue-ness of target
		if (is_C99_unary_operator_expression<'*'>(*src.data<2>()) && 0<src.data<2>()->data<2>()->type_code.pointer_power)
			{
			parse_tree tmp;
			src.c_array<2>()->c_array<2>()->OverwriteInto(tmp);
			tmp.type_code.traits &= ~type_spec::lvalue;
			tmp.MoveInto(src);
			}
#if 0
		if (is_array_deref(*src.data<2>()))
			{	//! \todo convert &(___[...]) to (__+...)
			}
#endif
		};
	return false;
}
#endif

//! \throw std::bad_alloc
static bool C99_EvalParseTree(parse_tree& src,const type_system& types)
{
	const size_t starting_errors = zcc_errors.err_count();
RestartEval:
	if (src.is_atomic() || (parse_tree::INVALID & src.flags)) return starting_errors==zcc_errors.err_count();
	if (eval_array_deref(src,types,C99_EvalParseTree,C99_literal_converts_to_integer,C99_intlike_literal_to_VM)) goto RestartEval;
	if (eval_conditional_operator(src,types,C99_EvalParseTree,C99_literal_converts_to_bool)) goto RestartEval;
	if (eval_logical_OR(src,types,C99_EvalParseTree,is_C99_logical_OR_expression,C99_literal_converts_to_bool)) goto RestartEval;
	if (eval_logical_AND(src,types,C99_EvalParseTree,is_C99_logical_AND_expression,C99_literal_converts_to_bool)) goto RestartEval;
	if (eval_deref(src,types,C99_EvalParseTree)) goto RestartEval; 
	if (eval_logical_NOT(src,types,C99_EvalParseTree,is_C99_unary_operator_expression<'!'>,C99_literal_converts_to_bool)) goto RestartEval;
	if (eval_unary_plus(src,types,C99_EvalParseTree)) goto RestartEval;
	if (eval_unary_minus(src,types,C99_EvalParseTree,C99_literal_converts_to_bool,C99_intlike_literal_to_VM)) goto RestartEval;
	if (eval_mult_expression(src,types,C99_EvalParseTree,C99_literal_converts_to_bool,C99_intlike_literal_to_VM)) goto RestartEval;
	if (eval_div_expression(src,types,C99_EvalParseTree,C99_literal_converts_to_bool,C99_intlike_literal_to_VM)) goto RestartEval;
	if (eval_mod_expression(src,types,C99_EvalParseTree,C99_literal_converts_to_bool,C99_intlike_literal_to_VM)) goto RestartEval;
	if (eval_add_expression(src,types,C99_EvalParseTree,C99_literal_converts_to_bool,C99_intlike_literal_to_VM)) goto RestartEval;
	if (eval_sub_expression(src,types,C99_EvalParseTree,C99_literal_converts_to_bool,C99_intlike_literal_to_VM)) goto RestartEval;
	if (eval_shift(src,types,C99_EvalParseTree,C99_literal_converts_to_bool,C99_intlike_literal_to_VM)) goto RestartEval;
	if (eval_relation_expression(src,types,C99_EvalParseTree,C99_intlike_literal_to_VM)) goto RestartEval;
	if (eval_equality_expression(src,types,C99_EvalParseTree,is_C99_equality_expression,C99_literal_converts_to_bool,C99_intlike_literal_to_VM)) goto RestartEval;
	if (eval_bitwise_AND(src,types,C99_EvalParseTree,is_C99_bitwise_AND_expression,C99_literal_converts_to_bool,C99_intlike_literal_to_VM)) goto RestartEval;
	if (eval_bitwise_XOR(src,types,C99_EvalParseTree,is_C99_bitwise_XOR_expression,C99_literal_converts_to_bool,C99_intlike_literal_to_VM)) goto RestartEval;
	if (eval_bitwise_OR(src,types,C99_EvalParseTree,is_C99_bitwise_OR_expression,C99_literal_converts_to_bool,C99_intlike_literal_to_VM)) goto RestartEval;
	if (eval_bitwise_compl(src,types,C99_EvalParseTree,is_C99_unary_operator_expression<'~'>,C99_intlike_literal_to_VM)) goto RestartEval;
	return starting_errors==zcc_errors.err_count();
}

//! \throw std::bad_alloc
static bool CPP_EvalParseTree(parse_tree& src,const type_system& types)
{
	const size_t starting_errors = zcc_errors.err_count();
RestartEval:
	if (src.is_atomic() || (parse_tree::INVALID & src.flags)) return starting_errors==zcc_errors.err_count();
	if (eval_array_deref(src,types,CPP_EvalParseTree,CPP_literal_converts_to_integer,CPP_intlike_literal_to_VM)) goto RestartEval;
	if (eval_conditional_operator(src,types,CPP_EvalParseTree,CPP_literal_converts_to_bool)) goto RestartEval;
	if (eval_logical_OR(src,types,CPP_EvalParseTree,is_CPP_logical_OR_expression,CPP_literal_converts_to_bool)) goto RestartEval;
	if (eval_logical_AND(src,types,CPP_EvalParseTree,is_CPP_logical_AND_expression,CPP_literal_converts_to_bool)) goto RestartEval;
	if (eval_deref(src,types,CPP_EvalParseTree)) goto RestartEval; 
	if (eval_logical_NOT(src,types,CPP_EvalParseTree,is_CPP_logical_NOT_expression,CPP_literal_converts_to_bool)) goto RestartEval;
	if (eval_unary_plus(src,types,CPP_EvalParseTree)) goto RestartEval;
	if (eval_unary_minus(src,types,CPP_EvalParseTree,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM)) goto RestartEval;
	if (eval_mult_expression(src,types,CPP_EvalParseTree,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM)) goto RestartEval;
	if (eval_div_expression(src,types,CPP_EvalParseTree,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM)) goto RestartEval;
	if (eval_mod_expression(src,types,CPP_EvalParseTree,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM)) goto RestartEval;
	if (eval_add_expression(src,types,CPP_EvalParseTree,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM)) goto RestartEval;
	if (eval_sub_expression(src,types,CPP_EvalParseTree,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM)) goto RestartEval;
	if (eval_shift(src,types,CPP_EvalParseTree,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM)) goto RestartEval;
	if (eval_relation_expression(src,types,CPP_EvalParseTree,CPP_intlike_literal_to_VM)) goto RestartEval;
	if (eval_equality_expression(src,types,CPP_EvalParseTree,is_CPP_equality_expression,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM)) goto RestartEval;
	if (eval_bitwise_AND(src,types,CPP_EvalParseTree,is_CPP_bitwise_AND_expression,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM)) goto RestartEval;
	if (eval_bitwise_XOR(src,types,CPP_EvalParseTree,is_CPP_bitwise_XOR_expression,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM)) goto RestartEval;
	if (eval_bitwise_OR(src,types,CPP_EvalParseTree,is_CPP_bitwise_OR_expression,CPP_literal_converts_to_bool,CPP_intlike_literal_to_VM)) goto RestartEval;
	if (eval_bitwise_compl(src,types,CPP_EvalParseTree,is_CPP_bitwise_complement_expression,CPP_intlike_literal_to_VM)) goto RestartEval;
	return starting_errors==zcc_errors.err_count();
}

//! \throw std::bad_alloc
void C99_PPHackTree(parse_tree& src,const type_system& types)
{
	if (parse_tree::INVALID & src.flags) return;
	if (	is_C99_unary_operator_expression<'-'>(src)
		&&	(PARSE_PRIMARY_EXPRESSION & src.data<2>()->flags))
		{	// compact - literal to literal to get past preprocessor
		src.eval_to_arg<2>(0);
		return;
		};
	const type_spec old_type = src.type_code;
	const bool non_representable_int_min = virtual_machine::twos_complement==target_machine->C_signed_int_representation() && !bool_options[boolopt::int_traps];
	//! \todo handle other instances of non-representable int min constant expressions
	if (is_C99_add_operator_expression<'-'>(src))
		{
		bool is_equal = false;
		if (C_string_literal_equal_content(*src.data<1>(),*src.data<2>(),is_equal))
			{
			assert(!is_equal);	// should have intercepted equal-literal reduction earlier
#ifndef NDEBUG
			force_decimal_literal(src,"1",types);
#else
			force_decimal_literal(src,is_equal ? "0" : "1",types);
#endif
			src.type_code.set_type(C_TYPE::INT);
			return;
			};
		if (non_representable_int_min)
			{
			umaxint res_int;
			umaxint rhs_int;
			const bool lhs_converted = C99_intlike_literal_to_VM(res_int,*src.data<1>() ARG_TYPES);
			const bool rhs_converted = C99_intlike_literal_to_VM(rhs_int,*src.data<2>() ARG_TYPES);
			if (lhs_converted && rhs_converted)
				{	//! \todo deal with signed integer arithmetic
				const promote_aux old(old_type.base_type_index ARG_TYPES);
				assert(old.is_signed);
				const promote_aux lhs(src.data<1>()->type_code.base_type_index ARG_TYPES);
				assert(old.bitcount>=lhs.bitcount);
				const promote_aux rhs(src.data<2>()->type_code.base_type_index ARG_TYPES);
				assert(old.bitcount>=rhs.bitcount);

				// handle sign-extension of lhs, rhs
#ifndef NDEBUG
				const bool lhs_negative = target_machine->C_promote_integer(res_int,lhs,old);
				const bool rhs_negative = target_machine->C_promote_integer(rhs_int,rhs,old);
#else
				target_machine->C_promote_integer(res_int,lhs,old);
				target_machine->C_promote_integer(rhs_int,rhs,old);
#endif
				assert(lhs_negative && !rhs_negative);
				umaxint lhs_test(res_int);
				umaxint rhs_test(rhs_int);
				umaxint ub(target_machine->signed_max(old.machine_type));
				target_machine->signed_additive_inverse(lhs_test,old.machine_type);
				ub += 1;
				assert(ub>=lhs_test && ub>=rhs_test);
				ub -= lhs_test;
				assert(ub>=rhs_test);
				lhs_test += rhs_test;
				assert(target_machine->signed_max(old.machine_type)<lhs_test);
				// ok...valid but won't reduce.  pick an argument and mock this up
				src.eval_to_arg<2>(0);
				return;
				}
			}
		}
}

//! \throw std::bad_alloc
void CPP_PPHackTree(parse_tree& src,const type_system& types)
{
	if (parse_tree::INVALID & src.flags) return;
	if (	is_C99_unary_operator_expression<'-'>(src)
		&&	(PARSE_PRIMARY_EXPRESSION & src.data<2>()->flags))
		{	// compact - literal to literal to get past preprocessor
		src.eval_to_arg<2>(0);
		return;
		};
	const type_spec old_type = src.type_code;
	const bool non_representable_int_min = virtual_machine::twos_complement==target_machine->C_signed_int_representation() && !bool_options[boolopt::int_traps];
	//! \todo handle other instances of non-representable int min constant expressions
	if (is_C99_add_operator_expression<'-'>(src))
		{
		bool is_equal = false;
		if (C_string_literal_equal_content(*src.data<1>(),*src.data<2>(),is_equal))
			{
			assert(!is_equal);	// should have intercepted equal-literal reduction earlier
#ifndef NDEBUG
			force_decimal_literal(src,"1",types);
#else
			force_decimal_literal(src,is_equal ? "0" : "1",types);
#endif
			src.type_code.set_type(C_TYPE::INT);
			return;
			};
		if (non_representable_int_min)
			{
			umaxint res_int;
			umaxint rhs_int;
			const bool lhs_converted = CPP_intlike_literal_to_VM(res_int,*src.data<1>() ARG_TYPES);
			const bool rhs_converted = CPP_intlike_literal_to_VM(rhs_int,*src.data<2>() ARG_TYPES);
			if (lhs_converted && rhs_converted)
				{	//! \todo deal with signed integer arithmetic
				const promote_aux old(old_type.base_type_index ARG_TYPES);
				assert(old.is_signed);
				const promote_aux lhs(src.data<1>()->type_code.base_type_index ARG_TYPES);
				assert(old.bitcount>=lhs.bitcount);
				const promote_aux rhs(src.data<2>()->type_code.base_type_index ARG_TYPES);
				assert(old.bitcount>=rhs.bitcount);

				// handle sign-extension of lhs, rhs
#ifndef NDEBUG
				const bool lhs_negative = target_machine->C_promote_integer(res_int,lhs,old);
				const bool rhs_negative = target_machine->C_promote_integer(rhs_int,rhs,old);
#else
				target_machine->C_promote_integer(res_int,lhs,old);
				target_machine->C_promote_integer(rhs_int,rhs,old);
#endif
				assert(lhs_negative && !rhs_negative);
				umaxint lhs_test(res_int);
				umaxint rhs_test(rhs_int);
				umaxint ub(target_machine->signed_max(old.machine_type));
				target_machine->signed_additive_inverse(lhs_test,old.machine_type);
				ub += 1;
				assert(ub>=lhs_test && ub>=rhs_test);
				ub -= lhs_test;
				assert(ub>=rhs_test);
				lhs_test += rhs_test;
				assert(target_machine->signed_max(old.machine_type)<lhs_test);
				// ok...valid but won't reduce.  pick an argument and mock this up
				src.eval_to_arg<2>(0);
				return;
				}
			}
		}
}


//! \todo really should be somewhere in natural-language output
void INFORM_separated_list(const char* const* x,size_t x_len, const char* const sep)
{
	assert(sep && *sep);
	assert(x);
	if (0<x_len)
		{
		INC_INFORM(*x);
		while(0< --x_len)
			{
			INC_INFORM(sep);
			INC_INFORM(*(++x));
			}
		};
}

PP_auxfunc C99_aux
 = 	{
	LengthOfCSystemHeader,
	CPurePreprocessingOperatorPunctuationCode,
	CPurePreprocessingOperatorPunctuationFlags,
	C_like_BalancingCheck,
	C99_ControlExpressionContextFreeErrorCount,
	C99_CondenseParseTree,
	C99_EvalParseTree,
	C99_PPHackTree,
	ConcatenateCStringLiterals,
	C99_bad_syntax_tokenized,
	};

PP_auxfunc CPlusPlus_aux
 = 	{
	LengthOfCPPSystemHeader,
	CPPPurePreprocessingOperatorPunctuationCode,
	CPPPurePreprocessingOperatorPunctuationFlags,
	C_like_BalancingCheck,
	CPP_ControlExpressionContextFreeErrorCount,
	CPP_CondenseParseTree,
	CPP_EvalParseTree,
	CPP_PPHackTree,
	ConcatenateCStringLiterals,
	CPP_bad_syntax_tokenized,
	};

#if 0
// this is causing crashes post-exit
static void clear_lexer_defs(void)
{
	delete CLexer;
	delete CPlusPlusLexer;
}
#endif

void InitializeCLexerDefs(const virtual_machine::CPUInfo& target)
{
	// main code
	target_machine = &target;
	CLexer = new LangConf(	"//",			// C99, should be NULL for C90, C94
							"/*",
							"*/",
							&C_IsLegalSourceChar,
							&IsCIdentifierChar,
							NULL,
							NULL,
							LengthOfEscapedCString,
							EscapeCString,
							LengthOfUnescapedCString,
							UnescapeCString,
							"'\"",
							C_WHITESPACE,	// prepare LangConf change to test for all-but-first WS character
							C_ATOMIC_CHAR,
							valid_keyword+C_KEYWORD_NONSTRICT_LB,C_KEYWORD_STRICT_UB-C_KEYWORD_NONSTRICT_LB,
							&C99_aux,
							0,2,
							'\\','\\',true,true);
	CPlusPlusLexer = new LangConf(	"//",
									"/*",
									"*/",
									&C_IsLegalSourceChar,
									&IsCIdentifierChar,
									NULL,
									NULL,
									LengthOfEscapedCString,		// think we're fine for C++98
									EscapeCString,
									LengthOfUnescapedCString,
									UnescapeCString,
									"'\"",
									C_WHITESPACE,	// prepare LangConf change to test for all-but-first WS character
									C_ATOMIC_CHAR,
									valid_keyword+CPP_KEYWORD_NONSTRICT_LB,CPP_KEYWORD_STRICT_UB-CPP_KEYWORD_NONSTRICT_LB,
									&CPlusPlus_aux,
									0,2,
									'\\','\\',true,true);

	CLexer->InstallGlobalFilter(&TrimMandatoryTerminalNewline);
	CLexer->InstallGlobalFilter(&TerminalLineContinue);
	CLexer->InstallGlobalFilter(&EnforceCTrigraphs);
	CLexer->InstallGlobalFilter(&FlattenUNICODE);

	CLexer->InstallTokenizer(&LengthOfCCharLiteral,CPP_FLAG_CHAR_LITERAL);
	CLexer->InstallTokenizer(&LengthOfCStringLiteral,CPP_FLAG_STRING_LITERAL);
	CLexer->InstallTokenizer(&LengthOfCPurePreprocessingOperatorPunctuation,CPP_FLAG_PP_OP_PUNC);
	CLexer->InstallTokenizer(&LengthOfCIdentifier,CPP_FLAG_IDENTIFIER);
	CLexer->InstallTokenizer(&LengthOfCPreprocessingNumber,CPP_FLAG_PP_NUMERAL);

	CPlusPlusLexer->InstallGlobalFilter(&TrimMandatoryTerminalNewline);
	CPlusPlusLexer->InstallGlobalFilter(&TerminalLineContinue);
	CPlusPlusLexer->InstallGlobalFilter(&EnforceCTrigraphs);
	CPlusPlusLexer->InstallGlobalFilter(&FlattenUNICODE);

	CPlusPlusLexer->InstallTokenizer(&LengthOfCCharLiteral,CPP_FLAG_CHAR_LITERAL);
	CPlusPlusLexer->InstallTokenizer(&LengthOfCStringLiteral,CPP_FLAG_STRING_LITERAL);
	CPlusPlusLexer->InstallTokenizer(&LengthOfCPPPurePreprocessingOperatorPunctuation,CPP_FLAG_PP_OP_PUNC);
	CPlusPlusLexer->InstallTokenizer(&LengthOfCIdentifier,CPP_FLAG_IDENTIFIER);
	CPlusPlusLexer->InstallTokenizer(&LengthOfCPreprocessingNumber,CPP_FLAG_PP_NUMERAL);

#if 0
	if (atexit(clear_lexer_defs)) FATAL("atexit handler not installed");
#endif

	// integrity checks on the data definitions
	// do the constants match the function calls
	assert(C_TYPE::NOT_VOID==linear_find("$not-void",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::INTEGERLIKE==linear_find("$integer-like",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::VOID==linear_find("void",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::CHAR==linear_find("char",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::SCHAR==linear_find("signed char",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::UCHAR==linear_find("unsigned char",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::SHRT==linear_find("short",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::USHRT==linear_find("unsigned short",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::INT==linear_find("int",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::UINT==linear_find("unsigned",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::LONG==linear_find("long",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::ULONG==linear_find("unsigned long",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::LLONG==linear_find("long long",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::ULLONG==linear_find("unsigned long long",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::FLOAT==linear_find("float",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::DOUBLE==linear_find("double",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::LDOUBLE==linear_find("long double",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::BOOL==linear_find("_Bool",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::FLOAT__COMPLEX==linear_find("float _Complex",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::DOUBLE__COMPLEX==linear_find("double _Complex",C_atomic_types,C_TYPE_MAX)+1);
	assert(C_TYPE::LDOUBLE__COMPLEX==linear_find("long double _Complex",C_atomic_types,C_TYPE_MAX)+1);

	assert(C_TYPE::NOT_VOID==linear_find("$not-void",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::INTEGERLIKE==linear_find("$integer-like",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::VOID==linear_find("void",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::CHAR==linear_find("char",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::SCHAR==linear_find("signed char",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::UCHAR==linear_find("unsigned char",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::SHRT==linear_find("short",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::USHRT==linear_find("unsigned short",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::INT==linear_find("int",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::UINT==linear_find("unsigned",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::LONG==linear_find("long",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::ULONG==linear_find("unsigned long",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::LLONG==linear_find("long long",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::ULLONG==linear_find("unsigned long long",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::FLOAT==linear_find("float",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::DOUBLE==linear_find("double",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::LDOUBLE==linear_find("long double",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::BOOL==linear_find("bool",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::FLOAT__COMPLEX==linear_find("float _Complex",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::DOUBLE__COMPLEX==linear_find("double _Complex",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::LDOUBLE__COMPLEX==linear_find("long double _Complex",CPP_atomic_types,CPP_TYPE_MAX)+1);
	assert(C_TYPE::WCHAR_T==linear_find("wchar_t",CPP_atomic_types,CPP_TYPE_MAX)+1);

	/* does bool converts_to_integerlike(size_t base_type_index) work */
	BOOST_STATIC_ASSERT(!(C_TYPE::BOOL<=C_TYPE::NOT_VOID && C_TYPE::NOT_VOID<=C_TYPE::INTEGERLIKE));
	BOOST_STATIC_ASSERT(!(C_TYPE::BOOL<=C_TYPE::VOID && C_TYPE::VOID<=C_TYPE::INTEGERLIKE));
	BOOST_STATIC_ASSERT(C_TYPE::BOOL<=C_TYPE::BOOL && C_TYPE::BOOL<=C_TYPE::INTEGERLIKE);
	BOOST_STATIC_ASSERT(C_TYPE::BOOL<=C_TYPE::CHAR && C_TYPE::CHAR<=C_TYPE::INTEGERLIKE);
	BOOST_STATIC_ASSERT(C_TYPE::BOOL<=C_TYPE::SCHAR && C_TYPE::SCHAR<=C_TYPE::INTEGERLIKE);
	BOOST_STATIC_ASSERT(C_TYPE::BOOL<=C_TYPE::UCHAR && C_TYPE::UCHAR<=C_TYPE::INTEGERLIKE);
	BOOST_STATIC_ASSERT(C_TYPE::BOOL<=C_TYPE::SHRT && C_TYPE::SHRT<=C_TYPE::INTEGERLIKE);
	BOOST_STATIC_ASSERT(C_TYPE::BOOL<=C_TYPE::USHRT && C_TYPE::USHRT<=C_TYPE::INTEGERLIKE);
	BOOST_STATIC_ASSERT(C_TYPE::BOOL<=C_TYPE::INT && C_TYPE::INT<=C_TYPE::INTEGERLIKE);
	BOOST_STATIC_ASSERT(C_TYPE::BOOL<=C_TYPE::UINT && C_TYPE::UINT<=C_TYPE::INTEGERLIKE);
	BOOST_STATIC_ASSERT(C_TYPE::BOOL<=C_TYPE::LONG && C_TYPE::LONG<=C_TYPE::INTEGERLIKE);
	BOOST_STATIC_ASSERT(C_TYPE::BOOL<=C_TYPE::ULONG && C_TYPE::ULONG<=C_TYPE::INTEGERLIKE);
	BOOST_STATIC_ASSERT(C_TYPE::BOOL<=C_TYPE::LLONG && C_TYPE::LLONG<=C_TYPE::INTEGERLIKE);
	BOOST_STATIC_ASSERT(C_TYPE::BOOL<=C_TYPE::ULLONG && C_TYPE::ULLONG<=C_TYPE::INTEGERLIKE);
	BOOST_STATIC_ASSERT(C_TYPE::BOOL<=C_TYPE::INTEGERLIKE && C_TYPE::INTEGERLIKE<=C_TYPE::INTEGERLIKE);
	BOOST_STATIC_ASSERT(!(C_TYPE::BOOL<=C_TYPE::FLOAT && C_TYPE::FLOAT<=C_TYPE::INTEGERLIKE));
	BOOST_STATIC_ASSERT(!(C_TYPE::BOOL<=C_TYPE::DOUBLE && C_TYPE::DOUBLE<=C_TYPE::INTEGERLIKE));
	BOOST_STATIC_ASSERT(!(C_TYPE::BOOL<=C_TYPE::LDOUBLE && C_TYPE::LDOUBLE<=C_TYPE::INTEGERLIKE));
	BOOST_STATIC_ASSERT(!(C_TYPE::BOOL<=C_TYPE::FLOAT__COMPLEX && C_TYPE::FLOAT__COMPLEX<=C_TYPE::INTEGERLIKE));
	BOOST_STATIC_ASSERT(!(C_TYPE::BOOL<=C_TYPE::DOUBLE__COMPLEX && C_TYPE::DOUBLE__COMPLEX<=C_TYPE::INTEGERLIKE));
	BOOST_STATIC_ASSERT(!(C_TYPE::BOOL<=C_TYPE::LDOUBLE__COMPLEX && C_TYPE::LDOUBLE__COMPLEX<=C_TYPE::INTEGERLIKE));
}

