#include "tokens.h"

/*
string
number - float, integer
bool
function
array
object
null
resource

true, false

class
*/

// 关键字 && 保留字
const lgx_token_t lgx_reserved_words[LGX_RESERVED_WORDS] = {
    {TK_AUTO, "auto"},    {TK_FOR, "for"},         {TK_DO, "do"},
    {TK_WHILE, "while"},  {TK_BREAK, "break"},     {TK_CONTINUE, "continue"},
    {TK_IF, "if"},        {TK_ELSE, "else"},       {TK_SWITCH, "switch"},
    {TK_CASE, "case"},    {TK_DEFAULT, "default"}, {TK_FUNCTION, "function"},
    {TK_RETURN, "return"}};