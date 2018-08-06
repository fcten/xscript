#include "common.h"
#include "str.h"

lgx_str_t* lgx_str_new(char *str, unsigned len) {
    lgx_str_t* ret = xmalloc(sizeof(lgx_str_t) + len * sizeof(char));
    if (!ret) {
        return NULL;
    }

    ret->buffer = ret->str;
    
    ret->size = len;
    ret->length = len;
    memcpy(ret->buffer, str, len);
    
    return ret;
}

void lgx_str_delete(lgx_str_t *str) {
    xfree(str);
}

int lgx_str_cmp(lgx_str_t *str1, lgx_str_t *str2) {
    if (str1->length != str2->length) {
        return 1;
    }
    
    int i;
    for (i = 0; i < str1->length; i++) {
        if (str1->buffer[i] != str2->buffer[i]) {
            return 1;
        }
    }
    
    return 0;
}