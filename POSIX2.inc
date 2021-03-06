# maintainer-mode build rules for derived files
# PYTHON variable does require configuration when moved to another machine
# remember to use 'make all' when these rules are in effect 
PYTHON = c:\Python27\python
SED = sed
SELECTIVE_CUT = tools\selective_cut.py
SUPPRESS_CUT = tools\suppress_cut.py

# source code
# touch causes unnecessary build noise here
#CPreproc_pp.hpp: CPreproc.hpp
#	$(PYTHON) $(SELECTIVE_CUT) cpp CPreproc.hpp CPreproc_pp.hpp.alt 
#	if cmp -s CPreproc_pp.hpp CPreproc_pp.hpp.alt; then rm CPreproc_pp.hpp.alt ; else mv CPreproc_pp.hpp.alt CPreproc_pp.hpp; fi

#CPreproc_pp.cpp: CPreproc.cpp.in
#	$(PYTHON) $(SELECTIVE_CUT) cpp CPreproc.cpp.in CPreproc_pp.cpp.alt2 
#	$(PYTHON) $(SUPPRESS_CUT) nocpp CPreproc_pp.cpp.alt2 CPreproc_pp.cpp.alt 
#	if cmp -s CPreproc_pp.cpp CPreproc_pp.cpp.alt; then rm CPreproc_pp.cpp.alt ; else mv CPreproc_pp.cpp.alt CPreproc_pp.cpp; fi
#	rm CPreproc_pp.cpp.alt2

#CPreproc.cpp: CPreproc.cpp.in
#	$(PYTHON) $(SUPPRESS_CUT) cpp CPreproc.cpp.in CPreproc.cpp.alt2 
#	$(PYTHON) $(SELECTIVE_CUT) nocpp CPreproc.cpp.alt2 CPreproc.cpp.alt 
#	if cmp -s CPreproc.cpp CPreproc.cpp.alt; then rm CPreproc.cpp.alt ; else mv CPreproc.cpp.alt CPreproc.cpp; fi
#	rm CPreproc.cpp.alt2
	
#CPreproc_autogen_pp.cpp: CPreproc_autogen.cpp
#	$(PYTHON) $(SELECTIVE_CUT) cpp CPreproc_autogen.cpp CPreproc_autogen_pp.cpp.alt 
#	if cmp -s CPreproc_autogen_pp.cpp CPreproc_autogen_pp.cpp.alt; then rm CPreproc_autogen_pp.cpp.alt ; else mv CPreproc_autogen_pp.cpp.alt CPreproc_autogen_pp.cpp; fi

#CSupport_pp.hpp: CSupport.hpp
#	$(PYTHON) $(SELECTIVE_CUT) cpp CSupport.hpp CSupport_pp.hpp.alt 
#	if cmp -s CSupport_pp.hpp CSupport_pp.hpp.alt; then rm CSupport_pp.hpp.alt; else mv CSupport_pp.hpp.alt CSupport_pp.hpp; fi

#CSupport_pp.cpp: CSupport.cpp.in
#	$(PYTHON) $(SELECTIVE_CUT) cpp CSupport.cpp.in CSupport_pp.cpp.alt2 
#	$(PYTHON) $(SUPPRESS_CUT) nocpp CSupport_pp.cpp.alt2 CSupport_pp.cpp.alt 
#	if cmp -s CSupport_pp.cpp CSupport_pp.cpp.alt; then rm CSupport_pp.cpp.alt ; else mv CSupport_pp.cpp.alt CSupport_pp.cpp; fi
#	rm CSupport_pp.cpp.alt2

#CSupport.cpp: CSupport.cpp.in
#	$(PYTHON) $(SUPPRESS_CUT) cpp CSupport.cpp.in CSupport.cpp.alt2 
#	$(PYTHON) $(SELECTIVE_CUT) nocpp CSupport.cpp.alt2 CSupport.cpp.alt 
#	if cmp -s CSupport.cpp CSupport.cpp.alt; then rm CSupport.cpp.alt ; else mv CSupport.cpp.alt CSupport.cpp; fi
#	rm CSupport.cpp.alt2

#type_spec_pp.cpp: type_spec.cpp
#	$(PYTHON) $(SELECTIVE_CUT) cpp type_spec.cpp type_spec_pp.cpp.alt 
#	if cmp -s type_spec_pp.cpp type_spec_pp.cpp.alt; then rm type_spec_pp.cpp.alt ; else mv type_spec_pp.cpp.alt type_spec_pp.cpp; fi
	
#type_system_pp.hpp: type_system.hpp.in
#	$(PYTHON) $(SELECTIVE_CUT) cpp type_system.hpp.in type_system_pp.hpp.alt2 
#	$(PYTHON) $(SUPPRESS_CUT) nocpp type_system_pp.hpp.alt2 type_system_pp.hpp.alt 
#	if cmp -s type_system_pp.hpp type_system_pp.hpp.alt; then rm type_system_pp.hpp.alt; else mv type_system_pp.hpp.alt type_system_pp.hpp; fi
#	rm type_system_pp.hpp.alt2

#type_system.hpp: type_system.hpp.in
#	$(PYTHON) $(SUPPRESS_CUT) cpp type_system.hpp.in type_system.hpp.alt2 
#	$(PYTHON) $(SELECTIVE_CUT) nocpp type_system.hpp.alt2 type_system.hpp.alt 
#	if cmp -s type_system.hpp type_system.hpp.alt; then rm type_system.hpp.alt ; else mv type_system.hpp.alt type_system.hpp; fi
#	rm type_system.hpp.alt2

#type_system_pp.cpp: type_system.cpp.in
#	$(PYTHON) $(SELECTIVE_CUT) cpp type_system.cpp.in type_system_pp.cpp.alt2 
#	$(PYTHON) $(SUPPRESS_CUT) nocpp type_system_pp.cpp.alt2 type_system_pp.cpp.alt 
#	if cmp -s type_system_pp.cpp type_system_pp.cpp.alt; then rm type_system_pp.cpp.alt ; else mv type_system_pp.cpp.alt type_system_pp.cpp; fi
#	rm type_system_pp.cpp.alt2

#type_system.cpp: type_system.cpp.in
#	$(PYTHON) $(SUPPRESS_CUT) cpp type_system.cpp.in type_system.cpp.alt2 
#	$(PYTHON) $(SELECTIVE_CUT) nocpp type_system.cpp.alt2 type_system.cpp.alt 
#	if cmp -s type_system.cpp type_system.cpp.alt; then rm type_system.cpp.alt ; else mv type_system.cpp.alt type_system.cpp; fi
#	rm type_system.cpp.alt2

# test case support
regenerate_tests: tests/zcc/decl.C99/Pass_class_def.hpp \
	tests/zcc/decl.C99/Pass_class_def_decl.hpp \
	tests/zcc/decl.C99/Pass_class_def2.hpp

tests/zcc/decl.C99/Pass_class_def.hpp: tests/zcc/decl.C99/Pass_struct_def.hpp
	$(SED) -e s/struct/class/ tests/zcc/decl.C99/Pass_struct_def.hpp > tests/zcc/decl.C99/Pass_class_def.hpp.alt 
	if cmp -s tests/zcc/decl.C99/Pass_class_def.hpp tests/zcc/decl.C99/Pass_class_def.hpp.alt; then rm tests/zcc/decl.C99/Pass_class_def.hpp.alt ; else mv tests/zcc/decl.C99/Pass_class_def.hpp.alt tests/zcc/decl.C99/Pass_class_def.hpp; fi

tests/zcc/decl.C99/Pass_class_def_decl.hpp: tests/zcc/decl.C99/Pass_struct_def_decl.hpp
	$(SED) -e s/struct/class/ tests/zcc/decl.C99/Pass_struct_def_decl.hpp > tests/zcc/decl.C99/Pass_class_def_decl.hpp.alt 
	if cmp -s tests/zcc/decl.C99/Pass_class_def_decl.hpp tests/zcc/decl.C99/Pass_class_def_decl.hpp.alt; then rm tests/zcc/decl.C99/Pass_class_def_decl.hpp.alt ; else mv tests/zcc/decl.C99/Pass_class_def_decl.hpp.alt tests/zcc/decl.C99/Pass_class_def_decl.hpp; fi

tests/zcc/decl.C99/Pass_class_def2.hpp: tests/zcc/decl.C99/Pass_struct_def2.hpp
	$(SED) -e s/struct/class/ tests/zcc/decl.C99/Pass_struct_def2.hpp > tests/zcc/decl.C99/Pass_class_def2.hpp.alt 
	if cmp -s tests/zcc/decl.C99/Pass_class_def2.hpp tests/zcc/decl.C99/Pass_class_def2.hpp.alt; then rm tests/zcc/decl.C99/Pass_class_def2.hpp.alt ; else mv tests/zcc/decl.C99/Pass_class_def2.hpp.alt tests/zcc/decl.C99/Pass_class_def2.hpp; fi

# unit testing
unit_tests: z_float.exe

z_float.exe: make_Zaimoni_STL z_float.cpp z_float.hpp
	$(CXX) -oz_float.exe $(LINK_FLAGS) -DTEST_APP z_float.cpp -lz_log_adapter -lz_stdio_log -lz_format_util
	z_float.exe
