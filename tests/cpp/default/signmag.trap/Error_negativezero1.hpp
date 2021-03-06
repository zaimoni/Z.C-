// default\signmag.trap\Error_negativezero1.hpp
// (C)2009 Kenneth Boyd, license: MIT.txt

#include <stdint.h>

// non-trapping machines accept bitwise -0
// C99 6.2.6.2 defines the two bit patterns for 0 as comparing equal
// | : impractical (have to start with bitwise -0)
// ~ : use INT_MAX
// ^ : ^ any positive integer with its negative (randdriver)
// & : (randdriver)

// promotion to intmax_t (C99 6.10.1p3) skews things
#if ~INTMAX_MAX
#endif

