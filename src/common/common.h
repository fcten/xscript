#ifndef LGX_COMMON_H
#define LGX_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdarg.h>
#include <string.h>
#include <memory.h>

#if defined(__GNUC__)
#define EXPECTED(x)	(__builtin_expect(((x) != 0), 1))
#define UNEXPECTED(x)	(__builtin_expect(((x) != 0), 0))
#else
#define EXPECTED(x)	(x)
#define UNEXPECTED(x)	(x)
#endif

#endif // LGX_COMMON_H