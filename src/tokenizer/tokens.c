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
    // 变量
    {TK_AUTO, "auto"},
    {TK_INT, "int"},
    {TK_FLOAT, "float"},
    {TK_BOOL, "bool"},
    {TK_STR, "string"},
    {TK_ARRAY, "array"},
    {TK_OBJECT, "object"},
    // 特殊值
    {TK_TRUE, "true"},
    {TK_FALSE, "false"},
    {TK_NULL, "null"},
    // 函数 & 方法
    {TK_FUNCTION, "function"},
    {TK_RETURN, "return"},
    // 类
    {TK_THIS, "this"},
    {TK_NEW, "new"},
    {TK_DELETE, "delete"},
    {TK_CLASS, "class"},
    {TK_EXTENDS, "extends"},
    {TK_PUBLIC, "public"},
    {TK_PRIVATE, "private"},
    {TK_PROTECTED, "protected"},
    {TK_FINAL, "final"},
    {TK_INTERFACE, "interface"},
    {TK_IMPLEMENTS, "implements"},
    // 流程控制
    {TK_IF, "if"},
    {TK_ELSE, "else"},
    {TK_SWITCH, "switch"},
    {TK_CASE, "case"},
    {TK_DEFAULT, "default"},
    // 循环
    {TK_FOR, "for"},
    {TK_DO, "do"},
    {TK_WHILE, "while"},
    {TK_FOREACH, "foreach"},
    // 控制转移
    {TK_BREAK, "break"},
    {TK_CONTINUE, "continue"},
    // 异常
    {TK_TRY, "try"},
    {TK_CATCH, "catch"},
    {TK_FINALLY, "finally"},
    {TK_THROW, "throw"},
    // 模块
    {TK_IMPORT, "import"},
    {TK_EXPORT, "export"},

    {TK_CONST, "const"},
    {TK_STATIC, "static"},
    
    {TK_NAMESPACE, "namespace"},
    
    {TK_USE, "use"},
    {TK_AS, "as"}};