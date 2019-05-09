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