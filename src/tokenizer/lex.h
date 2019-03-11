#ifndef LGX_LEX_H
#define LGX_LEX_H

#include "tokens.h"

typedef struct {
    char *file;
    char *source;
    int length;
    int offset;
    int milestone;
    int line;
    int line_start;
    int row;
} lgx_lex_t;

int lgx_lex_init();
int lgx_lex_cleanup();

int lgx_lex(lgx_lex_t *ctx);

#endif // LGX_LEX_H