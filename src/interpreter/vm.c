#include "../common/common.h"
#include "../common/bytecode.h"
#include "../common/operator.h"
#include "../common/fun.h"
#include "../common/obj.h"
#include "../common/str.h"
#include "../common/exception.h"
#include "vm.h"
#include "gc.h"
#include "coroutine.h"

#define R(r)  (vm->regs[r])
#define C(r)  (vm->constant->table[r].v)
#define G(r)  (vm->co_main->stack.buf[r])
#define LGX_MAX_STACK_SIZE (256 << 8)

// 输出当前的调用栈
int lgx_vm_backtrace(lgx_vm_t *vm) {
    unsigned int base = vm->co_running->stack.base;
    while (1) {
        printf("  <function:%d> %d\n", vm->co_running->stack.buf[base].v.fun->addr, base);

        if (base != 0) {
            base = vm->co_running->stack.buf[base+3].v.l;
        } else {
            break;
        }
    }

    return 0;
}

void lgx_vm_throw(lgx_vm_t *vm, lgx_val_t *e) {
    assert(vm->co_running);

    unsigned pc = vm->co_running->pc - 1;

    // 匹配最接近的 try-catch 块
    unsigned long long int key = ((unsigned long long int)pc + 1) << 32;

    wbt_str_t k;
    k.str = (char *)&key;
    k.len = sizeof(key);

    lgx_exception_t *exception;
    lgx_exception_block_t *block = NULL;
    wbt_rb_node_t *n = wbt_rb_get_lesser(vm->exception, &k);
    if (n) {
        exception = (lgx_exception_t *)n->value.str;
        if (pc >= exception->try_block.start && pc <= exception->try_block.end) {
            // TODO 匹配参数类型符合的 catch block
            block = wbt_list_first_entry(&exception->catch_blocks, lgx_exception_block_t, head);
        }
    }

    if (block) {
        // 跳转到指定的 catch block
        vm->co_running->pc = block->start;

        // TODO 把异常变量写入到 catch block 的参数中，并释放原位置变量
    } else {
        // 没有匹配的 catch 块，递归向上寻找
        unsigned base = vm->co_running->stack.base;
        lgx_val_t *regs = vm->co_running->stack.buf + base;

        assert(regs[0].type == T_FUNCTION);
        assert(regs[2].type == T_LONG);
        assert(regs[3].type == T_LONG);

        if (regs[2].v.l >= 0) {
            // 释放所有局部变量和临时变量
            int n;
            for (n = 4; n < regs[0].v.fun->stack_size; n ++) {
                if (e == &regs[n]) {
                    // 不要释放抛出的异常
                    continue;
                }
                lgx_gc_ref_del(&regs[n]);
                regs[n].type = T_UNDEFINED;
            }

            // 切换执行堆栈
            vm->co_running->stack.base = regs[3].v.l;
            vm->regs = vm->co_running->stack.buf + vm->co_running->stack.base;

            // 在函数调用点重新抛出异常
            vm->co_running->pc = regs[2].v.l;
            lgx_vm_throw(vm, e);
        } else {
            // 遍历调用栈依然未能找到匹配的 catch 块，退出当前协程
            printf("[uncaught exception] ");
            lgx_val_print(e);
            printf("\n");

            lgx_vm_backtrace(vm);

            vm->co_running->pc = regs[0].v.fun->end;
        }
    }
}

// 抛出一个异常
void lgx_vm_throw_s(lgx_vm_t *vm, const char *fmt, ...) {
    char *buf = (char *)xmalloc(128);

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, 128, fmt, args);
    va_end(args);

    lgx_val_t e;
    e.type = T_STRING;
    e.v.str = lgx_str_new_ref(buf, 128);
    e.v.str->length = len;
    e.v.str->is_ref = 0;

    lgx_vm_throw(vm, &e);
}

int lgx_vm_init(lgx_vm_t *vm, lgx_bc_t *bc) {
    wbt_list_init(&vm->co_ready);
    wbt_list_init(&vm->co_suspend);
    wbt_list_init(&vm->co_died);

    wbt_list_init(&vm->heap.young);
    wbt_list_init(&vm->heap.old);
    vm->heap.young_size = 0;
    vm->heap.old_size = 0;

    vm->bc = bc;
    vm->exception = bc->exception;
    vm->constant = bc->constant;

    // 创建事件池
    vm->events = wbt_event_init();
    if (!vm->events) {
        return 1;
    }

    vm->co_count = 0;

    // 创建主协程
    vm->co_running = NULL;

    lgx_fun_t *fun = lgx_fun_new(0);
    fun->addr = 0;
    fun->end = bc->bc_top - 1;
    fun->stack_size = bc->reg->max + 1;

    vm->co_main = lgx_co_create(vm, fun);
    if (!vm->co_main) {
        return 1;
    }

    lgx_co_resume(vm, vm->co_main);

    return 0;
}

int lgx_vm_cleanup(lgx_vm_t *vm) {
    // 释放 main 函数
    //lgx_fun_delete(vm->regs[0].v.fun);

    // TODO 释放协程
    //xfree(vm->stack.buf);

    // TODO 释放消息队列

    // TODO 释放事件池

    // 清理所有变量
    wbt_list_t *list = vm->heap.young.next;
    while(list != &vm->heap.young) {
        wbt_list_t *next = list->next;
        xfree(list);
        list = next;
    }
    list = vm->heap.old.next;
    while(list != &vm->heap.old) {
        wbt_list_t *next = list->next;
        xfree(list);
        list = next;
    }

    lgx_bc_cleanup(vm->bc);

    return 0;
}

// 确保堆栈上有足够的剩余空间
int lgx_vm_checkstack(lgx_vm_t *vm, unsigned int stack_size) {
    lgx_co_stack_t *stack = &vm->co_running->stack;

    if (stack->base + R(0).v.fun->stack_size + stack_size < stack->size) {
        return 0;
    }

    unsigned int size = stack->size;
    while (stack->base + R(0).v.fun->stack_size + stack_size >= size) {
        size *= 2;
    }

    if (size > LGX_MAX_STACK_SIZE) {
        return 1;
    }

    lgx_val_t *s = (lgx_val_t *)xrealloc(stack->buf, size * sizeof(lgx_val_t));
    if (!s) {
        return 1;
    }
    // 初始化新空间
    memset(s + stack->size, 0, size - stack->size);

    stack->buf = s;
    stack->size = size;
    vm->regs = stack->buf + stack->base;

    return 0;
}

int lgx_vm_execute(lgx_vm_t *vm) {
    if (!vm->co_running) {
        return 0;
    }

    unsigned i;
    unsigned *bc = vm->bc->bc;

    for(;;) {
        i = bc[vm->co_running->pc++];

        // 单步执行
        //lgx_bc_echo(vm->co_running->pc-1, i);
        //getchar();

        switch(OP(i)) {
            case OP_MOV:{
                lgx_gc_ref_del(&R(PA(i)));
                lgx_gc_ref_add(&R(PB(i)));

                R(PA(i)).type = R(PB(i)).type;
                R(PA(i)).v = R(PB(i)).v;

                break;
            }
            case OP_MOVI:{
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_LONG;
                R(PA(i)).v.l = PD(i);
                break;
            }
            case OP_ADD:{
                lgx_gc_ref_del(&R(PA(i)));

                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l + R(PC(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d + R(PC(i)).v.d;
                } else {
                    if (lgx_op_add(&R(PA(i)), &R(PB(i)), &R(PC(i)))) {
                        lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(PB(i))), "+", lgx_val_typeof(&R(PC(i))));
                    }
                    lgx_gc_trace(vm, &R(PA(i)));
                    lgx_gc_ref_add(&R(PA(i)));
                }
                break;
            }
            case OP_SUB:{
                lgx_gc_ref_del(&R(PA(i)));

                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l - R(PC(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d - R(PC(i)).v.d;
                } else {
                    if (lgx_op_sub(&R(PA(i)), &R(PB(i)), &R(PC(i)))) {
                        lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(PB(i))), "-", lgx_val_typeof(&R(PC(i))));
                    }
                }
                break;
            }
            case OP_MUL:{
                lgx_gc_ref_del(&R(PA(i)));

                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l * R(PC(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d * R(PC(i)).v.d;
                } else {
                    if (lgx_op_mul(&R(PA(i)), &R(PB(i)), &R(PC(i)))) {
                        lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(PB(i))), "*", lgx_val_typeof(&R(PC(i))));
                    }
                }
                break;
            }
            case OP_DIV:{
                lgx_gc_ref_del(&R(PA(i)));

                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    if (UNEXPECTED(R(PC(i)).v.l == 0)) {
                        lgx_vm_throw_s(vm, "division by zero\n");
                    } else {
                        R(PA(i)).type = T_LONG;
                        R(PA(i)).v.l = R(PB(i)).v.l / R(PC(i)).v.l;
                    }
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    if (UNEXPECTED(R(PC(i)).v.d == 0)) {
                        lgx_vm_throw_s(vm, "division by zero\n");
                    } else {
                        R(PA(i)).type = T_DOUBLE;
                        R(PA(i)).v.d = R(PB(i)).v.d / R(PC(i)).v.d;
                    }
                } else {
                    if (lgx_op_div(&R(PA(i)), &R(PB(i)), &R(PC(i)))) {
                        lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(PB(i))), "/", lgx_val_typeof(&R(PC(i))));
                    }
                }
                break;
            }
            case OP_ADDI:{
                lgx_gc_ref_del(&R(PA(i)));

                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l + PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d + PC(i);
                } else {
                    lgx_val_t c;
                    c.type = T_LONG;
                    c.v.l = PC(i);
                    if (lgx_op_add(&R(PA(i)), &R(PB(i)), &c)) {
                        lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(PB(i))));
                    }
                }
                break;
            }
            case OP_SUBI:{
                lgx_gc_ref_del(&R(PA(i)));

                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l - PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d - PC(i);
                } else {
                    lgx_val_t c;
                    c.type = T_LONG;
                    c.v.l = PC(i);
                    if (lgx_op_sub(&R(PA(i)), &R(PB(i)), &c)) {
                        lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(PB(i))));
                    }
                }
                break;
            }
            case OP_MULI:{
                lgx_gc_ref_del(&R(PA(i)));

                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l * PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d * PC(i);
                } else {
                    lgx_val_t c;
                    c.type = T_LONG;
                    c.v.l = PC(i);
                    if (lgx_op_mul(&R(PA(i)), &R(PB(i)), &c)) {
                        lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(PB(i))));
                    }
                }
                break;
            }
            case OP_DIVI:{
                lgx_gc_ref_del(&R(PA(i)));

                // TODO 判断除数是否为 0
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l / PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d / PC(i);
                } else {
                    lgx_val_t c;
                    c.type = T_LONG;
                    c.v.l = PC(i);
                    if (lgx_op_add(&R(PA(i)), &R(PB(i)), &c)) {
                        lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(PB(i))));
                    }
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
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_SHL:{
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG)) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l << R(PC(i)).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(PB(i))), "<<", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_SHR:{
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG)) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l >> R(PC(i)).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(PB(i))), ">>", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_SHLI:{
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_LONG)) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l << PC(i);
                } else {
                    lgx_vm_throw_s(vm, "makes integer from %s without a cast", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_SHRI:{
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_LONG)) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l >> PC(i);
                } else {
                    lgx_vm_throw_s(vm, "makes integer from %s without a cast", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_AND:{
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG)) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l & R(PC(i)).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(PB(i))), "&", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_OR:{
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG)) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l | R(PC(i)).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(PB(i))), "|", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_XOR:{
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG)) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l ^ R(PC(i)).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(PB(i))), "^", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_NOT:{
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_LONG)) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = ~R(PB(i)).v.l;
                } else {
                    lgx_vm_throw_s(vm, "error operation: %s %s %s", lgx_val_typeof(&R(PB(i))), "~", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_EQ:{
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_BOOL;

                if (lgx_val_cmp(&R(PB(i)), &R(PC(i)))) {
                    R(PA(i)).v.l = 1;
                } else {
                    R(PA(i)).v.l = 0;
                }
                break;
            }
            case OP_LE:{
                lgx_gc_ref_del(&R(PA(i)));

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
                lgx_gc_ref_del(&R(PA(i)));

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
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_BOOL;

                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l == PC(i);
                } else {
                    R(PA(i)).v.l = 0;
                }
                break;
            }
            case OP_GEI:{
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l >= PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d >= PC(i);
                } else {
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_LEI:{
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l <= PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d <= PC(i);
                } else {
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_GTI:{
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l > PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d > PC(i);
                } else {
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_LTI:{
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l < PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d < PC(i);
                } else {
                    lgx_vm_throw_s(vm, "makes number from %s without a cast", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_LNOT:{
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_BOOL;

                if (EXPECTED(R(PB(i)).type == T_BOOL)) {
                    R(PA(i)).v.l = !R(PB(i)).v.l;
                } else {
                    lgx_vm_throw_s(vm, "makes boolean from %s without a cast", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_TEST:{
                if (EXPECTED(R(PA(i)).type == T_BOOL)) {
                    if (!R(PA(i)).v.l) {
                        vm->co_running->pc += PD(i);
                    }
                } else {
                    lgx_vm_throw_s(vm, "makes boolean from %s without a cast", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_JMP:{                
                if (R(PA(i)).type == T_LONG) {
                    vm->co_running->pc = R(PA(i)).v.l;
                } else {
                    lgx_vm_throw_s(vm, "makes integer from %s without a cast", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_JMPI:{
                vm->co_running->pc = PE(i);
                break;
            }
            case OP_CALL_NEW:{
                if (EXPECTED(R(PA(i)).type == T_FUNCTION)) {
                    // 确保空余堆栈空间足够容纳本次函数调用
                    if (UNEXPECTED(lgx_vm_checkstack(vm, R(PA(i)).v.fun->stack_size) != 0)) {
                        // runtime error
                        lgx_vm_throw_s(vm, "maximum call stack size exceeded");
                    }
                } else {
                    // runtime error
                    lgx_vm_throw_s(vm, "attempt to call a %s value, function expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_CALL_SET:{
                R(R(0).v.fun->stack_size + PA(i)) = R(PB(i));
                break;
            }
            case OP_CALL:{
                if (EXPECTED(R(PD(i)).type == T_FUNCTION)) {
                    lgx_fun_t *fun = R(PD(i)).v.fun;
                    unsigned int base = R(0).v.fun->stack_size;

                    // 写入函数信息
                    R(base + 0) = R(PD(i));

                    // 写入返回值地址
                    R(base + 1).type = T_LONG;
                    R(base + 1).v.l = PA(i);

                    // 写入返回地址
                    R(base + 2).type = T_LONG;
                    R(base + 2).v.l = vm->co_running->pc;

                    // 写入堆栈地址
                    R(base + 3).type = T_LONG;
                    R(base + 3).v.l = vm->co_running->stack.base;

                    if (fun->buildin) {
                        fun->buildin(vm);
                        // 如果触发了协程切换，则返回
                        if (vm->co_running == NULL) {
                            return 0;
                        }
                    } else {
                        // 切换执行堆栈
                        vm->co_running->stack.base += R(0).v.fun->stack_size;
                        vm->regs = vm->co_running->stack.buf + vm->co_running->stack.base;

                        // 跳转到函数入口
                        vm->co_running->pc = fun->addr;
                    }
                } else {
                    // runtime error
                    lgx_vm_throw_s(vm, "attempt to call a %s value, function expected", lgx_val_typeof(&R(PD(i))));
                }
                break;
            }
            case OP_RET:{
                // 跳转到调用点
                long long pc = R(2).v.l;

                // 判断返回值
                int ret_idx = R(1).v.l;
                int has_ret = PA(i);
                lgx_val_t ret_val;
                if (has_ret) {
                    ret_val = R(PA(i));
                    lgx_gc_ref_add(&ret_val);
                }

                // 释放所有局部变量和临时变量
                if (UNEXPECTED(pc < 0 && vm->co_running == vm->co_main)) {
                    // 主协程退出时保留堆栈，以保证全局变量可以访问
                } else {
                    int n;
                    for (n = 4; n < R(0).v.fun->stack_size; n ++) {
                        lgx_gc_ref_del(&R(n));
                        R(n).type = T_UNDEFINED;
                    }
                }

                // 切换执行堆栈
                vm->co_running->stack.base = R(3).v.l;
                vm->regs = vm->co_running->stack.buf + vm->co_running->stack.base;

                // 写入返回值
                lgx_gc_ref_del(&R(ret_idx));
                if (has_ret) {
                    R(ret_idx) = ret_val;
                } else {
                    R(ret_idx).type = T_UNDEFINED;
                }

                if (UNEXPECTED(pc < 0)) {
                    // 如果在顶层作用域 return，则终止运行
                    // 此时，寄存器 1 中保存着返回值
                    lgx_co_died(vm);
                    return 0;
                } else {
                    vm->co_running->pc = pc;
                }

                break;
            }
            case OP_ARRAY_SET:{
                if (EXPECTED(R(PA(i)).type == T_ARRAY)) {
                    if (EXPECTED(R(PB(i)).type == T_LONG || R(PB(i)).type == T_STRING)) {
                        lgx_hash_node_t n;
                        n.k = R(PB(i));
                        n.v = R(PC(i));
                        lgx_hash_set(R(PA(i)).v.arr, &n);
                        lgx_gc_ref_add(&R(PB(i)));
                        lgx_gc_ref_add(&R(PC(i)));
                    } else {
                        // runtime warning
                        lgx_vm_throw_s(vm, "attempt to set a %s key, integer or string expected", lgx_val_typeof(&R(PA(i))));
                    }
                } else {
                    // runtime error
                    lgx_vm_throw_s(vm, "attempt to set a %s value, array expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_ARRAY_ADD:{
                if (EXPECTED(R(PA(i)).type == T_ARRAY)) {
                    lgx_hash_add(R(PA(i)).v.arr, &R(PB(i)));
                    lgx_gc_ref_add(&R(PB(i)));
                } else {
                    // runtime error
                    lgx_vm_throw_s(vm, "attempt to set a %s value, array expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_ARRAY_NEW:{
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_ARRAY;
                R(PA(i)).v.arr = lgx_hash_new(0);
                if (UNEXPECTED(!R(PA(i)).v.arr)) {
                    lgx_vm_throw_s(vm, "out of memory");
                }
                lgx_gc_ref_add(&R(PA(i)));
                lgx_gc_trace(vm, &R(PA(i)));
                break;
            }
            case OP_ARRAY_GET:{
                if (EXPECTED(R(PB(i)).type == T_ARRAY)) {
                    if (EXPECTED(R(PC(i)).type == T_LONG || R(PC(i)).type == T_STRING)) {
                        lgx_gc_ref_del(&R(PA(i)));
                        lgx_hash_node_t *n = lgx_hash_get(R(PB(i)).v.arr, &R(PC(i)));
                        if (n) {
                            R(PA(i)) = n->v;
                        } else {
                            // runtime warning
                            R(PA(i)).type = T_UNDEFINED;
                        }
                        lgx_gc_ref_add(&R(PA(i)));
                    } else {
                        // runtime warning
                        lgx_vm_throw_s(vm, "attempt to index a %s key, integer or string expected", lgx_val_typeof(&R(PC(i))));
                    }
                } else {
                    lgx_vm_throw_s(vm, "attempt to index a %s value, array expected", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_LOAD:{
                lgx_gc_ref_del(&R(PA(i)));
                lgx_gc_ref_add(&C(PD(i)));
                R(PA(i)) = C(PD(i));
                break;
            }
            case OP_GLOBAL_GET:{
                lgx_gc_ref_del(&R(PA(i)));
                lgx_gc_ref_add(&G(PD(i)));
                R(PA(i)) = G(PD(i));
                break;
            }
            case OP_GLOBAL_SET:{
                lgx_gc_ref_del(&G(PD(i)));
                lgx_gc_ref_add(&R(PA(i)));
                G(PD(i)) = R(PA(i));
                break;
            }
            case OP_OBJECT_NEW:{
                if (EXPECTED(C(PD(i)).type == T_OBJECT)) {
                    lgx_gc_ref_del(&R(PA(i)));
                    R(PA(i)).type = T_OBJECT;
                    R(PA(i)).v.obj = lgx_obj_new(C(PD(i)).v.obj);
                } else {
                    lgx_vm_throw_s(vm, "attempt to new a %s value, object expected", lgx_val_typeof(&C(PD(i))));
                }
                break;
            }
            case OP_OBJECT_GET:{
                if (EXPECTED(R(PB(i)).type == T_OBJECT)) {
                    if (EXPECTED(R(PC(i)).type == T_STRING)) {
                        lgx_gc_ref_del(&R(PA(i)));
                        //lgx_obj_print(R(PB(i)).v.obj);
                        lgx_val_t *v = lgx_obj_get(R(PB(i)).v.obj, &R(PC(i)));
                        if (v) {
                            R(PA(i)) = *v;
                        } else {
                            lgx_vm_throw_s(vm, "property or method %.*s not exists", R(PC(i)).v.str->length, R(PC(i)).v.str->buffer);
                        }
                        lgx_gc_ref_add(&R(PA(i)));
                    } else {
                        lgx_vm_throw_s(vm, "attempt to index a %s value, string expected", lgx_val_typeof(&R(PC(i))));
                    }
                } else {
                    lgx_vm_throw_s(vm, "attempt to access a %s value, object expected", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_OBJECT_SET:{
                if (EXPECTED(R(PA(i)).type == T_OBJECT)) {
                    if (EXPECTED(R(PB(i)).type == T_STRING)) {
                        if (lgx_obj_set(R(PA(i)).v.obj, &R(PB(i)), &R(PC(i))) == 0) {
                            lgx_gc_ref_add(&R(PC(i)));
                        } else {
                            lgx_vm_throw_s(vm, "property %.*s not exists", R(PB(i)).v.str->length, R(PB(i)).v.str->buffer);
                        }
                    } else {
                        lgx_vm_throw_s(vm, "attempt to index a %s value, string expected", lgx_val_typeof(&R(PB(i))));
                    }
                } else {
                    lgx_vm_throw_s(vm, "attempt to access a %s value, object expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_THROW: {
                lgx_vm_throw(vm, &R(PA(i)));
                break;
            }
            case OP_NOP: break;
            case OP_HLT: {
                // 释放所有局部变量和临时变量
                int n;
                for (n = 4; n < R(0).v.fun->stack_size; n ++) {
                    lgx_gc_ref_del(&R(n));
                    R(n).type = T_UNDEFINED;
                }

                // 写入返回值
                R(1).type = T_UNDEFINED;

                return 0;
            }
            case OP_ECHO:{
                lgx_val_print(&R(PA(i)));
                break;
            }
            case OP_TYPEOF:{
                lgx_gc_ref_del(&R(PA(i)));
                lgx_op_typeof(&R(PA(i)), &R(PB(i)));
                break;
            }
            default:
                lgx_vm_throw_s(vm, "unknown op %d @ %d", OP(i), vm->co_running->pc);
        }
    }

    return 0;
}

extern wbt_atomic_t wbt_wating_to_exit;

int lgx_vm_start(lgx_vm_t *vm) {
    time_t timeout = 0;

    while (!wbt_wating_to_exit) {
        lgx_vm_execute(vm);

        if (!lgx_co_has_task(vm)) {
            // 全部协程均执行完毕
            break;
        }

        if (lgx_co_has_ready_task(vm)) {
            lgx_co_schedule(vm);
        } else {
            timeout = wbt_timer_process(&vm->events->timer);
            wbt_event_wait(vm->events, timeout);
            timeout = wbt_timer_process(&vm->events->timer);
        }
    }

    return 0;
}