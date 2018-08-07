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

enum {
    // 基本类型
    T_UNDEFINED = 0,// 变量尚未定义
    T_LONG,         // 64 位有符号整数
    T_DOUBLE,       // 64 位有符号浮点数
    T_BOOL,         // T_BOOL 必须是基本类型的最后一种，因为它被用于是否为基本类型的判断
    // 高级类型
    T_REDERENCE,
    T_STRING,
    T_ARRAY,
    T_OBJECT,
    T_FUNCTION,
    T_RESOURCE
};

#endif // LGX_COMMON_H