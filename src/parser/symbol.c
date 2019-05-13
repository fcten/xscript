#include "symbol.h"

// 追加一条错误信息
static void symbol_error(lgx_ast_t* ast, lgx_ast_node_t* node, const char *fmt, ...) {
    va_list   args;

    lgx_ast_error_list_t* err = xcalloc(1, sizeof(lgx_ast_error_list_t));
    if (!err) {
        return;
    }

    lgx_list_init(&err->head);
    err->err_no = 1;

    if (lgx_str_init(&err->err_msg, 256)) {
        xfree(err);
        return;
    }

    if (ast->lex.source.path) {
        err->err_msg.length = snprintf(err->err_msg.buffer, err->err_msg.size,
            "[ERROR] [%s:%d:%d] ", ast->lex.source.path, node->line, node->row);
    } else {
        err->err_msg.length = snprintf(err->err_msg.buffer, err->err_msg.size,
            "[ERROR] ");
    }

    va_start(args, fmt);
    err->err_msg.length += vsnprintf(err->err_msg.buffer + err->err_msg.length,
        256 - err->err_msg.length, fmt, args);
    va_end(args);
    
    lgx_list_add_tail(&err->head, &ast->errors);
}

static lgx_symbol_t* symbol_new() {
    return (lgx_symbol_t*)xcalloc(1, sizeof(lgx_symbol_t));
}

static int symbol_add(lgx_ast_t* ast, lgx_ast_node_t* node, lgx_ht_t *symbols, lgx_symbol_type_t type) {
    assert(node->type == IDENTIFIER_TOKEN);

    lgx_str_t name;
    name.buffer = ast->lex.source.content + node->offset;
    name.length = node->length;

    lgx_ht_node_t* ht_node = lgx_ht_get(symbols, &name);
    if (ht_node) {
        lgx_symbol_t* symbol = (lgx_symbol_t*)ht_node->v;
        symbol_error(ast, node, "symbol `%.*s` alreay declared at line %d row %d\n", name.length, name.buffer, symbol->node->line, symbol->node->row);
        return 1;
    }

    lgx_symbol_t* symbol = symbol_new();
    if (!symbol) {
        return 1;
    }
    symbol->type = type;
    symbol->node = node;

    if (lgx_ht_set(symbols, &name, symbol)) {
        symbol_error(ast, node, "symbol `%.*s` unkonwn error\n", name.length, name.buffer);
        return 1;
    }

    switch (type) {
    case S_VARIABLE:
        symbol_error(ast, node, "[variable] %.*s\n", name.length, name.buffer);
        break;
    case S_CONSTANT:
        symbol_error(ast, node, "[constant] %.*s\n", name.length, name.buffer);
        break;
    default:
        break;
    }

    return 1;
}

static int symbol_add_variable(lgx_ast_t* ast, lgx_ast_node_t* node) {
    lgx_ast_node_t* id = node->child[0];

    switch (node->parent->type) {
        case BLOCK_STATEMENT: { // 在块作用域内定义变量
            // 添加变量到该块作用域中
            if (symbol_add(ast, id, node->parent->u.symbols, S_VARIABLE)) {
                return 1;
            }
            break;
        }
        case FOR_STATEMENT: { // 在 for 循环条件中定义变量
            // TODO 必须是第一个循环条件
            if (0) {
                symbol_error(ast, id, "[invalid variable declaration] %.*s\n", id->length, ast->lex.source.content + id->offset);
                return 1;
            }

            // TODO 添加变量到该 FOR 循环的块作用域中
            break;
        }
        case FUNCTION_DECL_PARAMETER: { // 在函数参数列表中定义变量
            // TODO 添加变量到该函数的块作用域中
            break;
        }
        case CATCH_STATEMENT: { // 在 catch block 中定义变量
            // TODO 必须是第一个子节点，因为只能定义一个变量

            // TODO 添加变量到该异常处理的块作用域中
            break;
        }
        default:
            symbol_error(ast, id, "[invalid variable declaration] %.*s\n", id->length, ast->lex.source.content + id->offset);
            return 1;
    }

    return 0;
}

static int symbol_add_constant(lgx_ast_t* ast, lgx_ast_node_t* node) {
    lgx_ast_node_t* id = node->child[0];

    switch (node->parent->type) {
        case BLOCK_STATEMENT: { // 在块作用域内定义常量
            // 添加常量到该块作用域中
            if (symbol_add(ast, id, node->parent->u.symbols, S_CONSTANT)) {
                return 1;
            }
            break;
        }
        default:
            symbol_error(ast, id, "[invalid constant declaration] %.*s\n", id->length, ast->lex.source.content + id->offset);
            return 1;
    }

    return 0;
}

static int symbol_add_function(lgx_ast_t* ast, lgx_ast_node_t* node) {
    lgx_ast_node_t* id = node->child[0];

    switch (node->parent->type) {
        case BLOCK_STATEMENT: { // 在块作用域内定义函数
            // 添加函数到该块作用域中
            if (symbol_add(ast, id, node->parent->u.symbols, S_CONSTANT)) {
                return 1;
            }
            break;
        }
        default:
            symbol_error(ast, id, "[invalid function declaration] %.*s\n", id->length, ast->lex.source.content + id->offset);
            return 1;
    }

    return 0;
}

static int symbol_generate(lgx_ast_t* ast, lgx_ast_node_t* node) {
    int i, ret = 0;

    switch (node->type) {
        case PACKAGE_DECLARATION:
            break;
        case IMPORT_DECLARATION:
            break;
        case EXPORT_DECLARATION:
            break;
        case VARIABLE_DECLARATION:
            if (symbol_add_variable(ast, node)) {
                ret = 1;
            }
            break;
        case CONSTANT_DECLARATION:
            if (symbol_add_constant(ast, node)) {
                ret = 1;
            }
            break;
        case FUNCTION_DECLARATION:
            if (symbol_add_function(ast, node)) {
                ret = 1;
            }
            break;
        case TYPE_DECLARATION:
            break;
        default: break;
    }

    for (i = 0; i < node->children; ++i) {
        if (symbol_generate(ast, node->child[i])) {
            ret = 1;
        }
    }
    
    return ret;
}

// 遍历语法树生成符号信息
int lgx_symbol_init(lgx_ast_t* ast) {
    return symbol_generate(ast, ast->root);
}

// 根据名称和位置查找符号表
// 如果不限制声明必须在使用之前，则传入 offset = 0
lgx_symbol_t* lgx_symbol_get(lgx_ast_node_t* node, lgx_str_t* name, unsigned offset) {
    if (node->type == BLOCK_STATEMENT) {
        lgx_ht_node_t* ht_node = lgx_ht_get(node->u.symbols, name);
        if (ht_node) {
            lgx_symbol_t* n = (lgx_symbol_t*)ht_node->v;
            if (offset == 0 || n->node->offset < offset) {
                return n;
            }
        }
    }

    if (!node->parent) {
        return NULL;
    }

    return lgx_symbol_get(node->parent, name, offset);
}