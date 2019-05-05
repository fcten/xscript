#ifndef LGX_TOKENS_H
#define LGX_TOKENS_H

typedef enum lgx_token_e {
    TK_UNKNOWN = 0, // 非法

    TK_EOF, // 文件结束
    TK_EOL, // 行末

    TK_COMMENT, // 注释
    TK_SPACE,   // 空格 & 制表符

    TK_IDENTIFIER, // 标识符

    TK_LONG,   // 整数
    TK_DOUBLE, // 浮点数
    TK_CHAR,   // 单个字符
    TK_STRING, // 字符串常量

    // 算术运算符
	TK_ADD, // +
	TK_SUB, // -
	TK_MUL, // *
	TK_DIV, // /
	TK_MOD, // %

    // 位运算符
	TK_AND, // &
	TK_OR,  // |
    TK_NOT, // ~
	TK_XOR, // ^
	TK_SHL, // <<
	TK_SHR, // >>

    // 赋值运算符
    TK_ASSIGN,     // =
	TK_ADD_ASSIGN, // +=
	TK_SUB_ASSIGN, // -=
	TK_MUL_ASSIGN, // *=
	TK_DIV_ASSIGN, // /=
	TK_MOD_ASSIGN, // %=

	TK_AND_ASSIGN, // &=
	TK_OR_ASSIGN,  // |=
    TK_NOT_ASSIGN, // ~=
	TK_XOR_ASSIGN, // ^=
	TK_SHL_ASSIGN, // <<=
	TK_SHR_ASSIGN, // >>=

    // 逻辑运算符
    TK_LOGIC_AND, // &&
    TK_LOGIC_OR,  // ||
    TK_LOGIC_NOT, // !

    // 自增 & 自减运算符
	TK_INC, // ++
	TK_DEC, // --

    // 比较运算符
    TK_EQUAL,         // ==
    TK_NOT_EQUAL,     // !=
    TK_GREATER,       // >
    TK_LESS,          // <
    TK_GREATER_EQUAL, // >=
    TK_LESS_EQUAL,    // <=

    // 括号
	TK_LEFT_PAREN,  // (
    TK_LEFT_BRACK,  // [
    TK_LEFT_BRACE,  // {
    TK_RIGHT_PAREN, // )
    TK_RIGHT_BRACK, // ]
    TK_RIGHT_BRACE, // }

    // 其它运算符
	TK_COMMA,     // ,
	TK_DOT,       // .
    TK_COLON,     // :
	TK_SEMICOLON, // ;
    TK_ARROW,     // ->
    
    // 关键字 & 保留字
    TK_VAR,
    TK_CONST,

    TK_AUTO,
    TK_INT,
    TK_FLOAT,
    TK_BOOL,
    TK_STRING,
    TK_ARRAY,

    TK_TYPEOF,

    TK_TRUE,
    TK_FALSE,
    TK_NULL,

    TK_FUNCTION,
    TK_RETURN,

    TK_STRUCT,
    TK_INTERFACE,

    TK_IF,
    TK_ELSE,
    TK_SWITCH,
    TK_CASE,
    TK_DEFAULT,

    TK_FOR,
    TK_DO,
    TK_WHILE,

    TK_BREAK,
    TK_CONTINUE,

    TK_TRY,
    TK_CATCH,
    TK_THROW,

    TK_CO,

    TK_IMPORT,
    TK_EXPORT,
    TK_PACKAGE,

    TK_LENGTH
} lgx_token_t;

typedef struct {
    lgx_token_t token;
    char* s;
} lgx_token_str_t;

#define LGX_RESERVED_WORDS  (TK_LENGTH - TK_VAR)

#endif // LGX_TOKENS_H