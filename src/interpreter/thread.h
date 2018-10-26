#ifndef LGX_THREAD_H
#define LGX_THREAD_H

#include <pthread.h>

#include "event.h"
#include "vm.h"

lgx_thread_t* lgx_thread_create(lgx_thread_pool_t *pool, unsigned thread_id);
lgx_thread_pool_t* lgx_thread_pool_create();

lgx_thread_t* lgx_thread_get(lgx_thread_pool_t *pool);

void lgx_thread_pool_wait(lgx_thread_pool_t *pool);

#endif // LGX_THREAD_H