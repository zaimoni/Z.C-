// decl.C99\Error_union_forward_def_const_volatile3.hpp
// using singly defined union
// (C)2010 Kenneth Boyd, license: MIT.txt

const union good_test volatile;
union good_test;

union good_test {
	int x_factor;
};

