#include <assert.h>
#include "escape.h"

static int is_hex_char(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static unsigned int hex_to_dec(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else {
        // error
        return 0;
    }
}

int lgx_escape_decode(lgx_str_t* src, lgx_str_t* dst) {
    assert(dst->size >= src->length);

    int i = 0, j = 0, len = src->length;
    for (i = 0; i < len; ++i) {
        if (src->buffer[i] == '\\') {
            ++ i;
            switch (src->buffer[i]) {
                case 'r':  dst->buffer[j++] = '\r'; break;
                case 'n':  dst->buffer[j++] = '\n'; break;
                case 't':  dst->buffer[j++] = '\t'; break;
                case '\\': dst->buffer[j++] = '\\'; break;
                case '"':  dst->buffer[j++] = '\"'; break;
                case '\'': dst->buffer[j++] = '\''; break;
                case '0':  dst->buffer[j++] = '\0'; break;
                case 'x': case 'X':
                    if ((len - i > 2) && is_hex_char(src->buffer[i+1]) && is_hex_char(src->buffer[i+2]) ) {
                        dst->buffer[j++] = (hex_to_dec(src->buffer[i+1]) << 4) | hex_to_dec(src->buffer[i+2]);
                        i += 2;
                    } else {
                        dst->buffer[j++] = src->buffer[i];
                    }
                    break;
                default:
                    dst->buffer[j++] = '\\';
                    dst->buffer[j++] = src->buffer[i];
            }
        } else {
            dst->buffer[j++] = src->buffer[i];
        }
    }
    dst->length = j;

    return 0;
}