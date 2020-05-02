// _CSupport3.hpp
// (C)2010 Kenneth Boyd, license: MIT.txt

#include "Zaimoni.STL/Compiler.h"
#include "Zaimoni.STL/POD.hpp"

inline constexpr const POD_pair<const char*, size_t> pragma_relay_keywords[]
	= { DICT_STRUCT("_ZCC_FP_CONTRACT_OFF"),
		DICT_STRUCT("_ZCC_FP_CONTRACT_DEFAULT"),
		DICT_STRUCT("_ZCC_FP_CONTRACT_ON"),
		DICT_STRUCT("_ZCC_FENV_ACCESS_OFF"),
		DICT_STRUCT("_ZCC_FENV_ACCESS_DEFAULT"),
		DICT_STRUCT("_ZCC_FENV_ACCESS_ON"),
		DICT_STRUCT("_ZCC_CX_LIMITED_RANGE_OFF"),
		DICT_STRUCT("_ZCC_CX_LIMITED_RANGE_DEFAULT"),
		DICT_STRUCT("_ZCC_CX_LIMITED_RANGE_ON"),
		DICT_STRUCT("_ZCC_enable_typeid")
};

#define RELAY_ZCC_ENABLE_TYPEID 9
#define PRAGMA_RELAY_KEYWORDS_STRICT_UB STATIC_SIZE(pragma_relay_keywords)

