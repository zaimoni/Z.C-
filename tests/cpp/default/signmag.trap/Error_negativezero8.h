// default\signmag.trap\Error_negativezero8.h
// (C)2009 Kenneth Boyd, license: MIT.txt

#include <limits.h>

// non-trapping machines accept bitwise -0
// C99 6.2.6.2 defines the two bit patterns for 0 as comparing equal
// | : impractical (have to start with bitwise -0)
// ~ : use INT_MAX
// ^ : ^ any positive integer with its negative (randdriver)
// & : (randdriver)

// spot-check &
#if LONG_MIN+1 & -1
#endif



