#include <stdio.h>
#include <stdlib.h>

#include "lex.h"
#include "ast.h"
#include "lgx.h"
#include "bytecode.h"

int lgx_ctx_init(lgx_ctx_t *ctx) {
    ctx->stack_size = 1024;
    ctx->stack_top = 0;
    ctx->stack = malloc(ctx->stack_size * sizeof(lgx_val_t*));

    ctx->pc = 0;

    return 0;
}

extern unsigned short buf[10240];
extern unsigned offset;

void lgx_push(lgx_ctx_t *ctx, lgx_val_t *v) {
    ctx->stack[ctx->stack_top++] = *v;
}

lgx_val_t* lgx_pop(lgx_ctx_t *ctx) {
    return &ctx->stack[ctx->stack_top--];
}

void lgx_vardump(lgx_val_t* v) {
    switch (v->type) {
        case T_UNDEFINED:
            printf("undefined\n");
            break;
        case T_UNINITIALIZED:
            printf("uninitialized\n");
            break;
        case T_LONG:
            printf("%lld\n", v->value.l);
            break;
        case T_DOUBLE:
            printf("%f\n", v->value.d);
            break;
        case T_STRING:
            printf("\"string\"\n");
            break;
        case T_ARRAY:
            printf("[array]\n");
            break;
        case T_OBJECT:
            printf("{object}\n");
            break;
        case T_RESOURCE:
            printf("<resource>\n");
            break;
        case T_REDERENCE:
            lgx_vardump(v->value.ref);
            break;
        case T_FUNCTION:
            printf("function\n");
            break;
        default:
            printf("error\n");
    }
}

void lgx_execute(lgx_ctx_t *ctx) {
    unsigned short op, a, b, c;
    while(ctx->pc < offset) {
        op = buf[ctx->pc] >> 10;
        a = (buf[ctx->pc] & 0b0000001111100000) >> 5;
        b = buf[ctx->pc] & 0b0000000000011111;
        c = buf[ctx->pc] & 0b0000001111111111;

        switch(op) {
            case OP_MOV:
                ctx->r[a] = ctx->r[b];
                break;
            case OP_LOAD:

                break;
            case OP_PUSH:
                lgx_push(ctx, &ctx->r[c]);
                break;
            case OP_POP:
                ctx->r[c] = *lgx_pop(ctx);
                break;
            case OP_CMP:

                break;
            case OP_ADD:
                ctx->r[a].value.l += ctx->r[b].value.l;
                break;
            case OP_TEST:

                break;
            case OP_JMP:

                break;
            case OP_CALL:

                break;
            case OP_RET:
                lgx_vardump(lgx_pop(ctx));
                break;
            case OP_NOP:

                break;
            default:
                // error
                break;
        }

        ctx->pc ++;
    }
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

    lgx_ctx_t ctx;
    lgx_ctx_init(&ctx);
    lgx_execute(&ctx);

    return 0;
}