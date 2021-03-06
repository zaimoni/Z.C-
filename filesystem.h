// filesystem.h

#ifndef FILESYSTEM_H
#define FILESYSTEM_H 1

// 2020-06-13: C API made sense back in 2010...not so much now
#include <filesystem>

inline static const std::filesystem::path origin_dir(std::filesystem::current_path());
const std::filesystem::path& self_path(const char* const _arg = 0);

#ifdef __cplusplus
extern "C"
{
#endif

/*! 
 * loosely inspired by POSIX dirname.
 * 
 * \param target_dirbuf: character array to be initialized with the directory name
 * \param src_path : path to extract directory part from
 */
void z_dirname(char* target_dirbuf,const char* const src_path);

#ifdef __cplusplus
}	/* extern "C" */
#endif

#endif
