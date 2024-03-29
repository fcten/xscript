/*
#include "./tokenizer/lex.h"
#include "./parser/ast.h"
#include "./compiler/compiler.h"
#include "./compiler/bytecode.h"
#include "./compiler/constant.h"
#include "./interpreter/vm.h"
*/

#include "xscript.hpp"
#include "framework/command.hpp"

using xscript::framework::command;
using xscript::tokenizer::scanner;
using xscript::parser::syntax;
using xscript::parser::ast;

int main(int argc, char* argv[]) {

    command::instance().init(argc, argv);

    /*
    lgx_token_init();

    lgx_ast_t ast;
    int ast_ok = lgx_ast_init(&ast, argv[1]);
    lgx_ast_print(&ast);
    lgx_ast_print_error(&ast);

    if (ast_ok == 0) {
        lgx_compiler_t c;
        lgx_compiler_init(&c);
        int c_ok = lgx_compiler_generate(&c, &ast);

        lgx_bc_print(c.bc.buffer, c.bc.length);
        lgx_ast_print_error(&ast);
        lgx_const_print(&c.constant);
        printf("\n\n");

        if (c_ok == 0) {
            lgx_vm_t vm;
            lgx_vm_init(&vm, &c);

            // 寻找 main 函数
            lgx_str_t mainfunc = lgx_str("main");
            lgx_symbol_t* symbol = lgx_symbol_get(ast.root, &mainfunc, -1);

            if (symbol) {
                lgx_function_t* fun  = symbol->v.v.fun;
                lgx_vm_call(&vm, fun);
            } else {
                // TODO
                // can't find main func
            }

            lgx_vm_cleanup(&vm);
        }

        lgx_compiler_cleanup(&c);
    }

    lgx_ast_cleanup(&ast);

    lgx_token_cleanup();
    */

    int ret = 0;
    syntax s;
    
    if (command::instance().get_source_files().size() == 0) {
        // TODO 交互式终端模式
    }

    // 
    for (auto source : command::instance().get_source_files()) {
        if (!s.load(source)) {
            ret = 1;
        }
    }
    
    return ret;
}