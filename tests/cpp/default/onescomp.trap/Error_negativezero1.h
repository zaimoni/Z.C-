// default\onescomp.trap\Error_negativezero1.h
// (C)2009 Kenneth Boyd, license: MIT.txt

#include <limits.h>

// one's-complement: -0 is trap representation

// trapping machines die on generating -0
// C99 6.2.6.2 defines the two bit patterns for 0 as comparing equal
// & : impractical (have to start with bitwise -0)
// ~ : use 0
// | : try INT_MAX | INT_MIN (randdriver)
// ^ : try INT_MAX ^ INT_MIN (randdriver)
// use preprocessor intmax_t promotion (C99 6.10.1p3)
#if ~0
#endif

