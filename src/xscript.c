#include <pthread.h>

#include "./common/common.h"
#include "./tokenizer/lex.h"
#include "./parser/ast.h"
#include "./parser/scope.h"
#include "./common/bytecode.h"
#include "./interpreter/vm.h"
#include "./compiler/compiler.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s [source.x]\n", argv[0]);
        return 1;
    }

    wbt_init();

    lgx_lex_init();

    lgx_ast_t ast;
    lgx_ast_init(&ast);
    lgx_ast_parser(&ast, argv[1]);
    if (ast.err_no) {
        printf("%.*s\n", ast.err_len, ast.err_info);
        return 1;
    }

    lgx_ast_print(ast.root, 0);

    //printf("%lld\n", execute(ast.root));

    lgx_bc_t bc;
    lgx_bc_compile(&ast, &bc);
    if (bc.err_no) {
        printf("%.*s\n", bc.err_len, bc.err_info);
        return 1;
    }

    lgx_bc_print(bc.bc, bc.bc_top);

    lgx_hash_print(bc.constant);
    printf("\n\n");

    lgx_vm_t vm;
    lgx_vm_init(&vm, &bc);
//    lgx_vm_init(&vm, opcode, sizeof(opcode)/sizeof(unsigned));

    lgx_vm_start(&vm);
    //printf("\nprogram exit with return value: ");
    //lgx_val_print(&vm.regs[1]);
    //printf("\n");

    lgx_vm_cleanup(&vm);

    return 0;
}