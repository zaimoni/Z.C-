// filesystem.cpp
// (C)2009 Kenneth Boyd, license: MIT.txt

#include "filesystem.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Zaimoni.STL/Compiler.h"
#include "Zaimoni.STL/Logging.h"
#include "Zaimoni.STL/flat_alg.h"

#include "Zaimoni.STL/Pure.C/comptest.h"
#ifdef ZAIMONI_HAS_MICROSOFT_IO_H
#include <direct.h>
#else
 #include <unistd.h>	// for getcwd; POSIX
#endif

const std::filesystem::path& self_path(const char* const _arg)
{
	static std::filesystem::path ooao;
	if (!_arg || !*_arg) return ooao;
	if (!ooao.empty()) return ooao;	// XXX invariant failure \todo debug mode should hard-error
	// should be argv[0], which exists as it is our name
	auto base = std::filesystem::path(origin_dir);
retry:
	auto test(std::filesystem::path(base) /= _arg);
	try {
		ooao = std::filesystem::canonical(test);
	} catch(std::exception& e) {
		// don't have real relative path.  Fake it for the test driver case
		if ('.' != _arg[0]) {
			auto next = base.parent_path();
			if (next != base) {
				base = std::move(next);
				goto retry;
			}
		}
		// but not being run from home directory (e.g, UNIX test driver)
		INFORM("failed to set up self-path");
		INFORM(_arg);
		INFORM(base.generic_string().data());
		FATAL(e.what());
	}
	return ooao;
}

// \todo this and z_realpath are made obsolete by C++17 filesystem
EXTERN_C void z_dirname(char* target_dirbuf,const char* const src_path)
{
	SUCCEED_OR_DIE(!zaimoni::is_empty_string(src_path));
	SUCCEED_OR_DIE(NULL!=target_dirbuf);
	strcpy(target_dirbuf,src_path);
	char* last_sep = strrchr(target_dirbuf,ZAIMONI_PATH_SEP_CHAR);
	if (NULL==last_sep)
		strcpy(target_dirbuf,".");
	else{	// POSIX: not correct for pathnames starting with //
			// WINDOWS: correct enough (handles the intended use cases)
		last_sep[(last_sep==target_dirbuf) ? 1 : 0] = '\x00';
		}
}
