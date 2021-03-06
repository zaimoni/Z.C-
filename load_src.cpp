// load_src.cpp
// (C)2009,2010,2012,2020; Boost license @ LICENSE.md

#include "Zaimoni.STL/cstdio"
#include "Zaimoni.STL/LexParse/Token.hpp"
#include "Zaimoni.STL/LexParse/LangConf.hpp"
#include "AtomicString.h"
#include "errors.hpp"
#include "errcount.hpp"
#include "end_nl.hpp"

using namespace zaimoni;

// for this to work best, LangConf should include controls for
// * pure-whitespace lines irrelevant (may be able to get away without this)
// * whether or not line-continues are allowed
// * whether or not C trigraphs are used (think GlobalFilters works)
// * whether or not C universal-encoding is used (think GlobalFilters works)

static void
clean_whitespace(autovalarray_ptr<Token<char>* >& TokenList, size_t v_idx, const LangConf& lang)
{
	assert(TokenList.size()>v_idx);
	if (strlen(TokenList[v_idx]->data())==strspn(TokenList[v_idx]->data(),lang.WhiteSpace+1))
		TokenList.DeleteIdx(v_idx);	// pure whitespace
}

static void
clean_linesplice_whitespace(autovalarray_ptr<Token<char>* >& TokenList, size_t v_idx, const LangConf& lang)
{
	assert(TokenList.size()>v_idx);
	bool want_to_zap_line = true;
	if ('\\'==TokenList[v_idx]->data()[TokenList[v_idx]->size()-1])
		{	// line continue
		if (TokenList[v_idx]->logical_line.first+1==TokenList[v_idx+1]->logical_line.first)
			{
			TokenList[v_idx]->append(1,*TokenList[v_idx+1]);
			TokenList.DeleteIdx(v_idx+1);
			want_to_zap_line = false;
			}
		else
			TokenList[v_idx]->rtrim(1);
		}
	if (want_to_zap_line) clean_whitespace(TokenList,v_idx,lang);
}

//! \throw std::bad_alloc
bool
load_sourcefile(autovalarray_ptr<Token<char>* >& TokenList, const char* const filename, LangConf& lang)
{
	{
	autovalarray_ptr<Token<char>* > tmpTokenList(1);
	char* Buffer = NULL;
#ifndef ZAIMONI_FORCE_ISO
#	define Buffer_size ArraySize(Buffer)
#else
	size_t Buffer_size = 0;
#endif

	assert(filename && *filename);

	// XXX should return true for empty files XXX
	if (!GetBinaryFileImage(filename, Buffer ZAIMONI_ISO_PARAM(Buffer_size))) return false;
	ConvertBinaryModeToTextMode(Buffer ZAIMONI_ISO_PARAM(Buffer_size));
	if (!lang.ApplyGlobalFilters(Buffer ZAIMONI_ISO_PARAM(Buffer_size), filename)) exit(EXIT_FAILURE);
	lang.FlattenComments(Buffer ZAIMONI_ISO_PARAM(Buffer_size));

	try {
#ifdef NO_LEGACY_FIELDS
		tmpTokenList[0] = new Token<char>(Buffer ZAIMONI_ISO_PARAM(Buffer_size), filename);
#else
		zaimoni::autovalarray_ptr_throws<char> tmp(strlen(filename));
		memmove(tmp.c_array(),filename,strlen(filename));
		tmpTokenList[0] = new Token<char>(Buffer ZAIMONI_ISO_PARAM(Buffer_size), tmp.release());
#endif
#ifdef ZAIMONI_FORCE_ISO
//		Buffer_size = 0;	// dead code; 
#endif
		}
	catch(...)
		{
		free(Buffer);
		throw;
		}

	// next: split on newline, to simplify spotting preprocessing-directives vs file to be preprocessed
	tmpTokenList[0]->ltrim(strspn(tmpTokenList[0]->data(),"\n"));
	if (tmpTokenList[0]->empty()) return TokenList.reset(),true;

	if (lang.BreakTokenOnNewline)
		{
		const char* newline_where = strchr(tmpTokenList.back()->data(),'\n');
		while(newline_where)
			{
			const size_t offset = newline_where-tmpTokenList.back()->data();
			tmpTokenList.insertNSlotsAt(1,tmpTokenList.size()-1);
			tmpTokenList[tmpTokenList.size()-2] = new Token<char>(*tmpTokenList.back(),offset,0);
			assert('\n'==tmpTokenList.back()->data()[0]);
			if (3<=tmpTokenList.size()) clean_linesplice_whitespace(tmpTokenList,tmpTokenList.size()-3,lang);
			tmpTokenList.back()->ltrim(strspn(tmpTokenList.back()->data(),"\n"));
			if (tmpTokenList.back()->empty())
				{
				tmpTokenList.DeleteIdx(tmpTokenList.size()-1);
				break;
				}
			newline_where = strchr(tmpTokenList.back()->data(),'\n');
			}

		// final cleanup: works for line-continue languages that consider pure whitespace lines meaningless
		if (2<=tmpTokenList.size()) clean_linesplice_whitespace(tmpTokenList,tmpTokenList.size()-2,lang);
		if (!tmpTokenList.empty()) clean_whitespace(tmpTokenList,tmpTokenList.size()-1,lang);
		}

	// if the language approves, flush leading whitespace
	// do not trim trailing whitespace at this time: this breaks error reporting for incomplete C [wide/narrow] character/string literals
	//! \todo work out how to handle tab stops cleanly
	{
	size_t i = tmpTokenList.size();
	while(0<i)
		{
		assert(tmpTokenList[i-1]);
		size_t LeadingWS = strspn(tmpTokenList[--i]->data(),lang.WhiteSpace+1);
		tmpTokenList[i]->ltrim(LeadingWS);
		assert(!tmpTokenList[i]->empty());
		}
	}

	// correct parent_dir
	if (!tmpTokenList.empty())
		{
#ifdef NO_LEGACY_FIELDS
		decltype(auto) tmp = canonical_cache<std::filesystem::path>::get().track(std::filesystem::canonical(filename));
		size_t j = tmpTokenList.size();
		do	tmpTokenList[--j]->src = tmp;
		while (0 < j);
#else
		char workspace[FILENAME_MAX];
		z_realpath(workspace,filename);
		zaimoni::flyweight<const char> tmp(C_string_to_flyweight(workspace,strlen(workspace)));
		size_t j = tmpTokenList.size();
		do	tmpTokenList[--j]->parent_dir = tmp;
		while(0<j);
#endif
		};
	swap(tmpTokenList,TokenList);
	}
	
#ifndef NDEBUG
	// post-condition testing
	{
	size_t i = TokenList.size();
	while(0<i)
		{
		SUCCEED_OR_DIE(NULL!=TokenList[--i]);
		SUCCEED_OR_DIE(0<TokenList[i]->size());
		}
	}
#endif

	return true;
}

//! \throw std::bad_alloc
bool load_raw_sourcefile(autovalarray_ptr<Token<char>* >& TokenList, const char* const filename)
{
	autovalarray_ptr<Token<char>* > tmpTokenList(1);
	char* Buffer = NULL;
#ifndef ZAIMONI_FORCE_ISO
#	define Buffer_size ArraySize(Buffer)
#else
	size_t Buffer_size = 0;
#endif

	assert(filename && *filename);

	// XXX should return true for empty files XXX
	if (!GetBinaryFileImage(filename, Buffer ZAIMONI_ISO_PARAM(Buffer_size))) return false;
	ConvertBinaryModeToTextMode(Buffer ZAIMONI_ISO_PARAM(Buffer_size));

	// if target language needs a warning for not ending in \n, emit one here
	// but we normalize to that
	TrimMandatoryTerminalNewline(Buffer ZAIMONI_ISO_PARAM(Buffer_size), filename);

	try	{
		tmpTokenList[0] = new Token<char>(Buffer ZAIMONI_ISO_PARAM(Buffer_size), filename);
#ifdef ZAIMONI_FORCE_ISO
//		Buffer_size = 0;	// dead code
#endif
		}
	catch(...)
		{
		free(Buffer);
		throw;
		};

	const char* newline_where = strchr(tmpTokenList.back()->data(),'\n');
	while(newline_where)
		{
		const size_t offset = newline_where-tmpTokenList.back()->data();
		tmpTokenList.insertNSlotsAt(1,tmpTokenList.size()-1);
		tmpTokenList[tmpTokenList.size()-2] = new Token<char>(*tmpTokenList.back(),offset,0);
		assert('\n'==tmpTokenList.back()->data()[0]);
		tmpTokenList.back()->ltrim(strspn(tmpTokenList.back()->data(),"\n"));
		if (tmpTokenList.back()->empty())
			{
			tmpTokenList.DeleteIdx(tmpTokenList.size()-1);
			break;
			}
		newline_where = strchr(tmpTokenList.back()->data(),'\n');
		}
	return swap(tmpTokenList,TokenList),true;
}
