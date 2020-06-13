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

EXTERN_C const char* self_path = NULL;
static char self_full_path[FILENAME_MAX];

EXTERN_C void bootstrap_filesystem(const char* const self_arg)
{
	// start library target
	if (NULL!=self_arg) self_path = z_realpath(self_full_path,self_arg);
	// end library target
}

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

