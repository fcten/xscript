#ifndef LGX_LEX_H
#define LGX_LEX_H

#include "token.h"

typedef struct {
    // 文件名
    char *file;
    // 文件内容
    char *source;
    // 文件长度
    int length;
    // 当前偏移
    int offset;
    // 当前 token 起始偏移
    int milestone;
    // 当前行号
    int line;
    // 当前行起始偏移
    int line_start;
    // 当前列
    int row;
} lgx_lex_t;

int lgx_lex(lgx_lex_t *ctx);

#endif // LGX_LEX_H