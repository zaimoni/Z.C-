// weak_token.hpp
// (C)2009,2011 Kenneth Boyd, license: MIT.txt

#ifndef WEAK_TOKEN_HPP
#define WEAK_TOKEN_HPP

#include "Zaimoni.STL/POD.hpp"
#include "Zaimoni.STL/LexParse/std.h"

struct weak_token
{
	zaimoni::POD_pair<const char*,size_t> token;	// start (not owned), length
	// pairs are: line number, origin (column number)
	zaimoni::POD_pair<size_t,size_t> logical_line;		// where the token actually is
	zaimoni::lex_flags flags;
	const char* src_filename;	// *NOT* owned; default NULL

	weak_token() = default;
	~weak_token() = default;
	weak_token(const weak_token& src) = default;
	weak_token(weak_token&& src) = delete;
	weak_token& operator=(const weak_token & src) = default;
	weak_token& operator=(weak_token&& src) = delete;
};

static_assert(std::is_pod_v<weak_token>);

// if we're already getting INC_INFORM from Zaimoni.STL/Logging.h then provide our own
#ifdef ZAIMONI_LOGGING_H
inline void INC_INFORM(const weak_token& src) {_inc_inform(src.token.first,src.token.second);}
#endif

#endif
