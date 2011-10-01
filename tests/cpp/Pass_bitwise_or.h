// tests/cpp/Pass_bitwise_or.h
// checks that | works in control expressions
// (C)2009 Kenneth Boyd, license: MIT.txt

// 2-ary
#if 1 | 1
#else
#error 1 | 1 is false
#endif

#if 1 | 0
#else
#error 1 | 0 is false
#endif

#if 0 | 1
#else
#error 0 | 1 is false
#endif

#if 0 | 0
#error 0 | 0 is true
#else
#endif

// check implicit left-right associativity
#if 1 | 1 | 1
#else
#error 1 | 1 | 1 is false
#endif

#if 1 | 1 | 0
#else
#error 1 | 1 | 0 is false
#endif

#if 1 | 0 | 1
#else
#error 1 | 0 | 1 is false
#endif

#if 1 | 0 | 0
#else
#error 1 | 0 | 0 is false
#endif

#if 0 | 1 | 1
#else
#error 0 | 1 | 1 is false
#endif

#if 0 | 1 | 0
#else
#error 0 | 1 | 0 is false
#endif

#if 0 | 0 | 1
#else
#error 0 | 0 | 1 is false
#endif

#if 0 | 0 | 0
#error 0 | 0 | 0 is true
#else
#endif

// spot-check character literals as well
#if 'A' | 'A'
#else
#error 'A' | 'A' is false
#endif

#if 'A' | '\0'
#else
#error 'A' | '\0' is false
#endif

#if '\0' | 'A'
#else
#error '\0' | 'A' is false
#endif

#if '\0' | '\0'
#error '\0' | '\0' is true
#else
#endif

// spot-check some signed bitwise or; be careful not to generate -0 on one's-complement machines
#if -3 | -2
#else
#error -3 | -2 is false
#endif
