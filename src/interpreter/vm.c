#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../common/bytecode.h"
#include "vm.h"

lgx_stack_t *stack_new(lgx_vm_t *vm, unsigned size) {
    lgx_stack_t *s = calloc(1, sizeof(lgx_stack_t) + size * sizeof(lgx_val_t*));
    if (!s) {
        return NULL;
    }
    s->size = size;
    lgx_list_init(&s->head);

    lgx_list_add_tail(&s->head, &vm->stack.head);

    return s;
}

lgx_stack_t *stack_get(lgx_vm_t *vm) {
    return lgx_list_last_entry(&vm->stack.head, lgx_stack_t, head);
}

void stack_delete(lgx_vm_t *vm) {
    lgx_stack_t *s = stack_get(vm);
    lgx_list_del(&s->head);
}

// TODO 使用真正的异常处理机制
void throw_exception(lgx_vm_t *vm, const char *fmt, ...) {
    printf("[runtime error: %d] ", vm->pc);

    static char buf[128];

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, 128, fmt, args);
    va_end(args);

    printf("%.*s\n", len, buf);

    //vm->pc = vm->bc_size - 1;
    exit(1);
}

int lgx_vm_init(lgx_vm_t *vm, lgx_bc_t *bc) {
    lgx_list_init(&vm->stack.head);

    lgx_stack_t *s = stack_new(vm, 256);
    if (!s) {
        return 1;
    }
    vm->regs = s->stack;

    vm->bc = bc->bc;
    vm->bc_size = bc->bc_size;
    
    vm->constant = &bc->constant;

    vm->pc = 0;

    return 0;
}

#define R(r)  (vm->regs[r])
#define C(r)  (vm->constant->table[r].k)

int lgx_vm_start(lgx_vm_t *vm) {
    unsigned i;

    for(;;) {
        i = vm->bc[vm->pc++];

        switch(OP(i)) {
            case OP_MOV:{
                R(PA(i)).type = R(PB(i)).type;
                R(PA(i)).v.l = R(PB(i)).v.l;
                break;
            }
            case OP_MOVI:{
                R(PA(i)).type = T_LONG;
                R(PA(i)).v.l = PD(i);
                break;
            }
            case OP_ADD:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l + R(PC(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d + R(PC(i)).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SUB:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l - R(PC(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d - R(PC(i)).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_MUL:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l * R(PC(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d * R(PC(i)).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_DIV:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l / R(PC(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d / R(PC(i)).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_ADDI:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l + PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d + PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SUBI:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l - PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d - PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_MULI:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l * PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d * PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_DIVI:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l / PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d / PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_NEG:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = -R(PB(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.d = -R(PB(i)).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SHL:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l << R(PC(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SHR:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l >> R(PC(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SHLI:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l << PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SHRI:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l >> PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_AND:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l & R(PC(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_OR:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l | R(PC(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_XOR:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l ^ R(PC(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_NOT:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = ~R(PB(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_EQ:{
                R(PA(i)).type = T_BOOL;

                if (R(PB(i)).type == R(PC(i)).type && R(PB(i)).v.l == R(PC(i)).v.l) {
                    R(PA(i)).v.l = 1;
                } else {
                    R(PA(i)).v.l = 0;
                }
                break;
            }
            case OP_LE:{
                R(PA(i)).type = T_BOOL;

                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l <= R(PC(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d <= R(PC(i)).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_LT:{
                R(PA(i)).type = T_BOOL;

                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l < R(PC(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d < R(PC(i)).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_EQI:{
                R(PA(i)).type = T_BOOL;

                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l == PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d == PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_GEI:{
                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l >= PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d >= PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_LEI:{
                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l <= PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d <= PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_GTI:{
                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l > PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d > PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_LTI:{
                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l < PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d < PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_LAND:{
                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_BOOL && R(PC(i)).type == T_BOOL) {
                    R(PA(i)).v.l = R(PB(i)).v.l && R(PC(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_LOR:{
                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_BOOL && R(PC(i)).type == T_BOOL) {
                    R(PA(i)).v.l = R(PB(i)).v.l || R(PC(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_LNOT:{
                R(PA(i)).type = T_BOOL;

                if (R(PB(i)).type == T_BOOL) {
                    R(PA(i)).v.l = !R(PB(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_TEST:{
                if (!R(PA(i)).v.l) {
                    vm->pc = PD(i);
                }
                break;
            }
            case OP_JMP:{                
                if (R(PA(i)).type == T_LONG) {
                    vm->pc = R(PA(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_JMPI:{
                vm->pc = PE(i);
                break;
            }
            case OP_CALL_NEW:{
                if (R(PB(i)).type == T_FUNCTION) {
                    R(PA(i)).type = T_FUNCTION;
                    // TODO 每次函数调用都创建新的执行堆栈太慢了
                    //R(PA(i)).v.fun = lgx_fun_copy(R(PB(i)).v.fun);
                    R(PA(i)).v.fun = R(PB(i)).v.fun;
                    // 初始化执行堆栈
                    if (lgx_fun_stack_init(R(PA(i)).v.fun) != 0) {
                        // runtime error
                        throw_exception(vm, "create new stack failed");
                    }
                } else {
                    // runtime error
                    throw_exception(vm, "attempt to call_new a %s value, function expected", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_CALL_SET:{
                if (R(PA(i)).type == T_FUNCTION) {
                    lgx_fun_t *fun = R(PA(i)).v.fun;
                    
                    fun->stack->stack[PB(i)] = R(PC(i));
                } else {
                    // runtime error
                    throw_exception(vm, "attempt to call_set a %s value, function expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_CALL:{
                if (R(PA(i)).type == T_FUNCTION) {
                    lgx_fun_t *fun = R(PA(i)).v.fun;

                    // 写入返回地址
                    fun->stack->ret = vm->pc;

                    // 切换执行堆栈
                    lgx_list_add_tail(&fun->stack->head, &vm->stack.head);
                    vm->regs = fun->stack->stack;

                    // 跳转到函数入口
                    vm->pc = fun->addr;
                } else {
                    // runtime error
                    throw_exception(vm, "attempt to call a %s value, function expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_CALL_END:{
                if (R(PA(i)).type == T_FUNCTION) {
                    lgx_fun_t *fun = R(PA(i)).v.fun;

                    if (fun->stack->ret_idx > 0) {
                        R(PB(i)) = fun->stack->stack[fun->stack->ret_idx];
                    } else {
                        R(PB(i)).type = T_UNDEFINED;
                    }

                    // TODO 每次函数调用都创建新的执行堆栈太慢了

                    // 释放执行堆栈
                    //free(fun->stack);
                    //fun->stack = NULL;

                    // 释放变量
                    //free(R(PA(i)).v.fun);
                    //R(PA(i)).v.fun = NULL;
                    //R(PA(i)).type = T_UNDEFINED;
                } else {
                    // runtime error
                    throw_exception(vm, "attempt to call_end a %s value, function expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_RET:{
                lgx_stack_t *s = stack_get(vm);

                // 跳转到调用点
                vm->pc = s->ret;

                // 写入返回值
                s->ret_idx = PA(i);

                // 释放栈
                stack_delete(vm);

                // 切换执行堆栈
                s = stack_get(vm);
                vm->regs = s->stack;
                break;
            }
            case OP_ARRAY_SET:{
                if (R(PA(i)).type == T_ARRAY) {
                    if (R(PB(i)).type == T_LONG) {
                        lgx_hash_node_t n;
                        n.k = R(PB(i));
                        n.v = R(PC(i));
                        lgx_hash_set(R(PA(i)).v.arr, &n);
                    } else {
                        // runtime warning
                        throw_exception(vm, "attempt to set a %s key, integer expected", lgx_val_typeof(&R(PA(i))));
                    }
                } else {
                    // runtime error
                    throw_exception(vm, "attempt to set a %s value, array expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_ARRAY_ADD:{
                if (R(PA(i)).type == T_ARRAY) {
                    lgx_hash_add(R(PA(i)).v.arr, &R(PB(i)));
                } else {
                    // runtime error
                    throw_exception(vm, "attempt to set a %s value, array expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_ARRAY_NEW:{
                R(PA(i)).type = T_ARRAY;
                R(PA(i)).v.arr = malloc(sizeof(lgx_hash_t));

                lgx_hash_init(R(PA(i)).v.arr, 32);

                break;
            }
            case OP_ARRAY_GET:{
                if (R(PB(i)).type == T_ARRAY) {
                    if (R(PC(i)).type == T_LONG) {
                        if (R(PC(i)).v.l >= 0 && R(PC(i)).v.l < R(PB(i)).v.arr->table_offset) {
                            R(PA(i)) = R(PB(i)).v.arr->table[R(PC(i)).v.l].v;
                        } else {
                            // runtime warning
                            R(PA(i)).type = T_UNDEFINED;
                        }
                    } else {
                        // runtime warning
                        throw_exception(vm, "attempt to index a %s key, integer expected", lgx_val_typeof(&R(PC(i))));
                    }
                } else {
                    throw_exception(vm, "attempt to index a %s value, array expected", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_LOAD:{
                R(PA(i)) = C(PD(i));
                break;
            }
            case OP_NOP: break;
            case OP_HLT: return 0;
            case OP_ECHO:{
                lgx_val_print(&R(PA(i)));
                printf("\n");
                break;
            }
            default:
                printf("unknown op %d @ %d\n", OP(i), vm->pc);
                return 1;
        }
    }
}