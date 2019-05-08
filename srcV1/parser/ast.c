#include "ast.h"

void ast_node_cleanup(lgx_ast_node_t* node) {
    switch (node->type) {
        case BLOCK_STATEMENT:
            if (node->u.symbols) {
                lgx_ht_cleanup(node->u.symbols);
                node->u.symbols = NULL;
            }
            break;
        default:
            break;
    }
}

lgx_ast_node_t* ast_node_new(lgx_ast_t* ast, lgx_ast_type_t type, lgx_ast_node_t* parent) {
    lgx_ast_node_t* node = (lgx_ast_node_t*)xcalloc(1, sizeof(lgx_ast_node_t));
    if (!node) {
        return NULL;
    }

    node->offset = ast->lex.offset;
    node->line = ast->lex.line;
    node->row = ast->lex.row;

    node->type = type;
    switch (type) {
        case BLOCK_STATEMENT:
            node->u.symbols = (lgx_ht_t*)xmalloc(sizeof(lgx_ht_t));
            if (!node->u.symbols) {
                goto error;
            }
            if (lgx_ht_init(node->u.symbols) != 0) {
                goto error;
            }
            break;
        default:
            break;
    }

    return node;

error:
    if (node) {
        ast_node_cleanup(node);
        xfree(node);
    }

    return NULL;
}

// 解析下一个 token
void ast_step(lgx_ast_t* ast) {
    lgx_token_t token;
    int loop = 1;
    while (loop) {
        token = lgx_lex_next(&ast->lex);
        switch (token) {
            case TK_SPACE:
            case TK_COMMENT:
                // 忽略空格与注释
                break;
            default:
                loop = 0;
        }
    }

    // TODO 遇到 TK_EOL 时，自动决定是忽略还是转换为分号

    ast->prev_token = ast->cur_token;

    ast->cur_token = token;
    ast->cur_start = ast->lex.source.content + ast->lex.milestone;
    ast->cur_length = ast->lex.offset - ast->lex.milestone;
}

// 追加一条错误信息
void ast_error(lgx_ast_t* ast, const char *fmt, ...) {

}

int ast_parse(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    while (1) {
        // 解析语句
        switch (ast->cur_token) {
            case TK_EOF:
                // 读取到文件末尾，解析结束
                return 0;
            case TK_PACKAGE:
                
                break;
            case TK_IMPORT:
                
                break;
            case TK_VAR:

                break;
            case TK_CONST:

                break;
            case TK_FUNCTION:

                break;
            case TK_TYPE:

                break;
            default:
                break;
        }
        
        // 语句应当以分号结尾
        if (ast->cur_token != TK_SEMICOLON) {
            ast_error(ast, "';' expected before '%.*s'\n", ast->cur_length, ast->cur_start);
            return 1;
        }
        ast_step(ast);
    }
}

int lgx_ast_init(lgx_ast_t* ast, char* file) {
    // 初始化
    memset(ast, 0, sizeof(lgx_ast_t));
    lgx_list_init(&ast->errors);

    if (lgx_lex_init(&ast->lex, file) != 0) {
        return 1;
    }

    // 初始化根节点
    ast->root = ast_node_new(ast, BLOCK_STATEMENT, NULL);
    if (!ast->root) {
        return 2;
    }

    // 读取一个 token
    ast_step(ast);

    return ast_parse(ast, ast->root);
}