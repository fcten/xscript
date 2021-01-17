#ifndef _XSCRIPT_TOKENIZER_TOKEN_HPP_
#define _XSCRIPT_TOKENIZER_TOKEN_HPP_

#include <string>
#include <unordered_map>

namespace xscript::tokenizer {

typedef enum {
    TK_UNKNOWN = 0, // 非法

    TK_EOF, // 文件结束
    TK_EOL, // 行末

    TK_COMMENT_LINE,  // 单行注释
    TK_COMMENT_BEGIN, // 多行注释
    TK_COMMENT_END,   // 多行注释
    
    TK_SPACE,   // 空格
    TK_TAB,     // 制表符

    TK_IDENTIFIER, // 标识符

    TK_LITERAL_LONG,   // 字面量：整数
    TK_LITERAL_DOUBLE, // 字面量：浮点数
    TK_LITERAL_CHAR,   // 字面量：字符
    TK_LITERAL_STRING, // 字面量：字符串

    // 算术运算符
	TK_ADD, // +
	TK_SUB, // -
	TK_MUL, // *
	TK_DIV, // /
	TK_MOD, // %

    // 位运算符
	TK_BITWISE_AND, // &
	TK_BITWISE_OR,  // |
    TK_BITWISE_NOT, // ~
	TK_BITWISE_XOR, // ^
	TK_BITWISE_SHL, // <<
	TK_BITWISE_SHR, // >>

    // 赋值运算符
    TK_ASSIGN,     // =
	TK_ASSIGN_ADD, // +=
	TK_ASSIGN_SUB, // -=
	TK_ASSIGN_MUL, // *=
	TK_ASSIGN_DIV, // /=
	TK_ASSIGN_MOD, // %=

	TK_ASSIGN_AND, // &=
	TK_ASSIGN_OR,  // |=
	TK_ASSIGN_XOR, // ^=
	TK_ASSIGN_SHL, // <<=
	TK_ASSIGN_SHR, // >>=

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
    TK_RIGHT_PAREN, // )
    TK_LEFT_BRACK,  // [
    TK_RIGHT_BRACK, // ]
    TK_LEFT_BRACE,  // {
    TK_RIGHT_BRACE, // }

    // 其它运算符
	TK_COMMA,       // ,
	TK_DOT,         // .
    TK_COLON,       // :
	TK_SEMICOLON,   // ;
    TK_RIGHT_ARROW, // ->
    TK_LEFT_ARROW,  // <-
    TK_ELLIPSIS,    // ...
    
    // 关键字 & 保留字
    TK_VAR,
    TK_CONST,

    TK_INT,
    TK_FLOAT,
    TK_BOOL,
    TK_STRING,
    TK_STRUCT,
    TK_INTERFACE,
    TK_FUNCTION,

    TK_TYPE,

    TK_TRUE,
    TK_FALSE,
    TK_NULL,

    TK_RETURN,

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

    TK_AS,
    TK_ECHO,

    TK_LENGTH
} token_t;
class token {
public:
    token_t type;
    std::string_view literal;

    token();
    token(token_t t);
    token(token_t t, std::string_view v);

    bool operator ==(token_t t) {
        return t == type;
    }

    bool operator !=(token_t t) {
        return t != type;
    }
};

}

#endif // _XSCRIPT_TOKENIZER_TOKEN_HPP_