// ParseTree.cpp
// (C)2009,2011 Kenneth Boyd, license: MIT.txt

#include "ParseTree.hpp"

#include "str_aux.h"

#include <memory>
#include <stdexcept>

using namespace zaimoni;

bool (*parse_tree::hook_INC_INFORM)(const parse_tree&) = NULL;
type_system* parse_tree::types = NULL;
const char* parse_tree::active_namespace = NULL;

// in case we have to debug memory corruption
// #define IRRATIONAL_CAUTION 1

bool parse_tree::is_atomic() const
{
	return index_tokens[0].token.first && !index_tokens[1].token.first
		&& _args[0].empty() && _args[1].empty() && _args[2].empty();
}

bool parse_tree::is_raw_list() const
{
	return !index_tokens[0].token.first &&	!index_tokens[1].token.first
		&& !_args[0].empty() && _args[1].empty() && _args[2].empty();
}

#if 0
// #ifndef ZAIMONI_FORCE_ISO
bool parse_tree::syntax_ok() const
{
	if (data<0>() && !_memory_block_start_valid(data<0>())) return false;
	if (data<1>() && !_memory_block_start_valid(data<1>())) return false;
	if (data<2>() && !_memory_block_start_valid(data<2>())) return false;

	if (own_index_token<0>())
		{
		if (!index_tokens[0].token.first) return false;
		if (!_memory_block_start_valid(index_tokens[0].token.first))
			return false;
		};
	if (own_index_token<1>())
		{
		if (!index_tokens[1].token.first) return false;
		if (!_memory_block_start_valid(index_tokens[1].token.first))
			return false;
		};

	size_t i = 0;
	while(size<0>()>i) if (!data<0>()[i++].syntax_ok()) return false;
	i = 0;
	while(size<1>()>i) if (!data<1>()[i++].syntax_ok()) return false;
	i = 0;
	while(size<2>()>i) if (!data<2>()[i++].syntax_ok()) return false;
	if (!type_code.syntax_ok()) return false;
	return true;
}

template<class iter,class src>
bool entangled_with(src& x,iter begin,iter end)
{
	while(begin!=end) if ((begin++)->entangled_with(x)) return true;
	return false;
}

bool parse_tree::entangled_with(const type_spec& x) const
{
	if (x.entangled_with(type_code)) return true;
	if (::entangled_with(x,begin<0>(),end<0>())) return true;
	if (::entangled_with(x,begin<1>(),end<1>())) return true;
	if (::entangled_with(x,begin<2>(),end<2>())) return true;
	return false;
}

bool parse_tree::entangled_with(const parse_tree& x) const
{
	if (x.type_code.entangled_with(type_code)) return true;
	if (begin<0>() && x.begin<0>() && begin<0>()==x.begin<0>()) return true;
	if (begin<0>() && x.begin<1>() && begin<0>()==x.begin<1>()) return true;
	if (begin<0>() && x.begin<2>() && begin<0>()==x.begin<2>()) return true;
	if (begin<1>() && x.begin<0>() && begin<1>()==x.begin<0>()) return true;
	if (begin<1>() && x.begin<1>() && begin<1>()==x.begin<1>()) return true;
	if (begin<1>() && x.begin<2>() && begin<1>()==x.begin<2>()) return true;
	if (begin<2>() && x.begin<0>() && begin<2>()==x.begin<0>()) return true;
	if (begin<2>() && x.begin<1>() && begin<2>()==x.begin<1>()) return true;
	if (begin<2>() && x.begin<2>() && begin<2>()==x.begin<2>()) return true;

	size_t i = 0;
	while(size<0>()>i)
		{
		if (::entangled_with(data<0>()[i],x.begin<0>(),x.end<0>())) return true;
		if (::entangled_with(data<0>()[i],x.begin<1>(),x.end<1>())) return true;
		if (::entangled_with(data<0>()[i],x.begin<2>(),x.end<2>())) return true;
		++i;
		}
	i = 0;
	while(size<1>()>i)
		{
		if (::entangled_with(data<1>()[i],x.begin<0>(),x.end<0>())) return true;
		if (::entangled_with(data<1>()[i],x.begin<1>(),x.end<1>())) return true;
		if (::entangled_with(data<1>()[i],x.begin<2>(),x.end<2>())) return true;
		++i;
		}
	i = 0;
	while(size<2>()>i)
		{
		if (::entangled_with(data<2>()[i],x.begin<0>(),x.end<0>())) return true;
		if (::entangled_with(data<2>()[i],x.begin<1>(),x.end<1>())) return true;
		if (::entangled_with(data<2>()[i],x.begin<2>(),x.end<2>())) return true;
		++i;
		}
	return false;
}

bool parse_tree::self_entangled() const
{
	if (data<0>() && data<1>() && data<0>()==data<1>()) return true;
	if (data<0>() && data<2>() && data<0>()==data<2>()) return true;
	if (data<1>() && data<2>() && data<1>()==data<2>()) return true;

	size_t i = 0;
	while(size<0>()>i)
		{
		if (data<0>()[i].self_entangled()) return true;
		if (data<0>()[i].entangled_with(type_code)) return true;
		if (0<i && ::entangled_with(data<0>()[i],begin<0>(),begin<0>()+i)) return true;
		++i;
		}
	i = 0;
	while(size<1>()>i)
		{
		if (data<1>()[i].self_entangled()) return true;
		if (data<1>()[i].entangled_with(type_code)) return true;
		if (0<i && ::entangled_with(data<1>()[i],begin<1>(),begin<1>()+i)) return true;
		++i;
		}
	i = 0;
	while(size<2>()>i)
		{
		if (data<2>()[i].self_entangled()) return true;
		if (data<2>()[i].entangled_with(type_code)) return true;
		if (0<i && ::entangled_with(data<2>()[i],begin<2>(),begin<2>()+i)) return true;
		++i;
		}
	return false;
}
#endif

void parse_tree::clear()
{
	zaimoni::clear<2>(index_tokens);
	_args[0].NULLPtr();
	_args[1].NULLPtr();
	_args[2].NULLPtr();
	flags = 0;
	subtype = 0;
	type_code.clear();
}

static void _destroy(zaimoni::POD_autoarray_ptr<parse_tree*>& target)
{
	if (!target.empty()) {
		size_t i = target.size();
		do  {
			if (decltype(auto) x = target.c_array()[--i]) {
				x->destroy();
				zaimoni::_single_flush(x);
				x = 0;
			}
		} while (0 < i);
		free(target.release());
	}
}

// source code locations must survive: some users need to retain that when rewriting
void parse_tree::destroy()
{
	_destroy(_args[2]);
	_destroy(_args[1]);
	_destroy(_args[0]);
	if (own_index_token<1>()) free(const_cast<char*>(index_tokens[1].token.first));
	if (own_index_token<0>()) free(const_cast<char*>(index_tokens[0].token.first));
	index_tokens[1].token.first = 0;
	index_tokens[0].token.first = 0;
	flags = 0;
	subtype = 0;
	type_code.destroy();
}

void parse_tree::core_flag_update()
{
	size_t i = size<0>();
	bool is_constant = true;
	bool is_invalid = false;
	flags &= RESERVED_MASK;	// just in case
	while(0<i)
		{
		if (decltype(auto) x = data<0>()[--i]) {
			if (!(CONSTANT_EXPRESSION & x->flags)) is_constant = false;
			if (INVALID & x->flags) is_invalid = true;
		} else is_invalid = true;
		};
	i = size<1>();
	while(0<i)
		{
		if (decltype(auto) x = data<1>()[--i]) {
			if (!(CONSTANT_EXPRESSION & x->flags)) is_constant = false;
			if (INVALID & x->flags) is_invalid = true;
		}
		else is_invalid = true;
		};
	i = size<2>();
	while(0<i)
		{
		if (decltype(auto) x = data<2>()[--i]) {
			if (!(CONSTANT_EXPRESSION & x->flags)) is_constant = false;
			if (INVALID & x->flags) is_invalid = true;
		}
		else is_invalid = true;
		};
	flags |= CONSTANT_EXPRESSION*is_constant+INVALID*is_invalid;
}

bool parse_tree::collapse_matched_pair(parse_tree& src, const zaimoni::POD_pair<size_t,size_t>& target)
{
	const size_t zap_span = target.second-target.first-1;
	std::unique_ptr<parse_tree> tmp(zaimoni::_new_buffer_nonNULL_throws<parse_tree>(1));
	// we actually are wrapping tokens
	if (0<zap_span)
		{
		if (!tmp->resize<0>(zap_span)) return false;
		std::remove_reference_t<decltype(*(tmp->c_array<0>()))> test;
		static_assert(std::is_pod_v<std::remove_reference_t<decltype(*(tmp->c_array<0>()))> >);
		const auto m_src = src.c_array<0>() + target.first + 1;
		memmove(tmp->c_array<0>(), m_src, (zap_span) * sizeof(decltype(src.data<0>())));
		std::fill_n(m_src, zap_span, nullptr);
		}
	tmp->take_weak_token<0, 0>(*src.c_array<0>()[target.first]);
	tmp->take_weak_token<1, 0>(*src.c_array<0>()[target.second]);

	size_t i = zap_span+1;
	src.DeleteNSlotsAt<0>(zap_span+1,target.first+1);
	{
	decltype(auto) dest = src.c_array<0>()[target.first];
	dest->destroy();
	FREE(dest);
	dest = tmp.release();
	}
#ifdef IRRATIONAL_CAUTION
	assert(src.syntax_ok());
	assert(!src.self_entangled());
#endif
	return true;
}

bool weak_token::CopyInto(weak_token& dest, bool owns) const
{
	memmove(&dest, this, sizeof(weak_token));
	if (owns) dest.token.first = C_make_string(token.first, token.second);
	return owns;
}

void value_copy(parse_tree** const dest, const parse_tree* const * const src, size_t ub)
{
	assert(dest);
	assert(src);
	while (0 < ub) {
		if (auto stage = src[--ub]) {
			dest[ub] = zaimoni::_new_buffer_nonNULL_throws<parse_tree>(1);
			value_copy(*dest[ub], *stage);
		} else throw std::logic_error("null copy");
	}
}

// ACID; throws std::bad_alloc on failure
void value_copy(parse_tree& dest, const parse_tree& src)
{	// favor ACID
	parse_tree_class tmp;

	value_copy(tmp.type_code,src.type_code);
	if (!src.empty<0>())
		{
		size_t i = src.size<0>();
		if (!tmp.resize<0>(i)) throw std::bad_alloc();
		value_copy(tmp.c_array<0>(), src.data<0>(), i);
		};
	if (!src.empty<1>())
		{
		size_t i = src.size<1>();
		if (!tmp.resize<1>(i)) throw std::bad_alloc();
		value_copy(tmp.c_array<1>(), src.data<1>(), i);
		}
	if (!src.empty<2>())
		{
		size_t i = src.size<2>();
		if (!tmp.resize<2>(i)) throw std::bad_alloc();
		value_copy(tmp.c_array<2>(), src.data<2>(), i);
		}
	// would like a value_copy for weak_token
	tmp.control_index_token<0>(src.index_tokens[0].CopyInto(tmp.index_tokens[0], src.own_index_token<0>()));
	tmp.control_index_token<1>(src.index_tokens[1].CopyInto(tmp.index_tokens[1], src.own_index_token<1>()));
	tmp.flags = src.flags;
	tmp.subtype = src.subtype;

	tmp.MoveInto(dest);
#ifdef IRRATIONAL_CAUTION
	assert(dest.syntax_ok());
	assert(!dest.self_entangled());
#endif
}

void parse_tree::MoveInto(parse_tree& dest)
{
	dest.destroy();
	dest = *this;
	clear();
#ifdef IRRATIONAL_CAUTION
	assert(dest.syntax_ok());
	assert(!dest.self_entangled());
#endif
}

void parse_tree::OverwriteInto(parse_tree& dest)
{
	dest = *this;
	clear();
}

void parse_tree::_eval_to_arg(size_t arg_idx, size_t i)
{
	parse_tree& staging = *c_array(arg_idx)[i];
	parse_tree tmp = staging;
	staging.clear();	// \todo redundant?
	destroy();	// reference dies here
	*this = tmp;
#ifdef IRRATIONAL_CAUTION
	assert(syntax_ok());
	assert(!self_entangled());
#endif
}

//! the new slots have no content, OverwriteInto won't leak memory
bool parse_tree::_resize(const size_t arg_idx,size_t n)
{
	assert(STATIC_SIZE(_args)>arg_idx);
	const size_t old_size = size(arg_idx);
	if (!_args[arg_idx].Resize(n)) return false;
#if ACTIVATE_PARSE_TREE_C_ARRAY
	while (old_size < n) c_array(arg_idx)[--n]->clear();
#else
	while(old_size<n) c_array(arg_idx)[--n].clear();
#endif
	return true;
}

void INC_INFORM(const parse_tree& src)
{	// generally...
	if (parse_tree::hook_INC_INFORM && parse_tree::hook_INC_INFORM(src)) return;
	// prefix data
#define USER_MASK (ULONG_MAX-((1U<<parse_tree::PREDEFINED_STRICT_UB)-1))
	const lex_flags my_rank = src.flags & USER_MASK;
	bool need_parens = (1==src.size<1>()) ? my_rank>(src.front<1>().flags & USER_MASK) : false;
	bool sp = false;	// "need space here"
	if (need_parens) INC_INFORM('(');
	size_t i = 0;
	while(src.size<1>()>i)
		{
		if (sp) INC_INFORM(' ');
#if ACTIVATE_PARSE_TREE_C_ARRAY
		sp = !(src.data<1>()[i]->flags & parse_tree::GOOD_LINE_BREAK);
		INC_INFORM(*src.data<1>()[i++]);
#else
		sp = !(src.data<1>()[i].flags & parse_tree::GOOD_LINE_BREAK);
		INC_INFORM(src.data<1>()[i++]);
#endif
		}
	if (need_parens)
		{
		INC_INFORM(')');
		sp = false;
		};
	// first index token
	if (src.index_tokens[0].token.first)
		{
		if (sp) INC_INFORM(' ');
		INC_INFORM(src.index_tokens[0]);
		sp = true;
		}
	// infix data
	need_parens = (1==src.size<0>()) ? my_rank>(src.front<0>().flags & USER_MASK) : false;
	if (need_parens)
		{
		INC_INFORM('(');
		sp = false;
		}
	i = 0;
	while(src.size<0>()>i)
		{
		if (sp) INC_INFORM(' ');
#if ACTIVATE_PARSE_TREE_C_ARRAY
		sp = !(src.data<0>()[i]->flags & parse_tree::GOOD_LINE_BREAK);
		INC_INFORM(*src.data<0>()[i++]);
#else
		sp = !(src.data<0>()[i].flags & parse_tree::GOOD_LINE_BREAK);
		INC_INFORM(src.data<0>()[i++]);
#endif
		}
	if (need_parens)
		{
		INC_INFORM(')');
		sp = false;
		};
	// second index token
	if (src.index_tokens[1].token.first)
		{
		if (sp) INC_INFORM(' ');
		INC_INFORM(src.index_tokens[1]);
		sp = true;
		}
	// postfix data
	need_parens = (1 == src.size<2>()) ? my_rank > (src.front<2>().flags & USER_MASK) : false;
	if (need_parens)
		{
		INC_INFORM('(');
		sp = false;
		}
	i = 0;
	while(src.size<2>()>i)
		{
		if (sp) INC_INFORM(' ');
#if ACTIVATE_PARSE_TREE_C_ARRAY
		sp = !(src.data<2>()[i]->flags & parse_tree::GOOD_LINE_BREAK);
		INC_INFORM(*src.data<2>()[i++]);
#else
		sp = !(src.data<2>()[i].flags & parse_tree::GOOD_LINE_BREAK);
		INC_INFORM(src.data<2>()[i++]);
#endif
		}
	if (need_parens) INC_INFORM(')');
	if (src.flags & parse_tree::GOOD_LINE_BREAK) INC_INFORM('\n');
#undef USER_MASK
}

// slicing copy constructor
parse_tree_class::parse_tree_class(const parse_tree& src,size_t begin,size_t end,size_t dest_idx)
{
	assert(STATIC_SIZE(_args)>dest_idx);
	assert(begin<src.size(dest_idx));
	assert(end<=src.size(dest_idx));
	this->clear();
	if (begin>=end) return;
	const size_t i = end-begin;
#if ACTIVATE_PARSE_TREE_C_ARRAY
	if (1 == i) value_copy(*this, *src.data(dest_idx)[begin]);
#else
	if (1==i) value_copy(*this,src.data(dest_idx)[begin]);
#endif
	else{
		if (!resize(dest_idx,i)) throw std::bad_alloc();
		zaimoni::autotransform_n<void (*)(parse_tree&,const parse_tree&)>(c_array(dest_idx),src.data(dest_idx)+begin,i,value_copy);
		}	
}
