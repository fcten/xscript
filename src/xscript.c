#include <pthread.h>

#include "./common/common.h"
#include "./tokenizer/lex.h"
#include "./parser/ast.h"
#include "./parser/scope.h"
#include "./common/bytecode.h"
#include "./interpreter/vm.h"
#include "./compiler/compiler.h"

int read_file(const char* file, char** p) {
    FILE* fp;
    fp = fopen(file, "rb");  // localfile文件名

    fseek(fp, 0L, SEEK_END);  /* 定位到文件末尾 */
    int flen = ftell(fp);     /* 得到文件大小 */
    *p = (char*)xmalloc(flen); /* 根据文件大小动态分配内存空间 */
    if (*p == NULL) {
        fclose(fp);
        return 0;
    }
    fseek(fp, 0L, SEEK_SET);       /* 定位到文件开头 */
    return fread(*p, 1, flen, fp); /* 一次性读取全部文件内容 */
}

int main(int argc, char* argv[]) {
    wbt_init();

    lgx_lex_init();

    lgx_ast_t ast;
    lgx_ast_init(&ast);
    ast.lex.length = read_file(argv[1], &ast.lex.source);
    //ast.lex.length = read_file("./test.x", &ast.lex.source);

    lgx_ast_parser(&ast);
    if (ast.err_no) {
        printf("%.*s\n", ast.err_len, ast.err_info);
        return 1;
    }

    //lgx_ast_print(ast.root, 0);

    //printf("%lld\n", execute(ast.root));

    lgx_bc_t bc;
    lgx_bc_compile(&ast, &bc);
    if (bc.err_no) {
        printf("%.*s\n", bc.err_len, bc.err_info);
        return 1;
    }

    //lgx_bc_print(bc.bc, bc.bc_top);

    //lgx_hash_print(bc.constant);
    //printf("\n");

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