#ifndef _XSCRIPT_PARSER_VALUE_HPP_
#define _XSCRIPT_PARSER_VALUE_HPP_

#include "type.hpp"

namespace xscript::parser {

class object;

typedef union {
    long long l;
    double d;
    object *obj;
} value_t;

class value {
public:
    // 类型
    v_type_t type;

    value_t value;

};

class object {
public:
    std::string& _tostring();
    std::string& _call();
    std::string& _set();
    std::string& _get(); 
};

}

#endif // _XSCRIPT_PARSER_VALUE_HPP_