#ifndef LGX_LEX_H
#define LGX_LEX_H

enum {
    TK_ID = 256,
    TK_NUMBER,  // 257
    TK_STRING,  // 258
    TK_EOF,     // 259
    TK_EOL,     // 260
    TK_SPACE,   // 261
    TK_EQ,      // 262
    TK_NE,      // 263
    TK_GE,      // 264
    TK_LE,      // 265
    TK_AND,     // 266
    TK_OR,      // 267
    TK_SHL,     // 268
    TK_SHR,     // 269
    TK_COMMENT, // 270
    TK_AUTO,    // 271
    TK_FOR,     // 272
    TK_DO,      // 273
    TK_WHILE,   // 274
    TK_BREAK,   // 275
    TK_CONTINUE,// 276
    TK_IF,      // 277
    TK_ELSE,    // 278
    TK_SWITCH,  // 279
    TK_CASE,    // 280
    TK_DEFAULT, // 281
    TK_FUNCTION,// 282
    TK_RETURN,  // 283
    TK_UNKNOWN = 1024
};

typedef struct {
    char *source;
    int length;
    int offset;
    int milestone;
    int line;
    int row;
} lgx_lex_t;

int lgx_lex_init();
int lgx_lex(lgx_lex_t *ctx);

#endif // LGX_LEX_H