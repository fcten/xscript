#include <stdio.h>
#include <stdlib.h>

#include "lex.h"
#include "ast.h"
#include "lgx.h"
#include "bytecode.h"

int lgx_ctx_init(lgx_ctx_t *ctx) {
    ctx->stack_size = 1024;
    ctx->stack_top = 0;
    ctx->stack = malloc(ctx->stack_size * sizeof(lgx_val_t *));
    return 0;
}

int read_file(const char* file, char** p) {
    FILE* fp;
    fp = fopen(file, "rb");  // localfile文件名

    fseek(fp, 0L, SEEK_END);  /* 定位到文件末尾 */
    int flen = ftell(fp);     /* 得到文件大小 */
    *p = (char*)malloc(flen); /* 根据文件大小动态分配内存空间 */
    if (*p == NULL) {
        fclose(fp);
        return 0;
    }
    fseek(fp, 0L, SEEK_SET);       /* 定位到文件开头 */
    return fread(*p, 1, flen, fp); /* 一次性读取全部文件内容 */
}

int main(int argc, char* argv[]) {
    lgx_lex_init();

    lgx_ast_t ast = { {"test", sizeof("test") - 1, 0, 0, 1, 0}, 0, 0};
    ast.lex.length = read_file("./test/test1.x", &ast.lex.source);

    lgx_ast_parser(&ast);
    
    lgx_ast_print(ast.root, 0);

    lgx_bc_gen(ast.root);

    lgx_bc_print();

    return 0;
}