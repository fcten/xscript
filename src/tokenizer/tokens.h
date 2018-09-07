#ifndef LGX_TOKENS_H
#define LGX_TOKENS_H

enum {
    TK_ID = 256,// 标识符
    TK_LONG,    // 整数
    TK_DOUBLE,  // 浮点数
    TK_STRING,  // 字符串常量
    TK_EOF,     // 文件结束标志
    TK_EOL,     // 行末标志
    TK_SPACE,   // 空格 & 制表符
    TK_EQ,      // ==
    TK_NE,      // !=
    TK_GE,      // >=
    TK_LE,      // <=
    TK_AND,     // &&
    TK_OR,      // ||
    TK_SHL,     // <<
    TK_SHR,     // >>
    TK_COMMENT, // 注释
    TK_ATTR,    // 对象属性访问运算符
    TK_INDEX,   // 数组下标访问运算符
    TK_CALL,    // 函数调用运算符
    TK_ARRAY,   // 数组常量
    TK_OBJECT,  // 对象常量
    TK_PTR,     // -> 指针运算符
    TK_ASSIGN_ADD, // +=
    TK_ASSIGN_SUB, // -=
    TK_ASSIGN_MUL, // *=
    TK_ASSIGN_DIV, // /=
    TK_ASSIGN_AND, // &=
    TK_ASSIGN_OR,  // |=
    TK_ASSIGN_SHL, // <<=
    TK_ASSIGN_SHR, // >>=
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

    TK_UNDEFINED,
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