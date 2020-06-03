// Token.hpp
// (C)2009 Kenneth Boyd, license: MIT.txt

#ifndef ZAIMONI_STL_LEXPARSE_TOKEN_HPP
#define ZAIMONI_STL_LEXPARSE_TOKEN_HPP 1

#include "MetaToken.hpp"
#include "std.h"

namespace zaimoni {

template<class T>
class Token : public MetaToken<T>
{
public:
	lex_flags flags;

	Token() noexcept : flags(0) {}
#ifndef ZAIMONI_FORCE_ISO
	Token(T*& src, const char* _SourceFileName) : MetaToken<T>(src,_SourceFileName),flags(0) {}
#else
	Token(T*& src, size_t src_len, const char* _SourceFileName) : MetaToken<T>(src,src_len,_SourceFileName),flags(0) {}
#endif
#ifndef ZAIMONI_FORCE_ISO
	Token(T*& src, const std::filesystem::path& src_path) : MetaToken<T>(src, src_path), flags(0) {}
#else
	Token(T*& src, size_t src_len, const std::filesystem::path& src_path) : MetaToken<T>(src, src_len, src_path), flags(0) {}
#endif
#ifndef ZAIMONI_FORCE_ISO
	Token(T*& src, std::filesystem::path&& src_path) : MetaToken<T>(src, std::move(src_path)), flags(0) {}
#else
	Token(T*& src, size_t src_len, std::filesystem::path&& src_path) : MetaToken<T>(src, src_len, std::move(src_path)), flags(0) {}
#endif
	Token(MetaToken<T>& src,size_t token_len,unsigned long new_flags) : MetaToken<T>(src, token_len), flags(new_flags) { src.prefix_chop(token_len); } // prefix-chop constructor
	Token(const MetaToken<T>& src,size_t offset,size_t token_len,unsigned long new_flags) : MetaToken<T>(src, offset, token_len), flags(new_flags) {} // substring constructor
	Token(const T* const src,size_t offset,size_t token_len,unsigned long new_flags) : MetaToken<T>(src, offset, token_len), flags(new_flags) {} // substring constructor
	Token(const Token& src) = default;
	Token(Token&& src) = default;
	Token& operator=(const Token& src) = default;
	Token& operator=(Token&& src) = default;
	virtual ~Token() = default;

	bool is_unclassified() const { return !flags; }
};

}	// namespace zaimoni

#endif
