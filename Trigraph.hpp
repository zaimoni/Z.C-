// Trigraph.hpp
// (C)2009-2011 Kenneth Boyd, license: MIT.txt

#ifndef Z_CPLUSPLUS_TRIGRAPH_HPP
#define Z_CPLUSPLUS_TRIGRAPH_HPP 1

/**
 *  Rewrites the string pointed to by Text in-place to eliminate C/C++ trigraphs, then shrinks it
 *
 * \pre string allocated with Zaimoni.STL memory manager
 *
 * \return true, return value is only to glue into LangConf
 */
#ifndef ZAIMONI_FORCE_ISO
bool EnforceCTrigraphs(char*& x, const char* filename);
#else
#include <stddef.h>

bool EnforceCTrigraphs(char*& x, size_t& x_len, const char* filename);
#endif

#endif
