#include "common.h"
#include "str.h"

/* 初始化字符串 str
 * 分配长度为 size 的可用内存空间，这些内存未经初始化
 */
int lgx_str_init(lgx_str_t* str, unsigned size) {
    memset(str, 0, sizeof(lgx_str_t));

    if (size > 0) {
        str->buffer = xmalloc(size);
        if (!str->buffer) {
            return 1;
        }

        str->size = size;
    }

    return 0;
}

/* 释放为 str 分配的内存
 */
void lgx_str_cleanup(lgx_str_t* str) {
    if (str->size && str->buffer) {
        xfree(str->buffer);
    }

    memset(str, 0, sizeof(lgx_str_t));
}

/* 比较字符串 str1 和 str2
 * 如果 str1 = str2，返回 0。
 * 如果 str1 > str2，返回正数。
 * 如果 str1 < str2，返回负数。
 */
int lgx_str_cmp(lgx_str_t *str1, lgx_str_t *str2) {
    int len = str1->length;
    if (str1->length > str2->length) {
        len = str2->length;
    }
    
    int i;
    for (i = 0; i < len; i++) {
        if (str1->buffer[i] != str2->buffer[i]) {
            return str1->buffer[i] - str2->buffer[i];
        }
    }
    
    if (str1->length == str2->length) {
        return 0;
    } else if (str1->length == len) {
        return -str2->buffer[i];
    } else {
        return str1->buffer[i];
    }
}

/* 调整字符串 str 的内存空间长度至 size
 */
int lgx_str_resize(lgx_str_t *str, unsigned size) {
    char *buffer = xrealloc(str->buffer, size);
    if (!buffer) {
        return 1;
    }

    str->buffer = buffer;
    str->size = size;
    if (str->length > str->size) {
        str->length = str->size;
    }

    return 0;
}

/* 复制字符串 src 到 dst 中
 * 调用时必须保证 dst 的内存空间大于等于 src 的长度
 */
void lgx_str_dup(lgx_str_t *src, lgx_str_t *dst) {
    assert(src->length > 0 && src->buffer);
    assert(dst->size > 0 && dst->buffer);
    assert(dst->size >= src->length);

    memcpy(dst->buffer, src->buffer, src->length);
    dst->length = src->length;
}

/* 拼接字符串 src 到 dst 的尾部
 * 调用时必须保证 dst 的内存空间大于等于 src 的长度加 dst 的长度
 */
void lgx_str_concat(lgx_str_t *src, lgx_str_t *dst) {
    assert(src->length > 0 && src->buffer);
    assert(dst->size > 0 && dst->buffer);
    assert(dst->size >= dst->length + src->length);

    memcpy(dst->buffer + dst->length, src->buffer, src->length);
    dst->length += src->length;
}

void lgx_str_print(lgx_str_t *str) {
    printf("%.*s", str->length, str->buffer);
}