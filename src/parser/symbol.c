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
            "[SYMBOL ERROR] [%s:%d:%d] ", ast->lex.source.path, node->line + 1, node->row);
    } else {
        err->err_msg.length = snprintf(err->err_msg.buffer, err->err_msg.size,
            "[SYMBOL ERROR] ");
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

static void symbol_del(lgx_symbol_t* symbol) {
    lgx_type_cleanup(&symbol->type);
    xfree(symbol);
}

static lgx_symbol_t* symbol_add(lgx_ast_t* ast, lgx_ast_node_t* node,
    lgx_ht_t* symbols, lgx_symbol_type_t s_type,
    lgx_str_t* name, unsigned char is_global) {

    lgx_ht_node_t* ht_node = lgx_ht_get(symbols, name);
    if (ht_node) {
        lgx_symbol_t* symbol = (lgx_symbol_t*)ht_node->v;
        symbol_error(ast, node, "symbol `%.*s` alreay declared at line %d row %d\n", name->length, name->buffer, symbol->node->line, symbol->node->row);
        return NULL;
    }

    lgx_symbol_t* symbol = symbol_new();
    if (!symbol) {
        return NULL;
    }
    symbol->s_type = s_type;
    symbol->node = node;
    symbol->is_global = is_global;

    if (lgx_ht_set(symbols, name, symbol)) {
        symbol_del(symbol);
        symbol_error(ast, node, "symbol `%.*s` unkonwn error\n", name->length, name->buffer);
        return NULL;
    }

    return symbol;
}

static int symbol_parse_type(lgx_ast_t* ast, lgx_ast_node_t* node, lgx_type_t* type);

static int symbol_parse_type_array(lgx_ast_t* ast, lgx_ast_node_t* node, lgx_type_array_t* arr) {
    assert(node->type == TYPE_EXPRESSION);
    assert(node->u.type == T_ARRAY);
    assert(node->children == 1);

    if (symbol_parse_type(ast, node->child[0], &arr->value)) {
        return 1;
    }

    return 0;
}

static int symbol_parse_type_map(lgx_ast_t* ast, lgx_ast_node_t* node, lgx_type_map_t* map) {
    assert(node->type == TYPE_EXPRESSION);
    assert(node->u.type == T_MAP);
    assert(node->children == 2);

    if (symbol_parse_type(ast, node->child[0], &map->key)) {
        return 1;
    }

    if (symbol_parse_type(ast, node->child[1], &map->value)) {
        return 1;
    }

    return 0;
}

static int symbol_parse_type_struct(lgx_ast_t* ast, lgx_ast_node_t* node, lgx_type_struct_t* sru) {
    assert(node->type == TYPE_EXPRESSION);
    assert(node->u.type == T_STRUCT);

    return 0;
}

static int symbol_parse_type_interface(lgx_ast_t* ast, lgx_ast_node_t* node, lgx_type_interface_t* itf) {
    assert(node->type == TYPE_EXPRESSION);
    assert(node->u.type == T_INTERFACE);

    return 0;
}

static int symbol_parse_type_function_receiver(lgx_ast_t* ast, lgx_ast_node_t* node, lgx_type_function_t* fun) {
    assert(node->type == FUNCTION_RECEIVER);

    if (node->children == 0) {
        return 0;
    }

    assert(node->children == 1);
    assert(node->child[0]->type == FUNCTION_DECL_PARAMETER);

    if (node->child[0]->children != 1) {
        symbol_error(ast, node, "there must be one and only one receiver of a method\n");
        return 1;
    }

    assert(node->child[0]->child[0]->type == VARIABLE_DECLARATION);
    assert(node->child[0]->child[0]->children == 2);
    assert(node->child[0]->child[0]->child[1]->type == TYPE_EXPRESSION);

    return symbol_parse_type(ast, node->child[0]->child[0]->child[1], &fun->receiver);
}

static int symbol_parse_type_function_parameter(lgx_ast_t* ast, lgx_ast_node_t* node, lgx_type_function_t* fun) {
    assert(node->type == FUNCTION_DECL_PARAMETER);

    if (node->children) {
        fun->args = xcalloc(node->children, sizeof(lgx_type_t*));
        if (!fun->args) {
            symbol_error(ast, node, "out of memory\n");
            return 1;
        }
    }
    fun->arg_len = node->children;

    int i, ret = 0;
    for (i = 0; i < node->children; ++i) {
        assert(node->child[i]->type == VARIABLE_DECLARATION);
        assert(node->child[i]->children == 2);
        assert(node->child[i]->child[1]->type == TYPE_EXPRESSION);

        fun->args[i] = xmalloc(sizeof(lgx_type_t));
        if (!fun->args[i]) {
            symbol_error(ast, node, "out of memory\n");
            return 1;
        }

        if (symbol_parse_type(ast, node->child[i]->child[1], fun->args[i])) {
            ret = 1;
        }
    }

    return ret;
}

static int symbol_parse_type_function_return(lgx_ast_t* ast, lgx_ast_node_t* node, lgx_type_function_t* fun) {
    assert(node->type == TYPE_EXPRESSION);

    return symbol_parse_type(ast, node, &fun->ret);
}

static int symbol_parse_type_function(lgx_ast_t* ast, lgx_ast_node_t* node, lgx_type_function_t* fun) {
    assert(node->type == TYPE_EXPRESSION);
    assert(node->u.type == T_FUNCTION);
    assert(node->children == 2);
    assert(node->child[0]->type == FUNCTION_DECL_PARAMETER);
    assert(node->child[1]->type == TYPE_EXPRESSION);

    if (symbol_parse_type_function_parameter(ast, node->child[0], fun)) {
        return 1;
    }

    return symbol_parse_type_function_return(ast, node->child[1], fun);
}

static int symbol_parse_type(lgx_ast_t* ast, lgx_ast_node_t* node, lgx_type_t* type) {
    assert(node->type == TYPE_EXPRESSION);

    if (lgx_type_init(type, node->u.type)) {
        symbol_error(ast, node, "out of memory\n");
        return 1;
    }

    type->literal.length = node->length;
    type->literal.buffer = ast->lex.source.content + node->offset;

    switch (node->u.type) {
        case T_ARRAY:
            if (symbol_parse_type_array(ast, node, type->u.arr)) {
                return 1;
            }
            break;
        case T_MAP:
            if (symbol_parse_type_map(ast, node, type->u.map)) {
                return 1;
            }
            break;
        case T_STRUCT:
            if (symbol_parse_type_struct(ast, node, type->u.sru)) {
                return 1;
            }
            break;
        case T_INTERFACE:
            if (symbol_parse_type_interface(ast, node, type->u.itf)) {
                return 1;
            }
            break;
        case T_FUNCTION:
            if (symbol_parse_type_function(ast, node, type->u.fun)) {
                return 1;
            }
            break;
        case T_CUSTOM:
            // TODO
            break;
        case T_UNKNOWN:
            // 未知类型，后续进行类型推断
            break;
        default:
            symbol_error(ast, node, "[invalid type expression] %.*s\n", type->literal.length, type->literal.buffer);
            return 1;
    }

    return 0;
}

static int symbol_add_variable(lgx_ast_t* ast, lgx_ast_node_t* node) {
    assert(node->children >= 2);
    assert(node->child[0]->type == IDENTIFIER_TOKEN);
    assert(node->child[1]->type == TYPE_EXPRESSION);

    // 变量名称标识符
    lgx_str_t name;
    name.buffer = ast->lex.source.content + node->child[0]->offset;
    name.length = node->child[0]->length;
    name.size = 0;

    // 是否全局变量
    unsigned char is_global = 0;
    if (node->parent && node->parent->type == BLOCK_STATEMENT && node->parent->parent == NULL) {
        is_global = 1;
    }

    int ret = 0;
    lgx_symbol_t* symbol = NULL;

    switch (node->parent->type) {
        case BLOCK_STATEMENT: { // 在块作用域内定义变量
            // 添加变量到该块作用域中
            symbol = symbol_add(ast, node,
                node->parent->u.symbols, S_VARIABLE,
                &name, is_global);
            break;
        }
        case FOR_STATEMENT: { // 在 for 循环条件中定义变量
            // TODO 必须是第一个循环条件
            if (0) {
                symbol_error(ast, node, "[invalid variable declaration] %.*s\n", name.length, name.buffer);
                ret = 1;
            } else {
                // TODO 添加变量到该 FOR 循环的块作用域中
            }
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
            symbol_error(ast, node, "[invalid variable declaration] %.*s\n", name.length, name.buffer);
            ret = 1;
    }

    if (!symbol) {
        return ret;
    }

    if (symbol_parse_type(ast, node->child[1], &symbol->type)) {
        return 1;
    }

    // 注意：这里解析完毕后，类型可能为未知，需要在编译阶段进行类型推断

    return 0;
}

static int symbol_add_constant(lgx_ast_t* ast, lgx_ast_node_t* node) {
    assert(node->children >= 2);
    assert(node->child[0]->type == IDENTIFIER_TOKEN);
    assert(node->child[1]->type == TYPE_EXPRESSION);

    // 常量名称标识符
    lgx_str_t name;
    name.buffer = ast->lex.source.content + node->child[0]->offset;
    name.length = node->child[0]->length;
    name.size = 0;

    // 是否全局常量
    unsigned char is_global = 0;
    if (node->parent && node->parent->type == BLOCK_STATEMENT && node->parent->parent == NULL) {
        is_global = 1;
    }

    int ret = 0;
    lgx_symbol_t* symbol = NULL;

    switch (node->parent->type) {
        case BLOCK_STATEMENT: { // 在块作用域内定义常量
            // 添加常量到该块作用域中
            symbol = symbol_add(ast, node,
                node->parent->u.symbols, S_CONSTANT,
                &name, is_global);
            break;
        }
        default:
            symbol_error(ast, node, "[invalid constant declaration] %.*s\n", name.length, name.buffer);
            ret = 1;
    }

    if (!symbol) {
        return ret;
    }

    if (symbol_parse_type(ast, node->child[1], &symbol->type)) {
        return 1;
    }

    // 注意：这里解析完毕后，类型可能为未知，需要在编译阶段进行类型推断

    return 0;
}

static int symbol_add_function(lgx_ast_t* ast, lgx_ast_node_t* node) {
    assert(node->children == 5);
    assert(node->child[0]->type == FUNCTION_RECEIVER);
    assert(node->child[1]->type == IDENTIFIER_TOKEN);
    assert(node->child[2]->type == FUNCTION_DECL_PARAMETER);
    assert(node->child[3]->type == TYPE_EXPRESSION);
    assert(node->child[4]->type == BLOCK_STATEMENT);

    // TODO 是否为成员函数

    // 函数名称标识符
    lgx_str_t name;
    name.buffer = ast->lex.source.content + node->child[1]->offset;
    name.length = node->child[1]->length;
    name.size = 0;

    // 是否全局函数
    unsigned char is_global = 0;
    if (node->parent && node->parent->type == BLOCK_STATEMENT && node->parent->parent == NULL) {
        is_global = 1;
    }

    int ret = 0;
    lgx_symbol_t* symbol = NULL;

    switch (node->parent->type) {
        case BLOCK_STATEMENT: { // 在块作用域内定义函数
            // 添加函数到该块作用域中
            symbol = symbol_add(ast, node,
                node->parent->u.symbols, S_CONSTANT,
                &name, is_global);
            break;
        }
        default:
            symbol_error(ast, node, "[invalid function declaration] %.*s\n", name.length, name.buffer);
            ret = 1;
    }

    if (!symbol) {
        return ret;
    }

    if (lgx_type_init(&symbol->type, T_FUNCTION)) {
        symbol_error(ast, node, "out of memory\n");
        return 1;
    }

    // 解析接收者
    if (symbol_parse_type_function_receiver(ast, node->child[0], symbol->type.u.fun)) {
        ret = 1;
    }

    // 解析参数列表
    if (symbol_parse_type_function_parameter(ast, node->child[2], symbol->type.u.fun)) {
        ret = 1;
    }

    // 解析返回值
    if (symbol_parse_type_function_return(ast, node->child[3], symbol->type.u.fun)) {
        ret = 1;
    }

    return ret;
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

static void symbol_cleanup(lgx_ast_node_t* node) {
    if (node->type == BLOCK_STATEMENT) {
        lgx_ht_node_t* n;
        for (n = lgx_ht_first(node->u.symbols); n; n = lgx_ht_next(n)) {
            symbol_del(n->v);
            n->v = NULL;
        }
    }

    int i;
    for (i = 0; i < node->children; ++i) {
        symbol_cleanup(node->child[i]);
    }
}

void lgx_symbol_cleanup(lgx_ast_t* ast) {
    symbol_cleanup(ast->root);
}

// 根据名称和位置查找符号表
lgx_symbol_t* lgx_symbol_get(lgx_ast_node_t* node, lgx_str_t* name, unsigned offset) {
    if (node->type == BLOCK_STATEMENT) {
        lgx_ht_node_t* ht_node = lgx_ht_get(node->u.symbols, name);
        if (ht_node) {
            lgx_symbol_t* n = (lgx_symbol_t*)ht_node->v;
            // 全局函数不需要遵循先声明后使用的原则
            if ((node->parent == NULL && n->s_type == S_CONSTANT && n->type.type == T_FUNCTION) ||
                n->node->offset <= offset) {
                return n;
            }
        }
    }

    if (!node->parent) {
        return NULL;
    }

    return lgx_symbol_get(node->parent, name, offset);
}