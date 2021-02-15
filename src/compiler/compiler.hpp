#ifndef _XSCRIPT_COMPILER_COMPILER_HPP_
#define _XSCRIPT_COMPILER_COMPILER_HPP_

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <initializer_list>
#include "opcode.hpp"
#include "../parser/value.hpp"
#include "../parser/ast.hpp"

namespace xscript::compiler {

class compiler {
private:
    parser::ast ast;

    // 字节码
    opcode opcodes;

    // 符号表

    // 常量表
    std::unordered_map<std::string, parser::value> constants;

    // 全局变量


    std::vector<std::string> errors;

    bool append_compile_error(std::string f, int l, std::initializer_list<std::string_view> args);

    bool compile_root(std::unique_ptr<parser::ast_node>& node);
    bool compile_function_declaration(std::unique_ptr<parser::ast_node>& node);
    bool compile_global_variable_declaration(std::unique_ptr<parser::ast_node>& node);
    bool compile_local_variable_declaration(std::unique_ptr<parser::ast_node>& node);
    bool compile_constant_declaration(std::unique_ptr<parser::ast_node>& node);

public:
    compiler(std::string s);

    bool compile();

};

}

#endif // _XSCRIPT_COMPILER_COMPILER_HPP_