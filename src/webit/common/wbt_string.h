/* 
 * File:   wbt_string.h
 * Author: Fcten
 *
 * Created on 2014年8月18日, 下午3:50
 */

#ifndef __WBT_STRING_H__
#define __WBT_STRING_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "../webit.h"

typedef struct {
    /* 使用 int 而不是 unsigned int，方便进行运算和比较 */
    int len;
    char * str;
} wbt_str_t;

typedef struct {
    int len;
    int start;
} wbt_str_offset_t;

typedef union {
    wbt_str_t s;
    wbt_str_offset_t o;
} wbt_str_union_t;

#define wbt_string(str)     { sizeof(str) - 1, (char *) str }
#define wbt_null_string     { 0, NULL }

#define wbt_str_set(stri, text)  (stri).len = sizeof(text) - 1; (stri).str = (char *) text
#define wbt_str_set_null(stri)   (stri).len = 0; (stri).str = NULL

#define wbt_offset_to_str(offset, stri, p)   do { \
    (stri).str = (char *)(p) + (offset).start; \
    (stri).len = (offset).len; \
} while(0);

#define wbt_variable_to_str(vari, stri)   do { \
    (stri).str = (char *)(&vari); \
    (stri).len = sizeof(vari); \
} while(0);

const char * wbt_stdstr(wbt_str_t * str);

int wbt_strpos( wbt_str_t *str1, wbt_str_t *str2 );
int wbt_stripos( wbt_str_t *str1, wbt_str_t *str2 );
int wbt_strncmp( wbt_str_t *str1, wbt_str_t *str2, int len );
int wbt_strnicmp( wbt_str_t *str1, wbt_str_t *str2, int len );
int wbt_strcmp( wbt_str_t *str1, wbt_str_t *str2);
int wbt_stricmp( wbt_str_t *str1, wbt_str_t *str2);

void wbt_strcat( wbt_str_t * dest, wbt_str_t * src, int max_len );

static wbt_inline unsigned int wbt_strlen(const char *s) {
    return strlen(s);
}

int wbt_atoi(wbt_str_t * str);

unsigned long long int wbt_str_to_ull(wbt_str_t * str, int base);

#ifdef	__cplusplus
}
#endif

#endif /* __WBT_STRING_H__ */
