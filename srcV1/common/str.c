#include "common.h"
#include "str.h"

int lgx_str_init(lgx_str_t* str, unsigned size) {
    memset(str, 0, sizeof(lgx_str_t));

    str->buffer = xmalloc(size);
    if (!str->buffer) {
        return 1;
    }

    str->size = size;

    return 0;
}

void lgx_str_cleanup(lgx_str_t* str) {
    if (str->buffer) {
        xfree(str->buffer);
    }

    memset(str, 0, sizeof(lgx_str_t));
}

// 相等返回 0
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

static int str_resize(lgx_str_t *str, unsigned size) {
    char *buffer = xrealloc(str->buffer, size);
    if (!buffer) {
        return 1;
    }

    str->buffer = buffer;
    str->size = size;

    return 0;
}

int lgx_str_dup(lgx_str_t *src, lgx_str_t *dst) {
    assert(src->length && src->buffer);

    if (dst->length < src->length) {
        if (str_resize(dst, src->length)) {
            return 1;
        }
    }

    memcpy(dst->buffer, src->buffer, src->length);
    dst->length = src->length;

    return 0;
}