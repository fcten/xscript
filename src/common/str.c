#include "common.h"
#include "str.h"
#include "val.h"

lgx_str_t* lgx_str_new(char *str, unsigned len) {
    unsigned head_size = sizeof(lgx_str_t);
    unsigned data_size = len * sizeof(char);

    lgx_str_t* ret = xmalloc(head_size);
    if (!ret) {
        return NULL;
    }

    ret->buffer = xmalloc(data_size);
    if (!ret->buffer) {
        xfree(ret);
        return NULL;
    }

    ret->gc.ref_cnt = 0;
    ret->gc.size = head_size + data_size;
    ret->gc.type = T_STRING;

    ret->is_ref = 0;
    ret->size = len;
    ret->length = len;
    memcpy(ret->buffer, str, len);
    
    return ret;
}

lgx_str_t* lgx_str_new_ref(char *str, unsigned len) {
    unsigned size = sizeof(lgx_str_t);

    lgx_str_t* ret = xmalloc(size);
    if (!ret) {
        return NULL;
    }

    ret->buffer = str;
    
    ret->gc.ref_cnt = 0;
    ret->gc.size = size;
    ret->gc.type = T_STRING;

    ret->is_ref = 1;
    ret->size = len;
    ret->length = len;
    
    return ret;
}

void lgx_str_delete(lgx_str_t *str) {
    if (!str->is_ref) {
        xfree(str->buffer);
    }
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

lgx_str_t* lgx_str_copy(lgx_str_t *str){
    if (str->is_ref) {
        return lgx_str_new_ref(str->buffer, str->length);
    } else {
        return lgx_str_new(str->buffer, str->length);
    }
}

int lgx_str_concat(lgx_str_t *str1, lgx_str_t *str2) {
    // 引用类型转换为非引用类型
    if (str1->is_ref) {
        char *new_buf = xmalloc(str1->length + str2->length);
        if (!new_buf) {
            return 1;
        }
        memcpy(new_buf, str1->buffer, str1->length);
        str1->buffer = new_buf;
        str1->size = str1->length + str2->length;
        str1->is_ref = 0;
    }

    // 如果空间不足，则扩容
    if (str1->size < str1->length + str2->length) {
        char *new_buf = xrealloc(str1->buffer, str1->size * 2);
        if (!new_buf) {
            return 1;
        }
        str1->buffer = new_buf;
        str1->size *= 2;
    }

    memcpy(str1->buffer + str1->length, str2->buffer, str2->length);
    str1->length += str2->length;

    return 0;
}