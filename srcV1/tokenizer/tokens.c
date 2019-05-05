#include "tokens.h"

const lgx_token_str_t reserved_words[LGX_RESERVED_WORDS] = {
    {TK_VAR, "var"},
    {TK_CONST, "const"},

    {TK_AUTO, "auto"},
    {TK_INT, "int"},
    {TK_FLOAT, "float"},
    {TK_BOOL, "bool"},
    {TK_STRING, "string"},
    {TK_ARRAY, "array"},

    {TK_TYPEOF, "typeof"},

    {TK_TRUE, "true"},
    {TK_FALSE, "false"},
    {TK_NULL, "null"},

    {TK_FUNCTION, "function"},
    {TK_RETURN, "return"},

    {TK_STRUCT, "struct"},
    {TK_INTERFACE, "interface"},

    {TK_IF, "if"},
    {TK_ELSE, "else"},
    {TK_SWITCH, "switch"},
    {TK_CASE, "case"},
    {TK_DEFAULT, "default"},

    {TK_FOR, "for"},
    {TK_DO, "do"},
    {TK_WHILE, "while"},

    {TK_BREAK, "break"},
    {TK_CONTINUE, "continue"},

    {TK_TRY, "try"},
    {TK_CATCH, "catch"},
    {TK_THROW, "throw"},

    {TK_CO, "co"},

    {TK_IMPORT, "import"},
    {TK_EXPORT, "export"}
};