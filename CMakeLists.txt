cmake_minimum_required(VERSION 3.15)
project(Z.C++ C CXX)
set(CMAKE_CXX_STANDARD 17)

if (NOT DEFINED $CACHE{ZSTL_CMAKE_SUFFIX})
	string(FIND ${CMAKE_INSTALL_PREFIX} "/" TMP_INDEX REVERSE)
	string(SUBSTRING ${CMAKE_INSTALL_PREFIX} ${TMP_INDEX} -1 TMP_CMAKE_SUFFIX)
	set(ZSTL_CMAKE_SUFFIX ${TMP_CMAKE_SUFFIX} CACHE STRING "platform suffix")
endif()

if (NOT DEFINED $CACHE{ZSTL_COMPILER})
	string(FIND ${CMAKE_C_COMPILER} "/" TMP_INDEX REVERSE)
	math(EXPR TMP_INDEX "${TMP_INDEX}+1")
	string(SUBSTRING ${CMAKE_C_COMPILER} ${TMP_INDEX} -1 TMP_COMPILER_STEM)
	string(FIND ${TMP_COMPILER_STEM} "." TMP_INDEX REVERSE)
	string(SUBSTRING ${TMP_COMPILER_STEM} 0 ${TMP_INDEX} TMP_COMPILER_STEM)
	set(ZSTL_COMPILER ${TMP_COMPILER_STEM} CACHE STRING "compiler")
endif()

add_subdirectory(Zaimoni.STL)

if ($CACHE{ZSTL_USLEEP_CODE})
	add_compile_definitions(ZSTL_USLEEP_CODE=$CACHE{ZSTL_USLEEP_CODE})
endif()

# might actually need this?
include_directories(${CMAKE_BINARY_DIR})

add_executable(z_cpp z_cpp.cpp to_stdout.cpp errors.cpp langroute.cpp AtomicString.cpp str_aux.cpp lex_core.cpp filesystem.cpp uchar_blob.cpp unsigned_var_int.cpp CPUInfo.cpp type_system.cpp ParseTree.cpp type_spec.cpp CSupport_pp.cpp C_PPNumCore.cpp C_PPDecimalInteger.cpp C_PPOctalInteger.cpp C_PPHexInteger.cpp C_PPDecimalFloat.cpp C_PPHexFloat.cpp unsigned_aux.cpp Trigraph.cpp Flat_UNI.cpp end_lc.cpp end_nl.cpp load_src.cpp CPreproc.cpp CPreproc_autogen.cpp)
target_link_libraries(z_cpp z_langconf z_log_adapter z_stdio_log z_mutex z_format_util z_stdio_c z_memory)
target_link_options(z_cpp PRIVATE /FORCE:MULTIPLE)	# for z_memory
target_compile_definitions(z_cpp PUBLIC BUILD_Z_CPP)
# add_dependencies(... AutoDetect)

add_executable(zcc zcc.cpp to_stdout.cpp errors.cpp langroute.cpp AtomicString.cpp str_aux.cpp lex_core.cpp filesystem.cpp uchar_blob.cpp unsigned_var_int.cpp CPUInfo.cpp struct_type.cpp type_system.cpp ParseTree.cpp type_spec.cpp CSupport.cpp C_PPNumCore.cpp C_PPDecimalInteger.cpp C_PPOctalInteger.cpp C_PPHexInteger.cpp C_PPDecimalFloat.cpp C_PPHexFloat.cpp unsigned_aux.cpp Trigraph.cpp Flat_UNI.cpp end_lc.cpp end_nl.cpp load_src.cpp CPreproc.cpp CPreproc_autogen.cpp ZParser.cpp)
target_link_libraries(zcc z_langconf z_log_adapter z_stdio_log z_mutex z_format_util z_stdio_c z_memory)
target_link_options(zcc PRIVATE /FORCE:MULTIPLE)	# for z_memory
target_compile_definitions(zcc PUBLIC BUILD_ZCC)
# add_dependencies(... AutoDetect)

add_custom_command(TARGET z_cpp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:z_memory>
        $<TARGET_FILE_DIR:z_cpp>)
add_custom_command(TARGET zcc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:z_memory>
        $<TARGET_FILE_DIR:zcc>)

# EXCLUDE_FROM_ALL means "can't test"
