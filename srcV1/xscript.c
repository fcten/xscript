#include "./tokenizer/lex.h"
#include "./parser/ast.h"

int main(int argc, char* argv[]) {
    lgx_token_init();

    lgx_ast_t ast;
    if (lgx_ast_init(&ast, argv[1])) {
        lgx_ast_print_error(&ast);
        return 1;
    }

    lgx_ast_print(&ast);

    lgx_ast_cleanup(&ast);

    lgx_token_cleanup();
    return 0;
}