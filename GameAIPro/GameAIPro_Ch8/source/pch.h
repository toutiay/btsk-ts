#ifndef __GLOBAL__
#define __GLOBAL__

#ifndef __MATH__
	#include "math.h"
#endif

// make sure we have 'NULL'
#if !defined(NULL)
	#define NULL (0)
#endif

// compile-time assert
#ifndef compileTimeAssert
	#define compileTimeAssert(test) typedef int __compileTimeTest##__LINE__[(test) ? 1 : -1]
#endif

// number of elements in an array
// this is a simple implementation of this macro, as this example only uses it on
// actual arrays, not pointers or anything else
// a proper implementation would guard against such shenanigans
#define TR_COUNTOF(ary) ((int)(sizeof(ary) / (sizeof(ary[0]))))

// standard library headers
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cmath>
#include <malloc.h>

// errors
#ifndef error
	#define error(...) { printf( __VA_ARGS__ ); assert(false); }
#endif

#endif // __GLOBAL__
