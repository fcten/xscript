#include "../common/common.h"
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
    {TK_ARR, "array"},
    // 类型判断
    {TK_TYPEOF, "typeof"},
    {TK_INSTANCEOF, "instanceof"},
    // 特殊值
    {TK_UNDEFINED, "undefined"},
    {TK_TRUE, "true"},  // 布尔
    {TK_FALSE, "false"},// 布尔
    {TK_NULL, "null"},  // 引用
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
    {TK_ABSTRACT, "abstract"},
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
    // 异步
    {TK_ASYNC, "async"},
    {TK_AWAIT, "await"},
    // 模块
    {TK_IMPORT, "import"},
    {TK_EXPORT, "export"},
    {TK_PACKAGE, "package"},
    // IO (测试使用，可能会移除)
    {TK_ECHO, "echo"},

    {TK_CONST, "const"},
    {TK_STATIC, "static"},
    
    {TK_NAMESPACE, "namespace"},
    
    {TK_USE, "use"},
    {TK_AS, "as"}};