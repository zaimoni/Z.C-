// decl.C99\Error_struct_forward_def_const_volatile3.hpp
// using singly defined struct
// (C)2010 Kenneth Boyd, license: MIT.txt

const struct good_test volatile;
struct good_test;

struct good_test {
	int x_factor;
};

