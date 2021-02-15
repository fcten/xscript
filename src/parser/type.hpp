#ifndef _XSCRIPT_PARSER_TYPE_HPP_
#define _XSCRIPT_PARSER_TYPE_HPP_

#include <string>

namespace xscript::parser {

typedef enum {
    T_UNKNOWN = 0,  // 未知类型
    T_NULL,
    T_LONG,         // 64 位有符号整数
    T_DOUBLE,       // 64 位有符号浮点数
    T_BOOL,
    T_OBJECT
} v_type_t;

}

#endif // _XSCRIPT_PARSER_TYPE_HPP_