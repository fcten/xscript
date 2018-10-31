/* 
 * File:   wbt_memory.h
 * Author: Fcten
 *
 * Created on 2014年8月20日, 下午3:50
 */

#ifndef __WBT_MEMORY_H__
#define	__WBT_MEMORY_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>

#if defined(USE_TCMALLOC)
#include <google/tcmalloc.h>
#if (TC_VERSION_MAJOR == 1 && TC_VERSION_MINOR >= 6) || (TC_VERSION_MAJOR > 1)
#define wbt_malloc_size(p) tc_malloc_size(p)
#else
#error "Newer version of tcmalloc required"
#endif

#elif defined(USE_JEMALLOC)
#include <jemalloc/jemalloc.h>
#if (JEMALLOC_VERSION_MAJOR == 2 && JEMALLOC_VERSION_MINOR >= 1) || (JEMALLOC_VERSION_MAJOR > 2)
#define wbt_malloc_size(p) je_malloc_usable_size(p)
#else
#error "Newer version of jemalloc required"
#endif

#else
#include <malloc.h>
#ifndef WIN32
#define wbt_malloc_size(p) malloc_usable_size(p)
#else
#define wbt_malloc_size(p) _msize(p)
#endif
#endif

void * wbt_malloc(size_t size);
void * wbt_calloc(size_t size);
void * wbt_realloc(void *ptr, size_t size);
void wbt_free(void *ptr);

void * wbt_memset(void *ptr, int ch, size_t size);
void * wbt_memcpy(void *dest, const void *src, size_t size);
void * wbt_memmove(void* dest, const void* src, size_t count);
void * wbt_strdup(const void *ptr, size_t size);

int wbt_is_oom();
size_t wbt_mem_usage();

#ifdef	__cplusplus
}
#endif

#endif	/* __WBT_MEMORY_H__ */

