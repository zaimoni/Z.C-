// Trigraph.cpp
// (C)2009-2011 Kenneth Boyd, license: MIT.txt

#include "Trigraph.hpp"

#include "Zaimoni.STL/MetaRAM2.hpp"

using namespace zaimoni;

/*
2.3  Trigraph sequences
[lex.trigraph]

1 Before any other processing takes place, each occurrence of one of
the following sequences of three characters ("trigraph sequences") is
replaced by the single character indicated in Table 1.

                       Table 1--trigraph sequences
 
+---------------------+----------------------+----------------------+
|trigraph replacement | trigraph replacement | trigraph replacement |
+---------------------+----------------------+----------------------+
|  ??=         #      |   ??(         [      |   ??<         {      | 
+---------------------+----------------------+----------------------+
|  ??/         \      |   ??)         ]      |   ??>         }      | 
+---------------------+----------------------+----------------------+
|  ??'         ^      |   ??!         |      |   ??-         ~      | 
+---------------------+----------------------+----------------------+
|  ???         ?      |                      |                      | 
+---------------------+----------------------+----------------------+

2 [Example:
          ??=define arraycheck(a,b) a??(b??) ??!??! b??(a??)
  becomes
          #define arraycheck(a,b) a[b] || b[a]
   --end example]

3 [Note: no other trigraph sequence exists.  Each ? that does not
begin one of the trigraphs listed above is not changed.]

4 Trigraph replacement is done left to right, so that when two
sequences which could represent trigraphs overlap, only the first 
sequence is replaced.  Characters resulting from trigraph replacement
are never part of a subsequent trigraph.  [Example: The sequence
"???=" becomes "?=", not "?#".  The sequence "?????????" becomes
"???", not "?". --end example]
*/

/* Aside: ??? is not supported in C99 or C++0x; example in 4 may be a doc. error for C++97.  Given table row was in HTML version but not PDF version. */
/* C89 rationale suggest to omit ??? */

static size_t EnforceTrigraphsAux(size_t& Offset, char* const x)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// this returns how much to increment i by (not directly passed)
	if ('?'!=x[1]) return 2;	// total miss: increment by 2
	if ('?'!=x[0]) return 1;	// partial miss: increment by 1

	if ('='==x[2])
		{
		x[0]='#';
		Offset+=2;
		}
	else if ('/'==x[2])
		{
		x[0]='\\';
		Offset+=2;
		}
	else if ('\''==x[2])
		{
		x[0]='^';
		Offset+=2;
		}
	else if ('('==x[2])
		{
		x[0]='[';
		Offset+=2;
		}
	else if (')'==x[2])
		{
		x[0]=']';
		Offset+=2;
		}
	else if ('!'==x[2])
		{
		x[0]='|';
		Offset+=2;
		}
	else if ('<'==x[2])
		{
		x[0]='{';
		Offset+=2;
		}
	else if ('>'==x[2])
		{
		x[0]='}';
		Offset+=2;
		}
	else if ('-'==x[2])
		{
		x[0]='~';
		Offset+=2;
		};
	return 3;
}

static size_t
EnforceTrigraphsAuxV2(size_t& Offset, char* const x, char* const Target)
{	// FORMALLY CORRECT: Kenneth Boyd, 10/17/2004
	// this returns how much to increment i by (not directly passed)
	if ('?'!=x[1])
		{
		Target[0] = x[0];
		Target[1] = x[1];
		return 2;	// total miss: increment by 2
		};
	if ('?'!=x[0])
		{
		Target[0] = x[0];
		return 1;	// partial miss: increment by 1
		};

	if ('='==x[2])
		{
		Target[0]='#';
		Offset+=2;
		}
	else if ('/'==x[2])
		{
		Target[0]='\\';
		Offset+=2;
		}
	else if ('\''==x[2])
		{
		Target[0]='^';
		Offset+=2;
		}
	else if ('('==x[2])
		{
		Target[0]='[';
		Offset+=2;
		}
	else if (')'==x[2])
		{
		Target[0]=']';
		Offset+=2;
		}
	else if ('!'==x[2])
		{
		Target[0]='|';
		Offset+=2;
		}
	else if ('<'==x[2])
		{
		Target[0]='{';
		Offset+=2;
		}
	else if ('>'==x[2])
		{
		Target[0]='}';
		Offset+=2;
		}
	else if ('-'==x[2])
		{
		Target[0]='~';
		Offset+=2;
		}
	else{
		Target[0] = x[0];
		Target[1] = x[1];
		Target[2] = x[2];
		}
	return 3;
}

#ifndef ZAIMONI_FORCE_ISO
bool EnforceCTrigraphs(char*& x, const char* filename)
#else
bool EnforceCTrigraphs(char*& x, size_t& x_len, const char* filename)
#endif
{	// FORMALLY CORRECT: Kenneth Boyd, 8/1/2002
	// there are 10 C trigraphs of interest (?)
	// We use a modified Boyer-Moore algorithm, and compact after 
	// the left-to-right sweep.  This is a candidate for 
	// a state-machine implementation.
#ifndef ZAIMONI_FORCE_ISO
	const size_t TextLength = SafeArraySize(x);
#else
	const size_t TextLength = x_len-1;
#endif
	if (3<=TextLength)
		{
		size_t i = 0;
		size_t Offset = 0;
		do	i += EnforceTrigraphsAux(Offset,x+i);
		while(i+2<TextLength && 0==Offset);
		if (0!=Offset)
			{
			while(i+2<TextLength)
				i += EnforceTrigraphsAuxV2(Offset,x+i,x+(i-Offset));
			while(i<ZAIMONI_LEN_WITH_NULL(TextLength))
				{
				x[i-Offset] = x[i];
				++i;
				};
#ifndef ZAIMONI_FORCE_ISO
			_shrink(x,TextLength-Offset);
#else
			_shrink(x,x_len,x_len-Offset);
#endif
			};
		};
	return true;
}

