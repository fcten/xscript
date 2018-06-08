#ifndef LGX_TOKENS_H
#define LGX_TOKENS_H

enum {
    TK_ID = 256,// 256 标识符
    TK_NUMBER,  // 257 立即数
    TK_STRING,  // 258 字符串常量
    TK_EOF,     // 259 文件结束标志
    TK_EOL,     // 260 行末标志
    TK_SPACE,   // 261 空格 & 制表符
    TK_EQ,      // 262 ==
    TK_NE,      // 263 !=
    TK_GE,      // 264 >=
    TK_LE,      // 265 <=
    TK_AND,     // 266 &&
    TK_OR,      // 267 ||
    TK_SHL,     // 268 <<
    TK_SHR,     // 269 >>
    TK_COMMENT, // 270 代码注释
    TK_CALL,    // 271 函数调用
    TK_ATTR,    // 272 对象属性访问
    TK_INDEX,   // 273 数组下标访问
    TK_ARRAY,   // 274 数组常量
    TK_OBJECT,  // 275 对象常量
    // 以下为关键字 & 保留字
    TK_AUTO,
    TK_INT,
    TK_FLOAT,
    TK_BOOL,
    TK_STR,
    TK_ARR,
    TK_OBJ,

    TK_TYPEOF,
    TK_INSTANCEOF,

    TK_TRUE,
    TK_FALSE,
    TK_NULL,

    TK_FUNCTION,
    TK_RETURN,

    TK_THIS,
    TK_NEW,
    TK_DELETE,
    TK_CLASS,
    TK_EXTENDS,
    TK_PUBLIC,
    TK_PRIVATE,
    TK_PROTECTED,
    TK_FINAL,
    TK_INTERFACE,
    TK_IMPLEMENTS,

    TK_IF,
    TK_ELSE,
    TK_SWITCH,
    TK_CASE,
    TK_DEFAULT,

    TK_FOR,
    TK_DO,
    TK_WHILE,
    TK_FOREACH,

    TK_BREAK,
    TK_CONTINUE,

    TK_TRY,
    TK_CATCH,
    TK_FINALLY,
    TK_THROW,

    TK_IMPORT,
    TK_EXPORT,

    TK_ECHO,

    TK_CONST,
    TK_STATIC,

    TK_NAMESPACE,
    
    TK_USE,
    TK_AS,

    TK_UNKNOWN = 1024
};

typedef struct {
    int token;
    char* s;
} lgx_token_t;

#define LGX_RESERVED_WORDS  (TK_AS - TK_AUTO + 1) 

#endif // LGX_TOKENS_H