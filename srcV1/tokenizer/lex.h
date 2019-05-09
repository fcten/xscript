#ifndef LGX_LEX_H
#define LGX_LEX_H

#include "source.h"
#include "token.h"

typedef struct lgx_lex_s {
    // 当前解析的源文件
    lgx_source_t source;

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

int lgx_lex_init(lgx_lex_t *ctx, char* path);
int lgx_lex_cleanup(lgx_lex_t *ctx);

lgx_token_t lgx_lex_next(lgx_lex_t *ctx);

#endif // LGX_LEX_H