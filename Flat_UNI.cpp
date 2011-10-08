// Flat_UNI.cpp
// (C)2009-2011 Kenneth Boyd, license: MIT.txt

#include "Flat_UNI.hpp"

#include "lex_core.h"
#include "Zaimoni.STL/MetaRAM2.hpp"

using namespace zaimoni;

/*
2 The  universal-character-name  construct  provides a way to name
other characters.
          hex-quad:
                  hexadecimal-digit hexadecimal-digit
hexadecimal-digit hexadecimal-digit

          universal-character-name:
                  \u hex-quad
                  \U hex-quad hex-quad
  The character designated by the universal-character-name \UNNNNNNNN
is that character whose encoding in ISO/IEC 10646 is the hexadecimal
value NNNNNNNN; the character designated by the
universal-character-name  \uNNNN  is that character whose encoding in
ISO/IEC 10646 is the hexadecimal value 0000NNNN.

[also see Appendix E for a validation list...probably should update against latest UNICODE references]
*/

#ifndef ZAIMONI_FORCE_ISO
bool FlattenUNICODE(char*& x, const char* filename)
#else
bool FlattenUNICODE(char*& x, size_t& x_len, const char* filename)
#endif
{
	if (!x) return true;

#ifndef ZAIMONI_FORCE_ISO
	size_t target_len = ArraySize(x);
#else
	size_t target_len = x_len-1;
#endif
	bool want_realloc = false;
	// full encoding
	char* bloat_unicode = strstr(x,"\\U0000");
	if (bloat_unicode && 10>target_len-(bloat_unicode-x)) bloat_unicode = NULL;
	while(bloat_unicode)
		{
		if (4<=strspn(bloat_unicode+6,C_HEXADECIMAL_DIGITS))
			{
			const size_t block_length = bloat_unicode-x+2;
			bloat_unicode[1] = 'u';
			target_len -= 4;
			if (block_length<target_len)
				memmove(bloat_unicode+2,bloat_unicode+6,(target_len-block_length));
			memset(x+target_len,0,4);
			want_realloc = true;
			}
		bloat_unicode += 6;
		bloat_unicode = strstr(bloat_unicode,"\\U0000");
		if (bloat_unicode && 10>x_len-(bloat_unicode-x)) bloat_unicode = NULL;
		}
	
	bloat_unicode = strstr(x,"\\u00");
	if (bloat_unicode && 6>target_len-(bloat_unicode-x)) bloat_unicode = NULL;
	while(bloat_unicode)
		{	// we down-convert a subset of allowed characters, excluding anything lower than ASCII space to avoid real weirdness
			//! \todo portability target: want to adapt to preprocessor CHAR_BIT/UCHAR_MAX, this assumes 8/255; also assumes ASCII
			// we restrict our attention here to the utterly safe \u00A0 - \u00FF conversion
		if (strchr(C_HEXADECIMAL_DIGITS+10,bloat_unicode[4]) && strchr(C_HEXADECIMAL_DIGITS,bloat_unicode[5]))
			{
			const size_t block_length = bloat_unicode-x+1;
			const unsigned char tmp = (unsigned char)(16*InterpretHexadecimalDigit(bloat_unicode[4]+InterpretHexadecimalDigit(bloat_unicode[5])));
			assert(160<=tmp);
			bloat_unicode[0] = tmp;
			target_len -= 5;
			if (block_length<x_len)
				memmove(bloat_unicode+1,bloat_unicode+6,(x_len-block_length));
			memset(x+target_len,0,5);
			want_realloc = true;
			++bloat_unicode;
			}
		else
			bloat_unicode += 4;

		bloat_unicode = strstr(bloat_unicode,"\\u00");
		if (bloat_unicode && 6>x_len-(bloat_unicode-x)) bloat_unicode = NULL;
		}
#ifndef ZAIMONI_FORCE_ISO
	if (want_realloc) _shrink(x,target_len);
#else
	if (want_realloc)
		{
		_shrink(x,x_len,ZAIMONI_LEN_WITH_NULL(target_len));
		ZAIMONI_NULL_TERMINATE(x[target_len]);
		}
#endif
	return true;
}

