#include "token.hpp"

namespace xscript::tokenizer {

std::unordered_map<std::string_view, token_t> tokens = {

    // TK_UNKNOWN

    // TK_EOF

    {"\r\n",        TK_EOL},
    {"\n",          TK_EOL},

    {"//",          TK_COMMENT_LINE},
    {"/*",          TK_COMMENT_BEGIN},
    {"*/",          TK_COMMENT_END},
    
    {" ",           TK_SPACE},
    {"\t",          TK_TAB},

    // TK_IDENTIFIER

    // TK_LITERAL_LONG
    // TK_LITERAL_DOUBLE
    // TK_LITERAL_CHAR
    // TK_LITERAL_STRING

    {"+",          TK_ADD},
    {"-",          TK_SUB},
    {"*",          TK_MUL},
    {"/",          TK_DIV},
    {"%",          TK_MOD},

    {"&",          TK_BITWISE_AND},
    {"|",          TK_BITWISE_OR},
    {"~",          TK_BITWISE_NOT},
    {"^",          TK_BITWISE_XOR},
    {"<<",         TK_BITWISE_SHL},
    {">>",         TK_BITWISE_SHR},

    {"=",          TK_ASSIGN},
    {"+=",         TK_ASSIGN_ADD},
    {"-=",         TK_ASSIGN_SUB},
    {"*=",         TK_ASSIGN_MUL},
    {"/=",         TK_ASSIGN_DIV},
    {"%=",         TK_ASSIGN_MOD},
    {"&=",         TK_ASSIGN_AND},
    {"|=",         TK_ASSIGN_OR},
    {"^=",         TK_ASSIGN_XOR},
    {"<<=",        TK_ASSIGN_SHL},
    {">>=",        TK_ASSIGN_SHR},

    {"&&",         TK_LOGIC_AND},
    {"||",         TK_LOGIC_OR},
    {"!",          TK_LOGIC_NOT},

    {"++",         TK_INC},
    {"--",         TK_DEC},

    {"==",         TK_EQUAL},
    {"!=",         TK_NOT_EQUAL},
    {">",          TK_GREATER},
    {"<",          TK_LESS},
    {">=",         TK_GREATER_EQUAL},
    {"<=",         TK_LESS_EQUAL},

    {"(",          TK_LEFT_PAREN},
    {")",          TK_RIGHT_PAREN},
    {"[",          TK_LEFT_BRACK},
    {"]",          TK_RIGHT_BRACK},
    {"{",          TK_LEFT_BRACE},
    {"}",          TK_RIGHT_BRACE},

    {",",          TK_COMMA},
    {".",          TK_DOT},
    {":",          TK_COLON},
    {";",          TK_SEMICOLON},
    {"->",         TK_RIGHT_ARROW},
    {"<-",         TK_LEFT_ARROW},
    {"...",        TK_ELLIPSIS}
};

std::unordered_map<std::string_view, token_t> reserved_words = {

    {"var",         TK_VAR},
    {"const",       TK_CONST},

    {"int",         TK_INT},
    {"float",       TK_FLOAT},
    {"bool",        TK_BOOL},
    {"string",      TK_STRING},
    {"struct",      TK_STRUCT},
    {"interface",   TK_INTERFACE},
    {"func",        TK_FUNCTION},

    {"type",        TK_TYPE},

    {"true",        TK_TRUE},
    {"false",       TK_FALSE},
    {"null",        TK_NULL},

    {"return",      TK_RETURN},

    {"if",          TK_IF},
    {"else",        TK_ELSE},
    {"switch",      TK_SWITCH},
    {"case",        TK_CASE},
    {"default",     TK_DEFAULT},

    {"for",         TK_FOR},
    {"do",          TK_DO},
    {"while",       TK_WHILE},

    {"break",       TK_BREAK},
    {"continue",    TK_CONTINUE},

    {"try",         TK_TRY},
    {"catch",       TK_CATCH},
    {"throw",       TK_THROW},

    {"co",          TK_CO},

    {"import",      TK_IMPORT},
    {"export",      TK_EXPORT},
    {"package",     TK_PACKAGE},

    {"echo",        TK_ECHO}
};

token::token() :
    type(TK_UNKNOWN)
{

}

token::token(token_t t) :
    type(t)
{

}

token::token(token_t t, std::string_view v) :
    type(t),
    literal(v)
{

}

}
