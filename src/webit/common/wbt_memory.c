/* 
 * File:   wbt_memory.c
 * Author: Fcten
 *
 * Created on 2014年8月20日, 下午3:50
 */

#include "../webit.h"
#include "wbt_error.h"
#include "wbt_memory.h"

#if defined(USE_TCMALLOC)
#define malloc(size) tc_malloc(size)
#define calloc(count,size) tc_calloc(count,size)
#define realloc(ptr,size) tc_realloc(ptr,size)
#define free(ptr) tc_free(ptr)
#elif defined(USE_JEMALLOC)
#define malloc(size) je_malloc(size)
#define calloc(count,size) je_calloc(count,size)
#define realloc(ptr,size) je_realloc(ptr,size)
#define free(ptr) je_free(ptr)
#endif

static size_t wbt_memory_usage = 0;

size_t wbt_mem_usage() {
    return wbt_memory_usage;
}

void * wbt_malloc(size_t size) {
    void * ptr = malloc(size);

    if(!ptr) {
        wbt_error( "Out of memory trying to allocate " JL_SIZE_T_SPECIFIER " bytes\n", size );
    } else {
        wbt_memory_usage += wbt_malloc_size(ptr);
    }
    
    return ptr;
}

void * wbt_calloc(size_t size) {
    void * ptr = calloc(1, size);

    if(!ptr) {
        wbt_error( "Out of memory trying to allocate " JL_SIZE_T_SPECIFIER " bytes\n", size );
    } else {
        wbt_memory_usage += wbt_malloc_size(ptr);
    }
    
    return ptr;
}

void * wbt_realloc(void *ptr, size_t size) {
    if( !ptr ) {
        return wbt_malloc(size);
    }
    
    if( !size ) {
        wbt_free(ptr);
        return NULL;
    }
    
    size_t oldsize = wbt_malloc_size(ptr);

    void * newptr = realloc(ptr, size);
    
    if( !newptr ) {
        wbt_error( "Out of memory trying to allocate " JL_SIZE_T_SPECIFIER " bytes\n", size );
    } else {
        wbt_memory_usage -= oldsize;
        wbt_memory_usage += wbt_malloc_size(newptr);
    }
    
    return newptr;
}

void wbt_free(void *ptr) {
    if ( !ptr ) {
        return;
    }
    
    wbt_memory_usage -= wbt_malloc_size(ptr);
    free(ptr);
}

void * wbt_memset(void *ptr, int ch, size_t size) {
    return memset(ptr, ch, size);
}

void * wbt_memcpy(void *dest, const void *src, size_t size) {
    return memcpy(dest, src, size);
}

void *wbt_memmove( void* dest, const void* src, size_t count ) {
    return memmove(dest, src, count);
}

void * wbt_strdup(const void *ptr, size_t size) {
    void * p = wbt_malloc(size);

    if( p ) {
        wbt_memcpy(p,ptr,size);
    }

    return p;
}