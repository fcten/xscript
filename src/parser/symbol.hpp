#ifndef _XSCRIPT_PARSER_SYMBOL_HPP_
#define _XSCRIPT_PARSER_SYMBOL_HPP_

#include <string>
#include "value.hpp"

namespace xscript::parser {

class symbol {
public:
    // 符号类型
    std::string_view name;

    // 符号的值
    value v;

    // 保存符号的编号（常量编号，全局变量编号，局部变量寄存器编号）
    unsigned r;

    // 是否为全局符号
    bool is_global;

    // 是否被赋值
    bool is_initialized;

    // 是否被使用
    bool is_used;
};

}

#endif // _XSCRIPT_PARSER_SYMBOL_HPP_