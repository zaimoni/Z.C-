// errors.hpp
// (C)2009,2010 Kenneth Boyd, license: MIT.txt

#ifndef ERRORS_HPP
#define ERRORS_HPP 1

#include <stddef.h>
//#include "Zaimoni.STL/OS/mutex.hpp"
namespace zaimoni {
namespace OS {
	class mutex;
}

template<class T1, class T2, class T3> struct POD_triple;
}

typedef bool string_opt_handler(const char* const);

#define default_option2(A) DEFAULT_##A
#define default_option(A) default_option2(A)
#define default_handler2(A) HANDLER_##A
#define default_handler(A) default_handler2(A)

namespace boolopt {

enum boolean_options {
	test = 0,
#define boolean_option_0 test
#define boolean_option_test 0
#define DEFAULT_test false
	warnings_are_errors,
#define boolean_option_1 warnings_are_errors
#define boolean_option_warnings_are_errors 1
#define DEFAULT_warnings_are_errors false
	pedantic,
#define boolean_option_2 pedantic
#define boolean_option_pedantic 2
#define DEFAULT_pedantic false
	int_sign_magnitude,
#define boolean_option_3 int_sign_magnitude
#define boolean_option_int_sign_magnitude 3
#define DEFAULT_int_sign_magnitude false
	int_ones_complement,
#define boolean_option_4 int_ones_complement
#define boolean_option_int_ones_complement 4
#define DEFAULT_int_ones_complement false
	int_twos_complement,
#define boolean_option_5 int_twos_complement
#define boolean_option_int_twos_complement 5
#define DEFAULT_int_twos_complement false
	int_traps,
#define boolean_option_6 int_traps
#define boolean_option_int_traps 6
#define DEFAULT_int_traps false
	char_is_signed,
#define boolean_option_7 char_is_signed
#define boolean_option_char_is_signed 7
#define DEFAULT_char_is_signed false
	char_is_unsigned,
#define boolean_option_8 char_is_unsigned
#define boolean_option_char_is_unsigned 8
#define DEFAULT_char_is_unsigned false
	int_neg_div_rounds_away_from_zero,
#define boolean_option_9 int_neg_div_rounds_away_from_zero
#define boolean_option_int_neg_div_rounds_away_from_zero 9
#define DEFAULT_int_neg_div_rounds_away_from_zero false
	warn_crosslang_compatibility,
#define boolean_option_10 warn_crosslang_compatibility
#define boolean_option_warn_crosslang_compatibility 10
#define DEFAULT_warn_crosslang_compatibility false
	warn_backport,
#define boolean_option_11 warn_backport
#define boolean_option_warn_backport 11
#define DEFAULT_warn_backport false
	source_to_stdout,
#define boolean_option_12 source_to_stdout
#define boolean_option_source_to_stdout 12
#define DEFAULT_source_to_stdout false
	preprocess_only
#define boolean_option_13 preprocess_only
#define boolean_option_preprocess_only 13
#define DEFAULT_preprocess_only false
};

#define boolean_option(A) boolean_option_##A
#define MAX_OPT_BOOL (boolopt::preprocess_only+1)

}

namespace stringopt {
enum string_options {
	lang = 0,
#define string_option_0 lang
#define string_option_lang 0
#define DEFAULT_lang NULL
#define HANDLER_lang interpret_stringopt_lang
	system_include
#define string_option_1 system_include
#define string_option_system_include 1
#define DEFAULT_system_include NULL
#define HANDLER_system_include interpret_stringopt_system_include

};

#define string_option(A) string_option_##A
#define MAX_OPT_STRING (stringopt::system_include+1)

}

namespace intopt {
enum int_options {
	error_ub = 0,
#define int_option_0 error_ub
#define int_option_error_ub 0
#define DEFAULT_error_ub 100
#define HANDLER_error_ub interpret_intopt_error_ub
	target_char_bit,
#define int_option_1 target_char_bit
#define int_option_target_char_bit 1
#define DEFAULT_target_char_bit 0
#define HANDLER_target_char_bit interpret_intopt_target_char_bit
	target_short_size,
#define int_option_2 target_short_size
#define int_option_target_short_size 2
#define DEFAULT_target_short_size 0
#define HANDLER_target_short_size interpret_intopt_target_short_size
	target_int_size,
#define int_option_3 target_int_size
#define int_option_target_int_size 3
#define DEFAULT_target_int_size 0
#define HANDLER_target_int_size interpret_intopt_target_int_size
	target_long_size,
#define int_option_4 target_long_size
#define int_option_target_long_size 4
#define DEFAULT_target_long_size 0
#define HANDLER_target_long_size interpret_intopt_target_long_size
	target_long_long_size
#define int_option_5 target_long_long_size
#define int_option_target_long_long_size 5
#define DEFAULT_target_long_long_size 0
#define HANDLER_target_long_long_size interpret_intopt_target_long_long_size
};

#define int_option(A) int_option_##A
#define MAX_OPT_INT (intopt::target_long_long_size+1)

}

// declare globals of interest
extern const bool bool_options_default[MAX_OPT_BOOL];
extern bool bool_options[MAX_OPT_BOOL];
extern const char* string_options[MAX_OPT_STRING];
extern string_opt_handler* option_handler_string[MAX_OPT_STRING];
extern int int_options[MAX_OPT_INT];
extern string_opt_handler* option_handler_int[MAX_OPT_INT];

extern zaimoni::OS::mutex errno_mutex;

// following are for use by zaimoni::simple_lock
/*
 * some errors are only problems at runtime, and shouldn't be noticed if
 * the expression is unevaluated
 */ 
extern unsigned int no_runtime_errors;

#ifndef NDEBUG
extern bool debug_tracer;
#endif

int recognize_bool_option(const char* const x,const zaimoni::POD_triple<const char*, size_t, const char*>* option_map,size_t j);
int recognize_parameter_option(const char* const x,const zaimoni::POD_triple<const char*, size_t, const char*>* option_map,size_t j);

bool interpret_stringopt_lang(const char* x);

void message_header(const char* const filename, size_t line_number);
void enforce_mutually_exclusive_exhaustive_options(void);

#endif
