#include "../common/str.h"
#include "../common/fun.h"
#include "std_coroutine.h"

LGX_FUNCTION(co_yield) {
    // 在协程切换前写入返回值
    LGX_RETURN_TRUE();

    return lgx_co_yield(vm);
}

typedef struct {
    wbt_timer_t timer;
    lgx_vm_t *vm;
    lgx_co_t *co;
} co_sleep_ctx;

static wbt_status timer_wakeup(wbt_timer_t *timer) {
    co_sleep_ctx *ctx = (co_sleep_ctx *)timer;

    lgx_co_resume(ctx->vm, ctx->co);

    return WBT_OK;
}

extern time_t wbt_cur_mtime;

LGX_FUNCTION(co_sleep) {
    unsigned base = vm->regs[0].v.fun->stack_size;
    long long sleep = vm->regs[base+4].v.l;

    if (sleep > 0) {
        co_sleep_ctx *ctx = (co_sleep_ctx *)xcalloc(1, sizeof(co_sleep_ctx));
        if (!ctx) {
            return LGX_RETURN_FALSE();
        }
        ctx->timer.on_timeout = timer_wakeup;
        ctx->timer.timeout = wbt_cur_mtime + sleep;
        ctx->vm = vm;
        ctx->co = vm->co_running;

        if (wbt_timer_add(&vm->events->timer, &ctx->timer) != WBT_OK) {
            xfree(ctx);
            return LGX_RETURN_FALSE();
        }

        // 在协程切换前写入返回值
        LGX_RETURN_TRUE();
        return lgx_co_suspend(vm);
    } else {
        LGX_RETURN_TRUE();
        return lgx_co_yield(vm);
    }
}

int std_coroutine_load_symbols(lgx_hash_t *hash) {
    LGX_FUNCTION_BEGIN(co_yield, 0)
        LGX_FUNCTION_RET(T_BOOL)
    LGX_FUNCTION_END

    LGX_FUNCTION_BEGIN(co_sleep, 1)
        LGX_FUNCTION_RET(T_BOOL)
        LGX_FUNCTION_ARG(0, T_LONG)
    LGX_FUNCTION_END

    return 0;
}

lgx_buildin_ext_t ext_std_coroutine_ctx = {
    "std.coroutine",
    std_coroutine_load_symbols
};