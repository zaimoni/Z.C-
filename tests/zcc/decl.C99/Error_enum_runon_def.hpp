// decl.C99\Error_enum_runon_def.hpp
// using singly defined enum
// (C)2011 Kenneth Boyd, license: MIT.txt

enum bad_test {
	x_factor = 1
};

// ringing the changes on extern
extern enum bad_test x1
extern const enum bad_test x2;
