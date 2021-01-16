/*
#include "./tokenizer/lex.h"
#include "./parser/ast.h"
#include "./compiler/compiler.h"
#include "./compiler/bytecode.h"
#include "./compiler/constant.h"
#include "./interpreter/vm.h"
*/

#include "xscript.hpp"

using xscript::tokenizer::scanner;

int main(int argc, char* argv[]) {
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

    if (argc < 2) {
        return 1;
    }

    scanner s(argv[1]);
    s.print();
    
    return 0;
}