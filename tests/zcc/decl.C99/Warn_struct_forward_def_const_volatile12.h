// decl.C99\Warn_struct_forward_def_const_volatile12.h
// using singly defined struct
// (C)2010 Kenneth Boyd, license: MIT.txt

struct good_test;
struct good_test volatile const;

struct good_test {
	int x_factor;
};

