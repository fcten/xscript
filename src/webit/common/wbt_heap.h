/* 
 * File:   wbt_heap.h
 * Author: Fcten
 *
 * Created on 2014年8月28日, 下午2:56
 */

#ifndef __WBT_HEAP_H__
#define	__WBT_HEAP_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <time.h>

#include "../webit.h"
#include "wbt_memory.h"
#include "wbt_timer.h"

typedef struct wbt_heap_s {
    wbt_timer_t **heap;
    unsigned int size;    /* 已有元素个数 */
    unsigned int max;     /* 最大元素个数 */
} wbt_heap_t;

/* 建立一个空堆 */
wbt_status wbt_heap_init(wbt_heap_t * p, size_t max_size);
/* 向堆中插入一个新元素 */
wbt_status wbt_heap_insert(wbt_heap_t * p, wbt_timer_t * node);
/* 获取堆顶元素 */
wbt_timer_t * wbt_heap_get(wbt_heap_t * p);
/* 删除堆顶元素 */
wbt_status wbt_heap_delete(wbt_heap_t * p);
/* 删除指定元素 */
wbt_status wbt_heap_remove(wbt_heap_t * p, unsigned int heap_idx);
/* 更新指定元素的位置 */
wbt_status wbt_heap_update(wbt_heap_t * p, unsigned int heap_idx);
/* 删除堆 */
wbt_status wbt_heap_destroy(wbt_heap_t * p);
/* 删除所有超时元素 */
void wbt_heap_delete_timeout(wbt_heap_t * p);

#define wbt_heap_for_each(pos, node, root) \
    for( pos=1, node=((wbt_heap_node_t *)(root)->heap.ptr)+pos ; \
         pos <= (root)->size ; \
         pos++, node=((wbt_heap_node_t *)(root)->heap.ptr)+pos )

#ifdef	__cplusplus
}
#endif

#endif	/* __WBT_HEAP_H__ */

