// Trigraph.hpp
// (C)2009,2010 Kenneth Boyd, license: MIT.txt

#ifndef Z_CPLUSPLUS_TRIGRAPH_HPP
#define Z_CPLUSPLUS_TRIGRAPH_HPP 1

/**
 *  Rewrites the string pointed to by Text in-place to eliminate C/C++ trigraphs, then shrinks it
 *
 * \pre string allocated with Zaimoni.STL memory manager
 *
 * \return true, return value is only to glue into LangConf
 */
bool EnforceCTrigraphs(char*& Text, const char* filename);

#endif
