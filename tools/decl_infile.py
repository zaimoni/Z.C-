#!/usr/bin/python
# an auxilliary script for generating the test files
# verified for Python 2.5, 2.6, 2.7
# fails: Python 3.0, 3.1, 3.2 (xrange not supported)
# (C)2011 Kenneth Boyd, license: MIT.txt

# target files
target_files = ['Pass_enum_def.in', 'Pass_struct_def.in', 'Pass_union_def.in']
target_files2 = ['Pass_enum_def_decl.in', 'Pass_struct_def_decl.in', 'Pass_union_def_decl.in']
target_files3 = ['Pass_enum_def2.hpp', 'Pass_struct_def2.hpp', 'Pass_union_def2.hpp']

invariant_header_lines = [
'SUFFIXES h hpp\n',
'OBJECTLIKE_MACRO THREAD_LOCAL _Thread_local thread_local\n',
'OBJECTLIKE_MACRO STATIC_ASSERT _Static_assert static_assert\n',
'OBJECTLIKE_MACRO TYPE_LINKAGE_CODE 0 2\n',
'OBJECTLIKE_MACRO DEFAULT_CONST_LINKAGE_CODE 2 1\n',
'// (C)2009,2011 Kenneth Boyd, license: MIT.txt\n'
]

context = {	'Pass_enum_def.in':'// using singly defined enum\n',
			'Pass_struct_def.in':'// using singly defined struct\n',
			'Pass_union_def.in':'// using singly defined union\n',
			'Pass_enum_def_decl.in':'// using singly defined enum\n',
			'Pass_struct_def_decl.in':'// using singly defined struct\n',
			'Pass_union_def_decl.in':'// using singly defined union\n',
			'Pass_enum_def2.hpp':'// using singly defined enum\n',
			'Pass_struct_def2.hpp':'// using singly defined struct\n',
			'Pass_union_def2.hpp':'// using singly defined union\n'}

global_define = {	'Pass_enum_def.in':'\nenum good_test {\n\tx_factor = 1\n};\n\n',
					'Pass_struct_def.in':'\nstruct good_test {\n\tint x_factor\n};\n\n',
					'Pass_union_def.in':'\nunion good_test {\n\tint x_factor\n};\n\n',
					'Pass_enum_def_decl.in':'\nenum good_test {\n\tx_factor = 1\n} y;\n\n',
					'Pass_struct_def_decl.in':'\nstruct good_test {\n\tint x_factor\n} y;\n\n',
					'Pass_union_def_decl.in':'\nunion good_test {\n\tint x_factor\n};\n\n',
					'Pass_enum_def2.hpp':'\nenum good_test {\n\tx_factor = 1\n};\n\n',
					'Pass_struct_def2.hpp':'\nstruct good_test {\n\tint x_factor\n};\n\n',
					'Pass_union_def2.hpp':'\nunion good_test {\n\tint x_factor\n};\n\n'}

section_comments = ['// ringing the changes on extern\n',
"// ringing the changes on static\n",
'// extern/static not in first position is deprecated, but legal\n',
'// ringing the changes on THREAD_LOCAL extern\n',
'// ringing the changes on THREAD_LOCAL static\n',
'// THREAD_LOCAL extern not in first two positions is deprecated, but legal\n',
'// THREAD_LOCAL static not in first two positions is deprecated, but legal\n']

def enum_decl(i):
	return "enum good_test x"+i

def struct_decl(i):
	return "struct good_test x"+i

def union_decl(i):
	return "union good_test x"+i

def enum_decl_terse(i):
	return "good_test x"+i

def struct_decl_terse(i):
	return "good_test x"+i

def union_decl_terse(i):
	return "good_test x"+i

var_decl = {'Pass_enum_def.in':enum_decl, 'Pass_struct_def.in':struct_decl,
			'Pass_union_def.in':union_decl, 'Pass_enum_def_decl.in':enum_decl,
			'Pass_struct_def_decl.in':struct_decl, 'Pass_union_def_decl.in':union_decl,
			'Pass_enum_def2.hpp':enum_decl, 'Pass_struct_def2.hpp':struct_decl,
			'Pass_union_def2.hpp':union_decl}

var_decl_terse = {	'Pass_enum_def2.hpp':enum_decl_terse, 'Pass_struct_def2.hpp':struct_decl_terse,
					'Pass_union_def2.hpp':union_decl_terse}

def enum_def(i):
	return 'enum good_test'+i+' { x_factor'+i+' = 1 } x_'+i

def struct_def(i):
	return 'struct good_test'+i+' { int x_factor'+i+'; } x_'+i

def union_def(i):
	return 'union good_test'+i+' { int x_factor'+i+'; } x_'+i

var_def = {	'Pass_enum_def.in':enum_def, 'Pass_struct_def.in':struct_def,
			'Pass_union_def.in':union_def, 'Pass_enum_def2.hpp':enum_def,
			'Pass_struct_def2.hpp':struct_def, 'Pass_union_def2.hpp':union_def}

#so...generally (decl family)
#* linkage of struct/union/enum good_test... is none in C, namespace-default in C++ (in practice C++ extern)
#* linkage of variables is static:internal or extern:C/C++ external
#** that is: 1 or 2
#** default linkage: extern in general, const is extern/C static/C++ (2/C 1/C++)

# in general, static const non-volatile variations should warn (the optimizer will go wild)
# on the other hand, C99 6.7.8p10/C1X 6.7.9p10 gives full default initialization (as zero-initialization) so C is fine
# C++0X 8.5p9: static storage duration is zero-initialized so C++ is fine as well
test_qualifiers = [
'extern',
'extern const',
'extern volatile',
'extern const volatile',
'extern volatile const',

'static',
'static const',
'static volatile',
'static const volatile',
'static volatile const',

'const extern',
'volatile extern',
'const extern volatile',
'const volatile extern',
'volatile extern const',
'volatile const extern',

'const static',
'volatile static',
'const static volatile',
'const volatile static',
'volatile static const',
'volatile const static',

'extern THREAD_LOCAL',
'extern THREAD_LOCAL const',
'extern THREAD_LOCAL volatile',
'extern THREAD_LOCAL const volatile',
'extern THREAD_LOCAL volatile const',
'THREAD_LOCAL extern',
'THREAD_LOCAL extern const',
'THREAD_LOCAL extern volatile',
'THREAD_LOCAL extern const volatile',
'THREAD_LOCAL extern volatile const',

'static THREAD_LOCAL',
'static THREAD_LOCAL const',
'static THREAD_LOCAL volatile',
'static THREAD_LOCAL const volatile',
'static THREAD_LOCAL volatile const',
'THREAD_LOCAL static',
'THREAD_LOCAL static const',
'THREAD_LOCAL static volatile',
'THREAD_LOCAL static const volatile',
'THREAD_LOCAL static volatile const',

'extern const THREAD_LOCAL',
'const extern THREAD_LOCAL',
'extern volatile THREAD_LOCAL',
'volatile extern THREAD_LOCAL',
'extern const THREAD_LOCAL volatile',
'extern const volatile THREAD_LOCAL',
'const extern THREAD_LOCAL volatile',
'const extern volatile THREAD_LOCAL',
'const volatile extern THREAD_LOCAL',
'extern volatile THREAD_LOCAL const',
'extern volatile const THREAD_LOCAL',
'volatile extern THREAD_LOCAL const',
'volatile extern const THREAD_LOCAL',
'volatile const extern THREAD_LOCAL',
'THREAD_LOCAL const extern',
'const THREAD_LOCAL extern',
'THREAD_LOCAL volatile extern',
'volatile THREAD_LOCAL extern',
'THREAD_LOCAL const extern volatile',
'THREAD_LOCAL const volatile extern',
'const THREAD_LOCAL extern volatile',
'const THREAD_LOCAL volatile extern',
'const volatile THREAD_LOCAL extern',
'THREAD_LOCAL volatile extern const',
'THREAD_LOCAL volatile const extern',
'volatile THREAD_LOCAL extern const',
'volatile THREAD_LOCAL const extern',
'volatile const THREAD_LOCAL extern',

'static const THREAD_LOCAL',
'const static THREAD_LOCAL',
'static volatile THREAD_LOCAL',
'volatile static THREAD_LOCAL',
'static const THREAD_LOCAL volatile',
'static const volatile THREAD_LOCAL',
'const static THREAD_LOCAL volatile',
'const static volatile THREAD_LOCAL',
'const volatile static THREAD_LOCAL',
'static volatile THREAD_LOCAL const',
'static volatile const THREAD_LOCAL',
'volatile static THREAD_LOCAL const',
'volatile static const THREAD_LOCAL',
'volatile const static THREAD_LOCAL',
'THREAD_LOCAL const static',
'const THREAD_LOCAL static',
'THREAD_LOCAL volatile static',
'volatile THREAD_LOCAL static',
'THREAD_LOCAL const static volatile',
'THREAD_LOCAL const volatile static',
'const THREAD_LOCAL static volatile',
'const THREAD_LOCAL volatile static',
'const volatile THREAD_LOCAL static',
'THREAD_LOCAL volatile static const',
'THREAD_LOCAL volatile const static',
'volatile THREAD_LOCAL static const',
'volatile THREAD_LOCAL const static',
'volatile const THREAD_LOCAL static'
]

def SpawnTestCase(dest_file):
	TargetFile = open(dest_file,'w')
	for line in invariant_header_lines:
		TargetFile.write(line)
	TargetFile.write(context[dest_file])
	TargetFile.write(global_define[dest_file])

	TargetFile.write(section_comments[0])
	for i in xrange(5):
		TargetFile.write(test_qualifiers[i]+' '+var_decl[dest_file](str(i+1))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[1])
	for i in xrange(5):
		TargetFile.write(test_qualifiers[i+5]+' '+var_decl[dest_file](str(i+6))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[2])
	for i in xrange(12):
		TargetFile.write(test_qualifiers[i+10]+' '+var_decl[dest_file](str(i+11))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[3])
	for i in xrange(10):
		TargetFile.write(test_qualifiers[i+22]+' '+var_decl[dest_file](str(i+23))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[4])
	for i in xrange(10):
		TargetFile.write(test_qualifiers[i+32]+' '+var_decl[dest_file](str(i+33))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[5])
	for i in xrange(28):
		TargetFile.write(test_qualifiers[i+42]+' '+var_decl[dest_file](str(i+43))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[6])
	for i in xrange(28):
		TargetFile.write(test_qualifiers[i+70]+' '+var_decl[dest_file](str(i+71))+';\n')
	TargetFile.write('\n')

	TargetFile.write('// define-declares\n')
	TargetFile.write(section_comments[0])
	for i in xrange(5):
		TargetFile.write(test_qualifiers[i]+' '+var_def[dest_file](str(i+1))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[1])
	for i in xrange(5):
		TargetFile.write(test_qualifiers[i+5]+' '+var_def[dest_file](str(i+6))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[2])
	for i in xrange(12):
		TargetFile.write(test_qualifiers[i+10]+' '+var_def[dest_file](str(i+11))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[3])
	for i in xrange(10):
		TargetFile.write(test_qualifiers[i+22]+' '+var_def[dest_file](str(i+23))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[4])
	for i in xrange(10):
		TargetFile.write(test_qualifiers[i+32]+' '+var_def[dest_file](str(i+33))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[5])
	for i in xrange(28):
		TargetFile.write(test_qualifiers[i+42]+' '+var_def[dest_file](str(i+43))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[6])
	for i in xrange(28):
		TargetFile.write(test_qualifiers[i+70]+' '+var_def[dest_file](str(i+71))+';\n')
	TargetFile.write('\n')

#	check ZCC compiler extensions
	TargetFile.write('\n// check ZCC compiler extensions\n')
	TargetFile.write('#if 9<=10000*__ZCC__+100*__ZCC_MINOR__+__ZCC_PATCHLEVEL__\n')
	TargetFile.write('// the type names should have no linkage in C, external linkage in C++\n')
	TargetFile.write('STATIC_ASSERT(TYPE_LINKAGE_CODE==__zcc_linkage(good_test),"good_test has incorrect linkage");\n')
	for i in xrange(5+5+12+10+10+28+18):
		TargetFile.write('STATIC_ASSERT(TYPE_LINKAGE_CODE==__zcc_linkage(good_test'+str(i+1)+'),"good_test'+str(i+1)+' has incorrect linkage");\n')

	TargetFile.write('\n// check external linkage of variables\n')	
	for i in xrange(5):
		TargetFile.write('STATIC_ASSERT(2==__zcc_linkage(x'+str(i+1)+'),"x'+str(i+1)+' has incorrect linkage");\n')
		TargetFile.write('STATIC_ASSERT(2==__zcc_linkage(x_'+str(i+1)+'),"x_'+str(i+1)+' has incorrect linkage");\n')

	for i in xrange(6):
		TargetFile.write('STATIC_ASSERT(2==__zcc_linkage(x'+str(i+11)+'),"x'+str(i+11)+' has incorrect linkage");\n')
		TargetFile.write('STATIC_ASSERT(2==__zcc_linkage(x_'+str(i+11)+'),"x_'+str(i+11)+' has incorrect linkage");\n')

	for i in xrange(10):
		TargetFile.write('STATIC_ASSERT(2==__zcc_linkage(x'+str(i+23)+'),"x'+str(i+23)+' has incorrect linkage");\n')
		TargetFile.write('STATIC_ASSERT(2==__zcc_linkage(x_'+str(i+23)+'),"x_'+str(i+23)+' has incorrect linkage");\n')

	for i in xrange(28):
		TargetFile.write('STATIC_ASSERT(2==__zcc_linkage(x'+str(i+43)+'),"x'+str(i+43)+' has incorrect linkage");\n')
		TargetFile.write('STATIC_ASSERT(2==__zcc_linkage(x_'+str(i+43)+'),"x_'+str(i+43)+' has incorrect linkage");\n')

	TargetFile.write('\n// check internal linkage of variables\n')
	for i in xrange(5):
		TargetFile.write('STATIC_ASSERT(1==__zcc_linkage(x'+str(i+6)+'),"x'+str(i+6)+' has incorrect linkage");\n')
		TargetFile.write('STATIC_ASSERT(1==__zcc_linkage(x_'+str(i+6)+'),"x_'+str(i+6)+' has incorrect linkage");\n')

	for i in xrange(6):
		TargetFile.write('STATIC_ASSERT(1==__zcc_linkage(x'+str(i+17)+'),"x'+str(i+17)+' has incorrect linkage");\n')
		TargetFile.write('STATIC_ASSERT(1==__zcc_linkage(x_'+str(i+17)+'),"x_'+str(i+17)+' has incorrect linkage");\n')

	for i in xrange(10):
		TargetFile.write('STATIC_ASSERT(1==__zcc_linkage(x'+str(i+33)+'),"x'+str(i+33)+' has incorrect linkage");\n')
		TargetFile.write('STATIC_ASSERT(1==__zcc_linkage(x_'+str(i+33)+'),"x_'+str(i+33)+' has incorrect linkage");\n')

	for i in xrange(28):
		TargetFile.write('STATIC_ASSERT(1==__zcc_linkage(x'+str(i+71)+'),"x'+str(i+71)+' has incorrect linkage");\n')
		TargetFile.write('STATIC_ASSERT(1==__zcc_linkage(x_'+str(i+71)+'),"x_'+str(i+71)+' has incorrect linkage");\n')

	TargetFile.write('#endif\n')
	TargetFile.close()

def SpawnTestCase2(dest_file):
	# first part copied from SpawnTestCase
	TargetFile = open(dest_file,'w')
	for line in invariant_header_lines:
		TargetFile.write(line)
	TargetFile.write(context[dest_file])
	TargetFile.write(global_define[dest_file])

	TargetFile.write(section_comments[0])
	for i in xrange(5):
		TargetFile.write(test_qualifiers[i]+' '+var_decl[dest_file](str(i+1))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[1])
	for i in xrange(5):
		TargetFile.write(test_qualifiers[i+5]+' '+var_decl[dest_file](str(i+6))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[2])
	for i in xrange(12):
		TargetFile.write(test_qualifiers[i+10]+' '+var_decl[dest_file](str(i+11))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[3])
	for i in xrange(10):
		TargetFile.write(test_qualifiers[i+22]+' '+var_decl[dest_file](str(i+23))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[4])
	for i in xrange(10):
		TargetFile.write(test_qualifiers[i+32]+' '+var_decl[dest_file](str(i+33))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[5])
	for i in xrange(28):
		TargetFile.write(test_qualifiers[i+42]+' '+var_decl[dest_file](str(i+43))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[6])
	for i in xrange(28):
		TargetFile.write(test_qualifiers[i+70]+' '+var_decl[dest_file](str(i+71))+';\n')
	TargetFile.write('\n')

#	check ZCC compiler extensions
	TargetFile.write('\n// check ZCC compiler extensions\n')
	TargetFile.write('#if 9<=10000*__ZCC__+100*__ZCC_MINOR__+__ZCC_PATCHLEVEL__\n')

	TargetFile.write('// the type names should have no linkage in C, external linkage in C++\n')
	TargetFile.write('STATIC_ASSERT(TYPE_LINKAGE_CODE==__zcc_linkage(good_test),"good_test has incorrect linkage");\n')

	TargetFile.write('\n// check external linkage of variables\n')	
	for i in xrange(5):
		TargetFile.write('STATIC_ASSERT(2==__zcc_linkage(x'+str(i+1)+'),"x'+str(i+1)+' has incorrect linkage");\n')

	for i in xrange(6):
		TargetFile.write('STATIC_ASSERT(2==__zcc_linkage(x'+str(i+11)+'),"x'+str(i+11)+' has incorrect linkage");\n')

	for i in xrange(10):
		TargetFile.write('STATIC_ASSERT(2==__zcc_linkage(x'+str(i+23)+'),"x'+str(i+23)+' has incorrect linkage");\n')

	for i in xrange(28):
		TargetFile.write('STATIC_ASSERT(2==__zcc_linkage(x'+str(i+43)+'),"x'+str(i+43)+' has incorrect linkage");\n')

	TargetFile.write('\n// check internal linkage of variables\n')
	for i in xrange(5):
		TargetFile.write('STATIC_ASSERT(1==__zcc_linkage(x'+str(i+6)+'),"x'+str(i+6)+' has incorrect linkage");\n')

	for i in xrange(6):
		TargetFile.write('STATIC_ASSERT(1==__zcc_linkage(x'+str(i+17)+'),"x'+str(i+17)+' has incorrect linkage");\n')

	for i in xrange(10):
		TargetFile.write('STATIC_ASSERT(1==__zcc_linkage(x'+str(i+33)+'),"x'+str(i+33)+' has incorrect linkage");\n')

	for i in xrange(28):
		TargetFile.write('STATIC_ASSERT(1==__zcc_linkage(x'+str(i+71)+'),"x'+str(i+71)+' has incorrect linkage");\n')

	TargetFile.write('#endif\n')

	# no define-declares
	TargetFile.close()

def SpawnTestCase3(dest_file):
	TargetFile = open(dest_file,'w')
	TargetFile.write('// decl.C99/'+dest_file+'\n')

	for line in invariant_header_lines[5:]:
		TargetFile.write(line)
	TargetFile.write(context[dest_file])
	TargetFile.write(global_define[dest_file])

	# restart
	TargetFile.write('// this section checks that suppressing keyword works\n')
	TargetFile.write(section_comments[0].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(5):
		TargetFile.write(test_qualifiers[i].replace('THREAD_LOCAL','thread_local')+' '+var_decl_terse[dest_file](str(i+1))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[1].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(5):
		TargetFile.write(test_qualifiers[i+5].replace('THREAD_LOCAL','thread_local')+' '+var_decl_terse[dest_file](str(i+6))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[2].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(12):
		TargetFile.write(test_qualifiers[i+10].replace('THREAD_LOCAL','thread_local')+' '+var_decl_terse[dest_file](str(i+11))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[3].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(10):
		TargetFile.write(test_qualifiers[i+22].replace('THREAD_LOCAL','thread_local')+' '+var_decl_terse[dest_file](str(i+23))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[4].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(10):
		TargetFile.write(test_qualifiers[i+32].replace('THREAD_LOCAL','thread_local')+' '+var_decl_terse[dest_file](str(i+33))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[5].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(28):
		TargetFile.write(test_qualifiers[i+42].replace('THREAD_LOCAL','thread_local')+' '+var_decl_terse[dest_file](str(i+43))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[6].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(28):
		TargetFile.write(test_qualifiers[i+70].replace('THREAD_LOCAL','thread_local')+' '+var_decl_terse[dest_file](str(i+71))+';\n')
	TargetFile.write('\n')

	# need: normal decl, def_decl work in namespaces
	TargetFile.write('// check that things work properly in namespaces\nnamespace test {\n')

	TargetFile.write(global_define[dest_file])
	TargetFile.write(section_comments[0].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(5):
		TargetFile.write(test_qualifiers[i].replace('THREAD_LOCAL','thread_local')+' '+var_decl[dest_file](str(i+1))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[1].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(5):
		TargetFile.write(test_qualifiers[i+5].replace('THREAD_LOCAL','thread_local')+' '+var_decl[dest_file](str(i+6))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[2].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(12):
		TargetFile.write(test_qualifiers[i+10].replace('THREAD_LOCAL','thread_local')+' '+var_decl[dest_file](str(i+11))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[3].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(10):
		TargetFile.write(test_qualifiers[i+22].replace('THREAD_LOCAL','thread_local')+' '+var_decl[dest_file](str(i+23))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[4].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(10):
		TargetFile.write(test_qualifiers[i+32].replace('THREAD_LOCAL','thread_local')+' '+var_decl[dest_file](str(i+33))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[5].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(28):
		TargetFile.write(test_qualifiers[i+42].replace('THREAD_LOCAL','thread_local')+' '+var_decl[dest_file](str(i+43))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[6].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(28):
		TargetFile.write(test_qualifiers[i+70].replace('THREAD_LOCAL','thread_local')+' '+var_decl[dest_file](str(i+71))+';\n')
	TargetFile.write('\n')

	TargetFile.write('// define-declares\n')
	TargetFile.write(section_comments[0].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(5):
		TargetFile.write(test_qualifiers[i].replace('THREAD_LOCAL','thread_local')+' '+var_def[dest_file](str(i+1))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[1].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(5):
		TargetFile.write(test_qualifiers[i+5].replace('THREAD_LOCAL','thread_local')+' '+var_def[dest_file](str(i+6))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[2].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(12):
		TargetFile.write(test_qualifiers[i+10].replace('THREAD_LOCAL','thread_local')+' '+var_def[dest_file](str(i+11))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[3].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(10):
		TargetFile.write(test_qualifiers[i+22].replace('THREAD_LOCAL','thread_local')+' '+var_def[dest_file](str(i+23))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[4].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(10):
		TargetFile.write(test_qualifiers[i+32].replace('THREAD_LOCAL','thread_local')+' '+var_def[dest_file](str(i+33))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[5].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(28):
		TargetFile.write(test_qualifiers[i+42].replace('THREAD_LOCAL','thread_local')+' '+var_def[dest_file](str(i+43))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[6].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(28):
		TargetFile.write(test_qualifiers[i+70].replace('THREAD_LOCAL','thread_local')+' '+var_def[dest_file](str(i+71))+';\n')
	TargetFile.write('\n')

	# need: keyword suppression works in namespaces
	TargetFile.write('}	// end namespace test\n\n// check that keyword suppression works in namespaces\nnamespace test2 {\n')
	TargetFile.write(global_define[dest_file])

	TargetFile.write(section_comments[0].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(5):
		TargetFile.write(test_qualifiers[i].replace('THREAD_LOCAL','thread_local')+' '+var_decl_terse[dest_file](str(i+1))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[1].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(5):
		TargetFile.write(test_qualifiers[i+5].replace('THREAD_LOCAL','thread_local')+' '+var_decl_terse[dest_file](str(i+6))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[2].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(12):
		TargetFile.write(test_qualifiers[i+10].replace('THREAD_LOCAL','thread_local')+' '+var_decl_terse[dest_file](str(i+11))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[3].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(10):
		TargetFile.write(test_qualifiers[i+22].replace('THREAD_LOCAL','thread_local')+' '+var_decl_terse[dest_file](str(i+23))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[4].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(10):
		TargetFile.write(test_qualifiers[i+32].replace('THREAD_LOCAL','thread_local')+' '+var_decl_terse[dest_file](str(i+33))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[5].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(28):
		TargetFile.write(test_qualifiers[i+42].replace('THREAD_LOCAL','thread_local')+' '+var_decl_terse[dest_file](str(i+43))+';\n')
	TargetFile.write('\n')

	TargetFile.write(section_comments[6].replace('THREAD_LOCAL','thread_local'))
	for i in xrange(28):
		TargetFile.write(test_qualifiers[i+70].replace('THREAD_LOCAL','thread_local')+' '+var_decl_terse[dest_file](str(i+71))+';\n')
	TargetFile.write('\n')

	TargetFile.write('}	// end namespace test2\n')

#   this is only called for C++ test cases
#	check ZCC compiler extensions
	TargetFile.write('\n// check ZCC compiler extensions\n')
	TargetFile.write('#if 9<=10000*__ZCC__+100*__ZCC_MINOR__+__ZCC_PATCHLEVEL__\n')

	TargetFile.write('// the type names should have external linkage in C++\n')
	TargetFile.write('static_assert(2==__zcc_linkage(good_test),"good_test has incorrect linkage");\n')
	TargetFile.write('static_assert(2==__zcc_linkage(::good_test),"good_test has incorrect linkage");\n')
	TargetFile.write('static_assert(2==__zcc_linkage(test::good_test),"good_test has incorrect linkage");\n')
	TargetFile.write('static_assert(2==__zcc_linkage(::test::good_test),"good_test has incorrect linkage");\n')
	TargetFile.write('static_assert(2==__zcc_linkage(test2::good_test),"good_test has incorrect linkage");\n')
	TargetFile.write('static_assert(2==__zcc_linkage(::test2::good_test),"good_test has incorrect linkage");\n')
	for i in xrange(5+5+12+10+10+28+18):
		TargetFile.write('static_assert(2==__zcc_linkage(test::good_test'+str(i+1)+'),"good_test'+str(i+1)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::test::good_test'+str(i+1)+'),"good_test'+str(i+1)+' has incorrect linkage");\n')

	TargetFile.write('\n// check external linkage of variables\n')	
	for i in xrange(5):
		TargetFile.write('static_assert(2==__zcc_linkage(x'+str(i+1)+'),"x'+str(i+1)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::x'+str(i+1)+'),"x'+str(i+1)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(test::x'+str(i+1)+'),"x'+str(i+1)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(test::x_'+str(i+1)+'),"x_'+str(i+1)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::test::x'+str(i+1)+'),"x'+str(i+1)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::test::x_'+str(i+1)+'),"x_'+str(i+1)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(test2::x'+str(i+1)+'),"x'+str(i+1)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::test2::x'+str(i+1)+'),"x'+str(i+1)+' has incorrect linkage");\n')

	for i in xrange(6):
		TargetFile.write('static_assert(2==__zcc_linkage(x'+str(i+11)+'),"x'+str(i+11)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::x'+str(i+11)+'),"x'+str(i+11)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(test::x'+str(i+11)+'),"x'+str(i+11)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(test::x_'+str(i+11)+'),"x_'+str(i+11)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::test::x'+str(i+11)+'),"x'+str(i+11)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::test::x_'+str(i+11)+'),"x_'+str(i+11)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(test2::x'+str(i+11)+'),"x'+str(i+11)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::test2::x'+str(i+11)+'),"x'+str(i+11)+' has incorrect linkage");\n')

	for i in xrange(10):
		TargetFile.write('static_assert(2==__zcc_linkage(x'+str(i+23)+'),"x'+str(i+23)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::x'+str(i+23)+'),"x'+str(i+23)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(test::x'+str(i+23)+'),"x'+str(i+23)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(test::x_'+str(i+23)+'),"x_'+str(i+23)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::test::x'+str(i+23)+'),"x'+str(i+23)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::test::x_'+str(i+23)+'),"x_'+str(i+23)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(test2::x'+str(i+23)+'),"x'+str(i+23)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::test2::x'+str(i+23)+'),"x'+str(i+23)+' has incorrect linkage");\n')

	for i in xrange(28):
		TargetFile.write('static_assert(2==__zcc_linkage(x'+str(i+43)+'),"x'+str(i+43)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::x'+str(i+43)+'),"x'+str(i+43)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(test::x'+str(i+43)+'),"x'+str(i+43)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(test::x_'+str(i+43)+'),"x_'+str(i+43)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::test::x'+str(i+43)+'),"x'+str(i+43)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::test::x_'+str(i+43)+'),"x_'+str(i+43)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(test2::x'+str(i+43)+'),"x'+str(i+43)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(2==__zcc_linkage(::test2::x'+str(i+43)+'),"x'+str(i+43)+' has incorrect linkage");\n')

	TargetFile.write('\n// check internal linkage of variables\n')
	for i in xrange(5):
		TargetFile.write('static_assert(1==__zcc_linkage(x'+str(i+6)+'),"x'+str(i+6)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(::x'+str(i+6)+'),"x'+str(i+6)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(test::x'+str(i+6)+'),"x'+str(i+6)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(test::x_'+str(i+6)+'),"x_'+str(i+6)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(::test::x'+str(i+6)+'),"x'+str(i+6)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(::test::x_'+str(i+6)+'),"x_'+str(i+6)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(test2::x'+str(i+6)+'),"x'+str(i+6)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(::test2::x'+str(i+6)+'),"x'+str(i+6)+' has incorrect linkage");\n')

	for i in xrange(6):
		TargetFile.write('static_assert(1==__zcc_linkage(x'+str(i+17)+'),"x'+str(i+17)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(::x'+str(i+17)+'),"x'+str(i+17)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(test::x'+str(i+17)+'),"x'+str(i+17)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(test::x_'+str(i+17)+'),"x_'+str(i+17)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(::test::x'+str(i+17)+'),"x'+str(i+17)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(::test::x_'+str(i+17)+'),"x_'+str(i+17)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(test2::x'+str(i+17)+'),"x'+str(i+17)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(::test2::x'+str(i+17)+'),"x'+str(i+17)+' has incorrect linkage");\n')

	for i in xrange(10):
		TargetFile.write('static_assert(1==__zcc_linkage(x'+str(i+33)+'),"x'+str(i+33)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(::x'+str(i+33)+'),"x'+str(i+33)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(test::x'+str(i+33)+'),"x'+str(i+33)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(test::x_'+str(i+33)+'),"x_'+str(i+33)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(::test::x'+str(i+33)+'),"x'+str(i+33)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(::test::x_'+str(i+33)+'),"x_'+str(i+33)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(test2::x'+str(i+33)+'),"x'+str(i+33)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(::test2::x'+str(i+33)+'),"x'+str(i+33)+' has incorrect linkage");\n')

	for i in xrange(28):
		TargetFile.write('static_assert(1==__zcc_linkage(x'+str(i+71)+'),"x'+str(i+71)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(::x'+str(i+71)+'),"x'+str(i+71)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(test::x'+str(i+71)+'),"x'+str(i+71)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(test::x_'+str(i+71)+'),"x_'+str(i+71)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(::test::x'+str(i+71)+'),"x'+str(i+71)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(::test::x_'+str(i+71)+'),"x_'+str(i+71)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(test2::x'+str(i+71)+'),"x'+str(i+71)+' has incorrect linkage");\n')
		TargetFile.write('static_assert(1==__zcc_linkage(::test2::x'+str(i+71)+'),"x'+str(i+71)+' has incorrect linkage");\n')

	TargetFile.write('#endif\n')

	TargetFile.close()

if __name__ == '__main__':
	for filename in target_files:
		SpawnTestCase(filename)
	for filename in target_files2:
		SpawnTestCase2(filename)
	for filename in target_files3:
		SpawnTestCase3(filename)

