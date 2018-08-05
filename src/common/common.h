#ifndef LGX_COMMON_H
#define LGX_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdarg.h>
#include <string.h>
#include <memory.h>

//#include <jemalloc/jemalloc.h>

#define lgx_inline inline

#define xmalloc malloc
#define xcalloc calloc
#define xrealloc realloc
#define xfree free

#if defined(__GNUC__)
#define EXPECTED(x)	(__builtin_expect(((x) != 0), 1))
#define UNEXPECTED(x)	(__builtin_expect(((x) != 0), 0))
#else
#define EXPECTED(x)	(x)
#define UNEXPECTED(x)	(x)
#endif

#if defined(__GNUC__)
#define ALIGN(x) ((1ULL << 32) >> __builtin_clz(x - 1))
#elif defined(WIN32)
static lgx_inline unsigned ALIGN(unsigned x) {
    unsigned long index;
	if (BitScanReverse(&index, x - 1)) {
		return (1ULL << 32) >> index;
	} else {
		return x;
    }
}
#else
static lgx_inline unsigned ALIGN(unsigned x) {
	x -= 1;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return x + 1;
}
#endif

#endif // LGX_COMMON_H