#include "../common/common.h"
#include "../parser/symbol.h"
#include "register.h"
#include "compiler.h"
#include "bytecode.h"
#include "constant.h"
#include "exception.h"

// 追加一条错误信息
static void compiler_error(lgx_ast_t* ast, lgx_ast_node_t* node, const char *fmt, ...) {
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

// 禁止在逻辑操作中使用赋值表达式，无论它的返回值是否为布尔值
static int compiler_check_assignment(lgx_compiler_t* c, lgx_ast_node_t *node) {
    if (node->type == BINARY_EXPRESSION && node->u.op == TK_ASSIGN) {
        compiler_error(c, node, "assignment expression can't be used as boolean value\n");
        return 1;
    }

    return 0;
}

typedef enum {
    // 字面量
    EXPR_LITERAL = 0,
    // 局部变量
    EXPR_LOCAL,
    // 全局变量
    EXPR_GLOBAL,
    // 临时（中间）变量
    EXPR_TEMP,
    // 常量
    EXPR_CONSTANT
} lgx_expr_type_t;

typedef struct {
    lgx_expr_type_t type;

    // 如果结果为字面量，存储字面量具体的值
    // 否则，存储常量或变量的类型
    lgx_value_t v;

    union {
        // 如果结果为常量，存储常量编号
        int constant;
        // 如果结果为局部变量，存储寄存器编号
        int local;
        // 如果结果为全局变量，存储全局变量编号
        int global;
    } u;
} lgx_expr_result_t;

#define is_literal(e)   ((e)->type == EXPR_LITERAL)
#define is_const(e)     ((e)->type == EXPR_CONSTANT)
#define is_global(e)    ((e)->type == EXPR_GLOBAL)
#define is_local(e)     ((e)->type == EXPR_LOCAL)
#define is_temp(e)      ((e)->type == EXPR_TEMP)
#define is_constant(e)  (is_literal(e) || is_const(e))
#define is_variable(e)  (is_global(e) || is_local(e) || is_temp(e))

#define check_type(e, t)     ((e)->v.type == t)
#define check_constant(e, t) (is_constant(e) && check_type(e, t))
#define check_variable(e, t) (is_variable(e) && check_type(e, t))

static int lgx_expr_result_init(lgx_expr_result_t* e) {
    memset(e, 0, sizeof(lgx_expr_result_t));
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
        reg_free(c, &e);

        if (compiler_block_statement(c, node->child[1])) {
            return 1;
        }

        bc_set_pd(c, pos, c->bc.length - pos - 1);
        return 0;
    }

    reg_free(c, &e);
    compiler_error(c, node, "makes boolean from %s without a cast\n", lgx_type_to_string(&e.v));
    return 1;
}

static int compiler(lgx_compiler_t* c, lgx_ast_node_t *node) {
    switch(node->type) {
        case IF_STATEMENT: return compiler_if_statement(c, node);
        case IF_ELSE_STATEMENT:{
            // 禁止在逻辑操作中使用赋值表达式，无论它的返回值是否为布尔值
            bc_expr_block_assignment(bc, node->child[0]);

            lgx_val_t e;
            lgx_val_init(&e);
            if (bc_expr(bc, node->child[0], &e)) {
                return 1;
            }

            if (check_constant(&e, T_BOOL)) {
                if (e.v.l == 0) {
                    if( bc_stat(bc, node->child[2])) {
                        return 1;
                    }
                } else {
                    if (bc_stat(bc, node->child[1])) {
                        return 1;
                    }
                }
            } else if (check_variable(&e, T_BOOL)) {
                unsigned pos1 = bc->bc_top; // 跳转指令位置
                bc_test(bc, &e, 0);
                reg_free(bc, &e);

                if (bc_stat(bc, node->child[1])) {
                    return 1;
                }

                unsigned pos2 = bc->bc_top; // 跳转指令位置
                bc_jmpi(bc, 0);
                bc_set_pd(bc, pos1, bc->bc_top - pos1 - 1);

                if (bc_stat(bc, node->child[2])) {
                    return 1;
                }

                bc_set_pe(bc, pos2, bc->bc_top);
            } else {
                bc_error(bc, node, "makes boolean from %s without a cast\n", lgx_val_typeof(&e));
                return 1;
            }
            break;
        }
        case FOR_STATEMENT:{
            // 禁止在逻辑操作中使用赋值表达式，无论它的返回值是否为布尔值
            if (node->child[1]) {
                bc_expr_block_assignment(bc, node->child[1]);
            }

            // exp1: 循环开始前执行一次
            if (node->child[0]) {
                lgx_val_t e;
                lgx_val_init(&e);
                if (bc_expr(bc, node->child[0], &e)) {
                    return 1;
                }
            }

            unsigned pos1 = bc->bc_top; // 跳转指令位置
            unsigned pos2 = 0; // 跳出指令位置

            // exp2: 每次循环开始前执行一次，如果值为假，则跳出循环
            if (node->child[1]) {
                lgx_val_t e;
                lgx_val_init(&e);
                if (bc_expr(bc, node->child[1], &e)) {
                    return 1;
                }

                if (check_constant(&e, T_BOOL)) {
                    if (e.v.l == 0) {
                        break;
                    }
                } else if (check_variable(&e, T_BOOL)) {
                    pos2 = bc->bc_top; 
                    bc_test(bc, &e, 0);
                    reg_free(bc, &e);
                } else {
                    bc_error(bc, node, "makes boolean from %s without a cast\n", lgx_val_typeof(&e));
                    return 1;
                }
            }

            // 循环主体
            if (bc_stat(bc, node->child[3])) {
                return 1;
            }

            unsigned pos3 = bc->bc_top; // 跳转指令位置

            // exp3: 每次循环结束后执行一次
            if (node->child[2]) {
                lgx_val_t e;
                lgx_val_init(&e);
                if (bc_expr(bc, node->child[2], &e)) {
                    return 1;
                }
            }

            bc_jmpi(bc, pos1);

            if (pos2) {
                bc_set_pd(bc, pos2, bc->bc_top - pos2 - 1);
            }

            jmp_fix(bc, node, pos3, bc->bc_top);

            break;
        }
        case WHILE_STATEMENT:{
            // 禁止在逻辑操作中使用赋值表达式，无论它的返回值是否为布尔值
            bc_expr_block_assignment(bc, node->child[0]);

            unsigned start = bc->bc_top; // 循环起始位置
            lgx_val_t e;
            lgx_val_init(&e);
            if (bc_expr(bc, node->child[0], &e)) {
                return 1;
            }

            if (check_constant(&e, T_BOOL)) {
                if (e.v.l == 0) {
                    break;
                } else {
                    if (bc_stat(bc, node->child[1])) {
                        return 1;
                    }
                    // 写入无条件跳转
                    bc_jmpi(bc, start);
                }
            } else if (check_variable(&e, T_BOOL)) {
                unsigned pos = bc->bc_top; // 循环跳出指令位置
                bc_test(bc, &e, 0);
                reg_free(bc, &e);

                if (bc_stat(bc, node->child[1])) {
                    return 1;
                }
                // 写入无条件跳转
                bc_jmpi(bc, start);
                // 更新条件跳转
                bc_set_pd(bc, pos, bc->bc_top - pos - 1);
            } else {
                bc_error(bc, node, "makes boolean from %s without a cast\n", lgx_val_typeof(&e));
                return 1;
            }

            jmp_fix(bc, node, start, bc->bc_top);
            break;
        }
        case DO_STATEMENT:{
            // 禁止在逻辑操作中使用赋值表达式，无论它的返回值是否为布尔值
            bc_expr_block_assignment(bc, node->child[1]);

            unsigned start = bc->bc_top; // 循环起始位置

            if (bc_stat(bc, node->child[0])) {
                return 1;
            }

            lgx_val_t e;
            lgx_val_init(&e);
            if (bc_expr(bc, node->child[1], &e)) {
                return 1;
            }

            if (check_constant(&e, T_BOOL)) {
                if (e.v.l == 0) {
                    break;
                } else {
                    // 写入无条件跳转
                    bc_jmpi(bc, start);
                }
            } else if (check_variable(&e, T_BOOL)) {
                unsigned pos = bc->bc_top;
                bc_test(bc, &e, 0);
                bc_jmpi(bc, start);
                reg_free(bc, &e);

                bc_set_pd(bc, pos, bc->bc_top - pos - 1);
            } else {
                bc_error(bc, node, "makes boolean from %s without a cast\n", lgx_val_typeof(&e));
                return 1;
            }

            jmp_fix(bc, node, start, bc->bc_top);
            break;
        }
        case CONTINUE_STATEMENT:
            // 只能出现在循环语句中
            if (jmp_add(bc, node)) {
                // error
                break;
            }
            node->u.pos = bc->bc_top;
            bc_jmpi(bc, 0);
            break;
        case BREAK_STATEMENT:
            // 只能出现在循环语句和 switch 语句中
            if (jmp_add(bc, node)) {
                // error
                break;
            }
            node->u.pos = bc->bc_top;
            bc_jmpi(bc, 0);
            break;
        case SWITCH_STATEMENT:{
            // 执行表达式
            lgx_val_t e;
            lgx_val_init(&e);
            if (bc_expr(bc, node->child[0], &e)) {
                return 1;
            }

            if (!check_type(&e, T_LONG) && check_type(&e, T_STRING)) {
                bc_error(bc, node, "makes integer or string from %s without a cast\n", lgx_val_typeof(&e));
                return 1;
            }

            if (node->children == 1) {
                // 没有 case 和 default，什么都不用做
                reg_free(bc, &e);
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
                            bc_error(bc, child, "only constant expression allowed in case label");
                            return 1;
                        }

                        if (lgx_hash_get(condition.v.arr, &case_node.k)) {
                            bc_error(bc, child, "duplicate case value");
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

                unsigned pos = bc->bc_top;
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
                        n->v.v.l = bc->bc_top;

                        // 这里只应该出现 T_LONG 和 T_STRING
                        if (IS_GC_VALUE(&k)) {
                            lgx_val_free(&k);
                        }

                        if (bc_stat(bc, child->child[1])) {
                            return 1;
                        }
                    } else { // child->type == DEFAULT_STATEMENT
                        bc_set_pe(bc, pos, bc->bc_top);

                        if (bc_stat(bc, child->child[0])) {
                            return 1;
                        }
                    }
                }

                if (!has_default) {
                    bc_set_pe(bc, pos, bc->bc_top);
                }
            }

            jmp_fix(bc, node, 0, bc->bc_top);
            break;
        }
        case TRY_STATEMENT:{
            lgx_exception_t *exception = lgx_exception_new();

            exception->try_block.start = bc->bc_top;
            bc_stat(bc, node->child[0]);
            exception->try_block.end = bc->bc_top;

            if (exception->try_block.start == exception->try_block.end) {
                // try block 没有编译产生任何代码
                lgx_exception_delete(exception);
                break;
            } else {
                exception->try_block.end --;
            }

            int i;
            for (i = 1; i < node->children; i ++) {
                lgx_ast_node_t *n = node->child[i];

                if (n->type == CATCH_STATEMENT) {
                    lgx_exception_block_t *block = lgx_exception_block_new();

                    unsigned pos = bc->bc_top;
                    bc_jmpi(bc, 0);

                    block->start = bc->bc_top;
                    // TODO 处理参数
                    // 编译 catch block
                    bc_stat(bc, n->child[1]);
                    block->end = bc->bc_top;

                    if (block->start == block->end) {
                        // catch block 没有编译产生任何代码
                        bc_nop(bc);
                    } else {
                        block->end --;
                    }

                    bc_set_pe(bc, pos, bc->bc_top);

                    wbt_list_add_tail(&block->head, &exception->catch_blocks);

                    // 保存该 catch block 的参数
                    lgx_str_t s;
                    s.buffer = ((lgx_ast_node_token_t *)n->child[0]->child[0]->child[0])->tk_start;
                    s.length = ((lgx_ast_node_token_t *)n->child[0]->child[0]->child[0])->tk_length;
                    block->e = lgx_scope_local_val_get(n->child[1], &s);
                    assert(block->e);
                } else {
                    bc_error(bc, n, "finally block is not supported\n");
                    return 1;
                }
            }

            if (wbt_list_empty(&exception->catch_blocks)) {
                // 如果没有任何 catch block
                lgx_exception_delete(exception);
                break;
            }

            // 添加 exception
            unsigned long long int key = ((unsigned long long int)exception->try_block.start << 32) | (0XFFFFFFFF - exception->try_block.end);

            wbt_str_t k;
            k.str = (char *)&key;
            k.len = sizeof(key);

            wbt_rb_node_t *n = wbt_rb_insert(bc->exception, &k);
            assert(n);
            n->value.str = (char *)exception;
            n->value.len = sizeof(lgx_exception_t);

            break;
        }
        case THROW_STATEMENT:{
            lgx_val_t r;
            lgx_val_init(&r);

            if (bc_expr(bc, node->child[0], &r)) {
                return 1;
            }

            bc_throw(bc, &r);
            reg_free(bc, &r);
            break;
        }
        case RETURN_STATEMENT:{
            lgx_val_t r;
            lgx_val_init(&r);

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
                    bc_error(bc, node, "makes %s from %s without a cast\n", lgx_val_typeof(&ret), lgx_val_typeof(&r));
                    return 1;
                }

                if (ret.type == T_OBJECT && !lgx_obj_is_instanceof(r.v.obj, ret.v.obj)) {
                    lgx_str_t *n1 = lgx_obj_get_name(r.v.obj);
                    lgx_str_t *n2 = lgx_obj_get_name(ret.v.obj);
                    bc_error(bc, node, "makes %s<%.*s> from %s<%.*s> without a cast\n",
                        lgx_val_typeof(&r),
                        n1->length, n1->buffer,
                        lgx_val_typeof(&ret),
                        n2->length, n2->buffer
                    );
                    return 1;
                }
            } else {
                if (ret.type == T_UNDEFINED) {
                    r.u.symbol.reg_type = R_LOCAL;
                } else {
                    bc_error(bc, node, "makes %s from undefined without a cast\n", lgx_val_typeof(&ret));
                    return 1;
                }
            }

            // 写入返回指令
            if (!is_tail) {
                bc_ret(bc, &r);
            }
            reg_free(bc, &r);
            break;
        }
        case EXPRESSION_STATEMENT: {
            lgx_val_t e;
            lgx_val_init(&e);
            if (bc_expr(bc, node->child[0], &e)) {
                return 1;
            }
            reg_free(bc, &e);
            break;
        }
        // 忽略声明
        case VARIABLE_DECLARATION:
        case CONSTANT_DECLARATION:
        case TYPE_DECLARATION:
            break;
        default:
            compiler_error(bc, node, "invalid ast-node type %d\n", node->type);
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
            compiler_error(c->ast, symbol->node, "too many local variables\n");
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
            compiler_error(c->ast, symbol->node, "too many global variables\n");
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