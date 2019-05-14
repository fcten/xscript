#include "./tokenizer/lex.h"
#include "./parser/ast.h"
#include "./compiler/compiler.h"
#include "./compiler/bytecode.h"

int main(int argc, char* argv[]) {
    lgx_token_init();

    lgx_ast_t ast;
    int ast_ok = lgx_ast_init(&ast, argv[1]);
    lgx_ast_print(&ast);
    lgx_ast_print_error(&ast);

    if (ast_ok == 0) {
        lgx_compiler_t c;
        lgx_compiler_init(&c);
        lgx_compiler_generate(&c, &ast);

        lgx_bc_print(c.bc.buffer, c.bc.length);

        lgx_compiler_cleanup(&c);
    }

    lgx_ast_cleanup(&ast);

    lgx_token_cleanup();
    return 0;
}