#include "../common/str.h"
#include "../common/fun.h"
#include "../common/obj.h"
#include "redis.h"

#include <hiredis/hiredis.h>
#include <hiredis/async.h>

typedef struct {
    // 虚拟机对象
    lgx_vm_t *vm;
    // 当前协程
    lgx_co_t *co;
    // Redis 对象
    lgx_obj_t *obj;
    // Redis context
    redisAsyncContext *ctx;
} lgx_redis_t;

static void on_complete(redisAsyncContext *c, void *r, void *privdata) {
    wbt_debug("redis:on_complete");

    lgx_redis_t *redis = c->ev.data;
    lgx_vm_t *vm = redis->vm;

    // 恢复协程执行
    lgx_co_resume(vm, redis->co);

    redisReply *reply = r;
    switch (reply->type) {
    case REDIS_REPLY_ERROR:
        wbt_debug("%.*s", reply->len, reply->str);
        break;
    case REDIS_REPLY_STATUS:
        if (redis->ctx->err) {
            wbt_debug("errno %d, %.*s",
                redis->ctx->err,
                redis->ctx->errstr);
        }
        break;
    }

    // 写入返回值
    LGX_RETURN_TRUE();
}

static void on_connect(const redisAsyncContext *c, int status) {
    wbt_debug("redis:on_connect");

    lgx_redis_t *redis = c->ev.data;
    lgx_vm_t *vm = redis->vm;

    // 恢复协程执行
    lgx_co_resume(vm, redis->co);

    // 写入返回值
    LGX_RETURN_TRUE();
}

static void on_close(const redisAsyncContext *c, int status) {
    wbt_debug("redis:on_close");


}

static void ev_add_read(void *privdata) {
    lgx_redis_t *redis = (lgx_redis_t*) privdata;
    wbt_event_t *ev = redis->ctx->data;

    // 修改事件
    ev->events |= WBT_EV_READ;

    if (wbt_event_mod(redis->vm->events, ev) != WBT_OK) {
        //on_close(ev);
    }
}

static void ev_del_read(void *privdata) {
    lgx_redis_t *redis = (lgx_redis_t*) privdata;
    wbt_event_t *ev = redis->ctx->data;

    // 修改事件
    ev->events &= ~WBT_EV_READ;

    if (wbt_event_mod(redis->vm->events, ev) != WBT_OK) {
        //on_close(ev);
    }
}

static void ev_add_write(void *privdata) {
    lgx_redis_t *redis = (lgx_redis_t*) privdata;
    wbt_event_t *ev = redis->ctx->data;

    // 修改事件
    ev->events |= WBT_EV_WRITE;

    if (wbt_event_mod(redis->vm->events, ev) != WBT_OK) {
        //on_close(ev);
    }
}

static void ev_del_write(void *privdata) {
    lgx_redis_t *redis = (lgx_redis_t*) privdata;
    wbt_event_t *ev = redis->ctx->data;

    // 修改事件
    ev->events &= ~WBT_EV_WRITE;

    if (wbt_event_mod(redis->vm->events, ev) != WBT_OK) {
        //on_close(ev);
    }
}

static void ev_cleanup(void *privdata) {
    lgx_redis_t *redis = (lgx_redis_t*) privdata;
    wbt_event_t *ev = redis->ctx->data;

    // 修改事件
    if (wbt_event_del(redis->vm->events, ev) != WBT_OK) {
        //on_close(ev);
    }
}

static wbt_status on_read(wbt_event_t *ev) {
    lgx_redis_t *redis = ev->ctx;

    redisAsyncHandleRead(redis->ctx);

    return WBT_OK;
}

static wbt_status on_write(wbt_event_t *ev) {
    lgx_redis_t *redis = ev->ctx;

    redisAsyncHandleWrite(redis->ctx);

    return WBT_OK;
}

LGX_METHOD(Redis, connect) {
    LGX_METHOD_ARGS_INIT();
    LGX_METHOD_ARGS_THIS(obj);
    LGX_METHOD_ARGS_GET(ip, 0);
    LGX_METHOD_ARGS_GET(port, 1);

    if (!ip || ip->type != T_STRING || ip->v.str->length > 32) {
        lgx_vm_throw_s(vm, "invalid param `ip`");
        return 1;
    }

    if (!port || port->type != T_LONG || port->v.l <= 0 || port->v.l >= 65535) {
        lgx_vm_throw_s(vm, "invalid param `port`");
        return 1;
    }

    char ip_str[64];
    memcpy(ip_str, ip->v.str->buffer, ip->v.str->length);
    ip_str[ip->v.str->length] = '\0';

    wbt_debug("redis: try to connect %s:%d", ip_str, port->v.l);

    redisAsyncContext *c = redisAsyncConnect(ip_str, (int)port->v.l);
    if (!c) {
        lgx_vm_throw_s(vm, "redisAsyncConnect() failed");
        return 1;
    }

    if (c->err) {
        redisAsyncFree(c);
        lgx_vm_throw_s(vm, c->errstr);
        return 1;
    }

    redisAsyncSetConnectCallback(c, on_connect);
    redisAsyncSetDisconnectCallback(c, on_close);

    lgx_redis_t *redis = xcalloc(1, sizeof(lgx_redis_t));
    if (!redis) {
        redisAsyncFree(c);
        lgx_vm_throw_s(vm, "xcalloc() failed");
        return 1;
    }

    redis->vm = vm;
    redis->co = vm->co_running;
    redis->obj = obj->v.obj;
    redis->ctx = c;

    // 保存 lgx_redis_t
    lgx_hash_node_t symbol;
    symbol.k.type = T_STRING;
    symbol.k.v.str = lgx_str_new_ref("ctx", sizeof("ctx") - 1);
    symbol.v.type = T_RESOURCE;
    symbol.v.v.l = (long long)(redis);
    if (lgx_obj_add_property(obj->v.obj, &symbol)) {
        return 1;
    }

    // 添加事件
    wbt_event_t *p_ev, tmp_ev;
    tmp_ev.timer.on_timeout = NULL;
    tmp_ev.timer.timeout = 0;
    tmp_ev.on_recv = on_read;
    tmp_ev.on_send = on_write;
    tmp_ev.events  = WBT_EV_WRITE | WBT_EV_ET;
    tmp_ev.fd      = c->c.fd;

    if((p_ev = wbt_event_add(vm->events, &tmp_ev)) == NULL) {
        xfree(redis);
        redisAsyncFree(c);
        lgx_vm_throw_s(vm, "add event faild");
        return 1;
    }

    p_ev->ctx = redis;

    c->ev.addRead = ev_add_read;
    c->ev.delRead = ev_del_read;
    c->ev.addWrite = ev_add_write;
    c->ev.delWrite = ev_del_write;
    c->ev.cleanup = ev_cleanup;
    c->ev.data = redis;
    c->data = p_ev;

    LGX_RETURN_TRUE();

    lgx_co_suspend(vm);

    return 0;
}

LGX_METHOD(Redis, set) {
    LGX_METHOD_ARGS_INIT();
    LGX_METHOD_ARGS_THIS(obj);
    LGX_METHOD_ARGS_GET(key, 0);
    LGX_METHOD_ARGS_GET(value, 1);

    if (!key || key->type != T_STRING) {
        lgx_vm_throw_s(vm, "invalid param `key`");
        return 1;
    }

    if (!value || value->type != T_STRING) {
        lgx_vm_throw_s(vm, "invalid param `value`");
        return 1;
    }

    lgx_val_t *res = lgx_obj_get_s(obj->v.obj, "ctx");
    if (!res || res->type != T_RESOURCE) {
        lgx_vm_throw_s(vm, "invalid property `ctx`");
        return 1;
    }

    lgx_redis_t *redis = (lgx_redis_t *)res->v.res;

    if (redisAsyncCommand(redis->ctx, on_complete, NULL, "SET %b %b",
        key->v.str->buffer, key->v.str->length,
        value->v.str->buffer, value->v.str->length) != REDIS_OK) {
        lgx_vm_throw_s(vm, "redisAsyncCommand() failed");
        return 1;
    }

    LGX_RETURN_TRUE();

    lgx_co_suspend(vm);

    return 0;
}

LGX_CLASS(Redis) {
    LGX_METHOD_BEGIN(Redis, connect, 2)
        LGX_METHOD_RET(T_BOOL)
        LGX_METHOD_ARG(0, T_STRING)
        LGX_METHOD_ARG_OPTIONAL(1, T_LONG, 6379)
        LGX_METHOD_ACCESS(P_PUBLIC)
    LGX_METHOD_END

    LGX_METHOD_BEGIN(Redis, set, 2)
        LGX_METHOD_RET(T_BOOL)
        LGX_METHOD_ARG(0, T_STRING)
        LGX_METHOD_ARG(1, T_STRING)
        LGX_METHOD_ACCESS(P_PUBLIC)
    LGX_METHOD_END

    return 0;
}

int redis_load_symbols(lgx_hash_t *hash) {
    LGX_CLASS_INIT(Redis);

    return 0;
}

lgx_buildin_ext_t ext_redis_ctx = {
    "redis",
    redis_load_symbols
};