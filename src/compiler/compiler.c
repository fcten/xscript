#include "../common/common.h"
#include "../parser/symbol.h"
#include "register.h"
#include "compiler.h"
#include "bytecode.h"
#include "constant.h"
#include "exception.h"
#include "expr_result.h"

// 追加一条错误信息
static void compiler_error(lgx_compiler_t* c, lgx_ast_node_t* node, const char *fmt, ...) {
    va_list   args;

    assert(c->ast);
    lgx_ast_t* ast = c->ast;

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

// 禁止在逻辑操作中使用赋值表达式，无论它的返回值是否为布尔值
static int compiler_check_assignment(lgx_compiler_t* c, lgx_ast_node_t *node) {
    if (node->type == BINARY_EXPRESSION && node->u.op == TK_ASSIGN) {
        compiler_error(c, node, "assignment expression can't be used as boolean value\n");
        return 1;
    }

    return 0;
}

static int compiler_expression(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    return 0;
}

static int compiler_block_statement(lgx_compiler_t* c, lgx_ast_node_t *node);

static int compiler_if_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == IF_STATEMENT);
    assert(node->children == 2);
    assert(node->child[1]->type == BLOCK_STATEMENT);

    if (compiler_check_assignment(c, node->child[0])) {
        return 1;
    }

    lgx_expr_result_t e;
    lgx_expr_result_init(&e);
    if (compiler_expression(c, node->child[0], &e)) {
        return 1;
    }

    if (check_constant(&e, T_BOOL)) {
        if (e.v.v.l == 0) {
            // if 条件始终为 false
            return 0;
        } else {
            return compiler_block_statement(c, node->child[1]);
        }
    }
    
    if (check_variable(&e, T_BOOL)) {
        unsigned pos = c->bc.length; // 跳出指令位置
        bc_test(c, &e, 0);
        lgx_expr_result_cleanup(&e);

        if (compiler_block_statement(c, node->child[1])) {
            return 1;
        }

        bc_set_param_d(c, pos, c->bc.length - pos - 1);
        return 0;
    }

    lgx_expr_result_cleanup(&e);
    compiler_error(c, node, "makes boolean from %s without a cast\n", lgx_type_to_string(&e.v));
    return 1;
}

static int compiler_if_else_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == IF_ELSE_STATEMENT);
    assert(node->children == 3);
    assert(node->child[1]->type == BLOCK_STATEMENT);
    assert(node->child[2]->type == BLOCK_STATEMENT);

    if (compiler_check_assignment(c, node->child[0])) {
        return 1;
    }

    lgx_expr_result_t e;
    lgx_expr_result_init(&e);
    if (compiler_expression(c, node->child[0], &e)) {
        return 1;
    }

    if (check_constant(&e, T_BOOL)) {
        if (e.v.v.l == 0) {
            return compiler_block_statement(c, node->child[2]);
        } else {
            return compiler_block_statement(c, node->child[1]);
        }
    }

    if (check_variable(&e, T_BOOL)) {
        unsigned pos1 = c->bc.length; // 跳转指令位置
        bc_test(c, &e, 0);
        lgx_expr_result_cleanup(&e);

        if (compiler_block_statement(c, node->child[1])) {
            return 1;
        }

        unsigned pos2 = c->bc.length; // 跳转指令位置
        bc_jmpi(c, 0);
        bc_set_param_d(c, pos1, c->bc.length - pos1 - 1);

        if (compiler_block_statement(c, node->child[2])) {
            return 1;
        }

        bc_set_param_e(c, pos2, c->bc.length);
        return 0;
    }

    lgx_expr_result_cleanup(&e);
    compiler_error(c, node, "makes boolean from %s without a cast\n", lgx_type_to_string(&e.v));
    return 1;
}

static int compiler_for_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == FOR_STATEMENT);
    assert(node->children == 4);
    assert(node->child[0]->type == FOR_CONDITION);
    assert(node->child[0]->children <= 1);
    assert(node->child[1]->type == FOR_CONDITION);
    assert(node->child[1]->children <= 1);
    assert(node->child[2]->type == FOR_CONDITION);
    assert(node->child[2]->children <= 1);
    assert(node->child[3]->type == BLOCK_STATEMENT);

    if (node->child[1]->children == 1 && compiler_check_assignment(c, node->child[1]->child[0])) {
        return 1;
    }

    // exp1: 循环开始前执行一次
    if (node->child[0]->children == 1) {
        lgx_expr_result_t e;
        lgx_expr_result_init(&e);
        if (compiler_expression(c, node->child[0]->child[0], &e)) {
            return 1;
        }
        lgx_expr_result_cleanup(&e);
    }

    unsigned pos1 = c->bc.length; // 跳转指令位置
    unsigned pos2 = 0; // 跳出指令位置

    // exp2: 每次循环开始前执行一次，如果值为假，则跳出循环
    if (node->child[1]->children == 1) {
        lgx_expr_result_t e;
        lgx_expr_result_init(&e);

        if (compiler_expression(c, node->child[1]->child[0], &e)) {
            return 1;
        }

        if (check_constant(&e, T_BOOL)) {
            if (e.v.v.l == 0) {
                return 0;
            }
        } else if (check_variable(&e, T_BOOL)) {
            pos2 = c->bc.length; 
            bc_test(c, &e, 0);
            lgx_expr_result_cleanup(&e);
        } else {
            compiler_error(c, node, "makes boolean from %s without a cast\n", lgx_type_to_string(&e));
            return 1;
        }
    }

    // 循环主体
    if (compiler_block_statement(c, node->child[3])) {
        return 1;
    }

    unsigned pos3 = c->bc.length; // 跳转指令位置

    // exp3: 每次循环结束后执行一次
    if (node->child[2]->children == 1) {
        lgx_expr_result_t e;
        lgx_expr_result_init(&e);
        if (compiler_expression(c, node->child[2]->child[0], &e)) {
            return 1;
        }
        lgx_expr_result_cleanup(&e);
    }

    bc_jmpi(c, pos1);

    if (pos2) {
        bc_set_param_d(c, pos2, c->bc.length - pos2 - 1);
    }

    jmp_fix(c, node, pos3, c->bc.length);
    return 0;
}

static int compiler_while_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == WHILE_STATEMENT);
    assert(node->children == 2);
    assert(node->child[1]->type == BLOCK_STATEMENT);

    if (compiler_check_assignment(c, node->child[0])) {
        return 1;
    }

    unsigned start = c->bc.length; // 循环起始位置

    lgx_expr_result_t e;
    lgx_expr_result_init(&e);
    if (compiler_expression(c, node->child[0], &e)) {
        return 1;
    }

    if (check_constant(&e, T_BOOL)) {
        if (e.v.v.l == 0) {
            return 0;
        } else {
            if (compiler_block_statement(c, node->child[1])) {
                return 1;
            }
            // 写入无条件跳转
            bc_jmpi(c, start);
        }
    } else if (check_variable(&e, T_BOOL)) {
        unsigned pos = c->bc.length; // 循环跳出指令位置
        bc_test(c, &e, 0);
        lgx_expr_result_cleanup(&e);

        if (compiler_block_statement(c, node->child[1])) {
            return 1;
        }

        // 写入无条件跳转
        bc_jmpi(c, start);
        // 更新条件跳转
        bc_set_param_d(c, pos, c->bc.length - pos - 1);
    } else {
        lgx_expr_result_cleanup(&e);
        compiler_error(c, node, "makes boolean from %s without a cast\n", lgx_type_to_string(&e));
        return 1;
    }

    jmp_fix(c, node, start, c->bc.length);
    return 0;
}

static int compiler_do_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == DO_STATEMENT);
    assert(node->children == 2);
    assert(node->child[0]->type == BLOCK_STATEMENT);

    if (compiler_check_assignment(c, node->child[1])) {
        return 1;
    }

    unsigned start = c->bc.length; // 循环起始位置

    if (compiler_block_statement(c, node->child[0])) {
        return 1;
    }

    lgx_expr_result_t e;
    lgx_expr_result_init(&e);
    if (compiler_expression(c, node->child[1], &e)) {
        return 1;
    }

    if (check_constant(&e, T_BOOL)) {
        if (e.v.v.l == 0) {
            // 循环条件始终为 false
        } else {
            // 写入无条件跳转
            bc_jmpi(c, start);
        }
    } else if (check_variable(&e, T_BOOL)) {
        unsigned pos = c->bc.length;
        bc_test(c, &e, 0);
        bc_jmpi(c, start);
        lgx_expr_result_cleanup(&e);

        bc_set_param_d(c, pos, c->bc.length - pos - 1);
    } else {
        compiler_error(c, node, "makes boolean from %s without a cast\n", lgx_type_to_string(&e));
        return 1;
    }

    jmp_fix(c, node, start, c->bc.length);
    return 0;
}

static int compiler_continue_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == CONTINUE_STATEMENT);
    assert(node->children == 0);

    // 只能出现在循环语句中
    if (jmp_add(c, node)) {
        // error
        return 1;
    }

    node->u.pos = c->bc.length;
    bc_jmpi(c, 0);

    return 0;
}

static int compiler_break_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == BREAK_STATEMENT);
    assert(node->children == 0);

    // 只能出现在循环语句和 switch 语句中
    if (jmp_add(c, node)) {
        // error
        return 1;
    }

    node->u.pos = c->bc.length;
    bc_jmpi(c, 0);

    return 0;
}

static int compiler_switch_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == SWITCH_STATEMENT);
    assert(node->children >= 1);

    // 执行表达式
    lgx_expr_result_t e;
    lgx_expr_result_init(&e);
    if (compiler_expression(c, node->child[0], &e)) {
        return 1;
    }

    if (!check_type(&e, T_LONG) && check_type(&e, T_STRING)) {
        compiler_error(c, node, "makes integer or string from %s without a cast\n", lgx_type_to_string(&e));
        return 1;
    }

    if (node->children == 1) {
        // 没有 case 和 default，什么都不用做
        lgx_expr_result_cleanup(&e);
    } else {
        lgx_val_t condition;
        condition.type = T_ARRAY;
        condition.v.arr = lgx_hash_new(node->children - 1);

        int i, has_default = 0;
        for (i = 1; i < node->children; i ++) {
            lgx_ast_node_t *child = node->child[i];
            if (child->type == CASE_STATEMENT) {
                lgx_hash_node_t case_node;
                lgx_val_init(&case_node.k);

                if (bc_expr(bc, child->child[0], &case_node.k)) {
                    return 1;
                }

                if (!check_constant(&case_node.k, T_LONG) && !check_constant(&case_node.k, T_STRING)) {
                    compiler_error(c, child, "only constant expression allowed in case label");
                    return 1;
                }

                if (lgx_hash_get(condition.v.arr, &case_node.k)) {
                    compiler_error(c, child, "duplicate case value");
                    return 1;
                }

                case_node.v.type = T_LONG;
                case_node.v.v.l = 0;

                lgx_hash_set(condition.v.arr, &case_node);
            } else {
                has_default = 1;
            }
        }

        lgx_val_t undef;
        undef.type = T_UNDEFINED;
        undef.u.symbol.reg_type = R_TEMP;
        undef.u.symbol.reg_num = reg_pop(bc);

        lgx_val_t tmp, result;
        tmp.u.symbol.reg_type = R_TEMP;
        tmp.u.symbol.reg_num = reg_pop(bc);
        result.u.symbol.reg_type = R_TEMP;
        result.u.symbol.reg_num = reg_pop(bc);

        bc_load(bc, &tmp, &condition);
        bc_array_get(bc, &tmp, &tmp, &e);
        bc_load(bc, &undef, &undef);
        bc_eq(bc, &result, &tmp, &undef);
        bc_test(bc, &result, 1);

        unsigned pos = c->bc.length;
        bc_jmpi(bc, 0);
        bc_jmp(bc, &tmp);

        reg_free(bc, &result);
        reg_free(bc, &tmp);
        reg_free(bc, &undef);
        reg_free(bc, &e);

        for (i = 1; i < node->children; i ++) {
            lgx_ast_node_t *child = node->child[i];

            if (child->type == CASE_STATEMENT) {
                lgx_val_t k;
                if (bc_expr(bc, child->child[0], &k)) {
                    return 1;
                }

                lgx_hash_node_t *n = lgx_hash_get(condition.v.arr, &k);
                n->v.v.l = c->bc.length;

                // 这里只应该出现 T_LONG 和 T_STRING
                if (IS_GC_VALUE(&k)) {
                    lgx_val_free(&k);
                }

                if (bc_stat(bc, child->child[1])) {
                    return 1;
                }
            } else { // child->type == DEFAULT_STATEMENT
                bc_set_pe(bc, pos, c->bc.length);

                if (bc_stat(bc, child->child[0])) {
                    return 1;
                }
            }
        }

        if (!has_default) {
            bc_set_pe(bc, pos, c->bc.length);
        }
    }

    jmp_fix(c, node, 0, c->bc.length);
    return 0;
}

static int compiler_try_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == TRY_STATEMENT);
    assert(node->children >= 1);

    int ret = 0;

    lgx_exception_t *exception = lgx_exception_new();

    exception->try_block.start = c->bc.length;
    if (compiler_block_statement(c, node->child[0])) {
        ret = 1;
    }
    exception->try_block.end = c->bc.length;

    if (exception->try_block.start == exception->try_block.end) {
        // try block 没有编译产生任何代码
        lgx_exception_delete(exception);
        return ret;
    } else {
        exception->try_block.end --;
    }

    int i;
    for (i = 1; i < node->children; i ++) {
        lgx_ast_node_t *n = node->child[i];
        assert(n->type == CATCH_STATEMENT);

        lgx_exception_block_t *block = lgx_exception_block_new();

        unsigned pos = c->bc.length;
        bc_jmpi(c, 0);

        // TODO 处理参数

        // 编译 catch block
        block->start = c->bc.length;
        if (compiler_block_statement(c, n->child[1])) {
            ret = 1;
        }
        block->end = c->bc.length;

        if (block->start == block->end) {
            // catch block 没有编译产生任何代码
            bc_nop(c);
        }
        block->end --;

        bc_set_param_e(c, pos, c->bc.length);

        lgx_list_add_tail(&block->head, &exception->catch_blocks);

        // TODO 保存该 catch block 的参数
    }

    if (lgx_list_empty(&exception->catch_blocks)) {
        // 如果没有任何 catch block
        lgx_exception_delete(exception);
        return ret;
    }

    // 添加 exception
    unsigned long long key = ((unsigned long long)exception->try_block.start << 32) | (0XFFFFFFFF - exception->try_block.end);

    lgx_str_t k;
    k.buffer = (char *)&key;
    k.length = sizeof(key);

    lgx_rb_node_t *n = lgx_rb_insert(&c->exception, &k);
    assert(n);
    n->value = exception;

    return 0;
}

static int compiler_throw_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == THROW_STATEMENT);
    assert(node->children == 1);

    lgx_expr_result_t e;
    lgx_expr_result_init(&e);

    if (compiler_expression(c, node->child[0], &e)) {
        return 1;
    }
    bc_throw(c, &e);

    lgx_expr_result_cleanup(&e);

    return 0;
}

static int compiler_return_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == RETURN_STATEMENT);
    assert(node->children == 1);

    lgx_expr_result_t r;
    lgx_expr_result_init(&r);

    // 获取返回值类型
    lgx_ast_node_t *n = node->parent;
    while (n && n->type != FUNCTION_DECLARATION) {
        if (n->parent) {
            n = n->parent;
        } else {
            n = NULL;
        }
    }

    lgx_val_t ret;
    if (n) {
        lgx_str_t s;
        s.buffer = ((lgx_ast_node_token_t *)(n->child[0]))->tk_start;
        s.length = ((lgx_ast_node_token_t *)(n->child[0]))->tk_length;
        lgx_val_t *f = lgx_scope_val_get(n, &s);
        ret = f->v.fun->ret;
    } else {
        ret.type = T_UNDEFINED;
    }

    unsigned is_tail = 0;

    // 计算返回值
    if (node->child[0]) {
        if (node->child[0]->type == BINARY_EXPRESSION && node->child[0]->u.op == TK_CALL) {
            // 尾调用
            // 内建函数以及 async 函数不能也不需要做尾调用优化，所以这里也可能生成一个普通调用
            if (bc_expr_binary_tail_call(bc, node->child[0], &r, &is_tail)) {
                return 1;
            }
        } else {
            if (bc_expr(bc, node->child[0], &r)) {
                return 1;
            }
        }

        if (ret.type != T_UNDEFINED && !is_auto(&r) && ret.type != r.type) {
            compiler_error(c, node, "makes %s from %s without a cast\n", lgx_type_to_string(&ret), lgx_type_to_string(&r));
            return 1;
        }

        if (ret.type == T_OBJECT && !lgx_obj_is_instanceof(r.v.obj, ret.v.obj)) {
            lgx_str_t *n1 = lgx_obj_get_name(r.v.obj);
            lgx_str_t *n2 = lgx_obj_get_name(ret.v.obj);
            compiler_error(c, node, "makes %s<%.*s> from %s<%.*s> without a cast\n",
                lgx_type_to_string(&r),
                n1->length, n1->buffer,
                lgx_type_to_string(&ret),
                n2->length, n2->buffer
            );
            return 1;
        }
    } else {
        if (ret.type == T_UNDEFINED) {
            r.u.symbol.reg_type = R_LOCAL;
        } else {
            compiler_error(c, node, "makes %s from undefined without a cast\n", lgx_type_to_string(&ret));
            return 1;
        }
    }

    // 写入返回指令
    if (!is_tail) {
        bc_ret(bc, &r);
    }
    reg_free(bc, &r);

    return 0;
}

static int compiler_expression_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == EXPRESSION_STATEMENT);
    assert(node->children == 1);

    lgx_expr_result_t e;
    lgx_expr_result_init(&e);
    if (compiler_expression(c, node->child[0], &e)) {
        return 1;
    }
    lgx_expr_result_cleanup(&e);

    return 0;
}

static int compiler(lgx_compiler_t* c, lgx_ast_node_t *node) {
    switch(node->type) {
        case IF_STATEMENT: return compiler_if_statement(c, node);
        case IF_ELSE_STATEMENT: return compiler_if_else_statement(c, node);
        case FOR_STATEMENT: return compiler_for_statement(c, node);
        case WHILE_STATEMENT: return compiler_while_statement(c, node);
        case DO_STATEMENT: return compiler_do_statement(c, node);
        case CONTINUE_STATEMENT: return compiler_continue_statement(c, node);
        case BREAK_STATEMENT: return compiler_break_statement(c, node);
        case SWITCH_STATEMENT: return compiler_switch_statement(c, node);
        case TRY_STATEMENT: return compiler_try_statement(c, node);
        case THROW_STATEMENT: return compiler_throw_statement(c, node);
        case RETURN_STATEMENT: return compiler_return_statement(c, node);
        case EXPRESSION_STATEMENT: return compiler_expression_statement(c, node);
        // 忽略声明
        case VARIABLE_DECLARATION:
        case CONSTANT_DECLARATION:
        case TYPE_DECLARATION:
            break;
        default:
            compiler_error(c, node, "invalid ast-node type %d\n", node->type);
            return 1;
    }

    return 0;
}

static int compiler_block_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == BLOCK_STATEMENT);

    int i;
    for(i = 0; i < node->children; ++i) {
        if (compiler_statement(c, node->child[i])) {
            return 1;
        }
    }

    return 0;
}

static int compiler_function(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == FUNCTION_DECLARATION);
    assert(node->children == 4);
    assert(node->child[0]->type == IDENTIFIER_TOKEN);
    assert(node->child[1]->type == FUNCTION_DECL_PARAMETER);
    assert(node->child[2]->type == TYPE_EXPRESSION);
    assert(node->child[3]->type == BLOCK_STATEMENT);

    // 为局部变量分配寄存器
    lgx_ht_node_t* n;
    for (n = lgx_ht_first(node->child[3]->u.symbols); n; n = lgx_ht_next(n)) {
        lgx_symbol_t *symbol = (lgx_symbol_t *)n->v;
        // TODO 如果符号类型为变量
        int reg = lgx_reg_pop(node->u.regs);
        if (reg < 0) {
            compiler_error(c, symbol->node, "too many local variables\n");
            return 1;
        }
    }

    // 编译语句
    return compiler_block_statement(c, node->child[3]);
}

static int compiler(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == BLOCK_STATEMENT);

    // 为全局变量分配寄存器
    lgx_ht_node_t* n;
    for (n = lgx_ht_first(node->u.symbols); n; n = lgx_ht_next(n)) {
        lgx_symbol_t *symbol = (lgx_symbol_t *)n->v;
        // TODO 如果符号类型为变量
        if (c->global.length >= 65535) {
            compiler_error(c, symbol->node, "too many global variables\n");
            return 1;
        }
        if (lgx_ht_set(&c->global, &n->k, c->global.length)) {
            return 1;
        }
    }

    // 编译函数
    int i;
    for(i = 0; i < node->children; ++i) {
        if (node->child[i]->type == FUNCTION_DECLARATION) {
            if (compiler_function(c, node->child[i])) {
                return 1;
            }
        }
    }

    return 0;
}

int lgx_compiler_init(lgx_compiler_t* c) {
    memset(c, 0, sizeof(lgx_compiler_t));

    if (lgx_str_init(&c->bc, 1024)) {
        lgx_compiler_cleanup(c);
        return 1;
    }

    if (lgx_ht_init(&c->constant, 32)) {
        lgx_compiler_cleanup(c);
        return 1;
    }

    if (lgx_ht_init(&c->global, 32)) {
        lgx_compiler_cleanup(c);
        return 1;
    }

    if (lgx_rb_init(&c->exception, LGX_RB_KEY_INTEGER)) {
        lgx_compiler_cleanup(c);
        return 1;
    }

    return 0;
}

int lgx_compiler_generate(lgx_compiler_t* c, lgx_ast_t *ast) {
    c->ast = ast;
    int ret = compiler(c, ast->root);
    c->ast = NULL;

    return ret;
}

void lgx_compiler_cleanup(lgx_compiler_t* c) {
    // 释放字节码缓存
    lgx_str_cleanup(&c->bc);

    // 是否常量表
    lgx_heap_cleanup(&c->constant);

    // 释放堆变量表
    lgx_heap_cleanup(&c->global);

    // 释放异常信息
    lgx_rb_node_t *node;
    for (node = wbt_rb_first(&c->exception); node; node = lgx_rb_next(node)) {
        lgx_exception_del((lgx_exception_t *)node->value);
    }
    lgx_rb_cleanup(&c->exception);

    // 重新初始化内存，避免野指针
    memset(c, 0, sizeof(lgx_compiler_t));
}