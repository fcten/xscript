/* 
 * File:   wbt_list.h
 * Author: Fcten
 *
 * Created on 2015年2月15日, 上午9:24
 */

#ifndef __WBT_LIST_H__
#define	__WBT_LIST_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "../webit.h"

typedef struct wbt_list_s {  
    struct wbt_list_s *next, *prev;  
} wbt_list_t;

/**
 * 用于通过 wbt_list_t 指针获得完整结构体的指针
 * @ptr:                wbt_list_t 结构体指针
 * @type:               想要获取的完整结构体类型
 * @member:         wbt_list_t 成员在完整结构体中的名称
 */
#define wbt_list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * 用于 wbt_list_t 初始化，必须先声明再调用
 * @ptr:     wbt_list_t 结构体指针
 */
#define wbt_list_init(ptr) do { \
    (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/* 对 wbt_list_t 结构体进行遍历 */
#define wbt_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/* 获取链表的第一个元素 */
#define wbt_list_first_entry(ptr, type, member) \
	wbt_list_entry((ptr)->next, type, member)

/* 获取链表的最后一个元素 */
#define wbt_list_last_entry(ptr, type, member) \
	wbt_list_entry((ptr)->prev, type, member)

/* 获取链表的下一个元素 */
#define wbt_list_next_entry(pos, type, member) \
	wbt_list_entry((pos)->member.next, type, member)

/* 获取链表的上一个元素 */
#define wbt_list_prev_entry(pos, type, member) \
	wbt_list_entry((pos)->member.prev, type, member)

/**
 * 对 wbt_list_t 结构体进行遍历并返回完整结构体的指针
 * @pos:                用于遍历的完整结构体临时指针
 * @head:               链表头中的 wbt_list_t 指针
 * @member:         wbt_list_t 成员在完整结构体中的名称
 */
#define wbt_list_for_each_entry(pos, type, head, member)			\
	for (pos = wbt_list_first_entry(head, type, member);	\
	     &pos->member != (head);					\
	     pos = wbt_list_next_entry(pos, type, member))

static wbt_inline void __wbt_list_add(wbt_list_t *list, wbt_list_t *prev, wbt_list_t *next) {
    next->prev = list;
    list->next = next;
    list->prev = prev;
    prev->next = list;
}

static wbt_inline void __wbt_list_del(wbt_list_t *prev, wbt_list_t *next) {
    next->prev = prev;
    prev->next = next;
}
  
static wbt_inline void wbt_list_add(wbt_list_t *list, wbt_list_t *head) {
    __wbt_list_add(list, head, head->next);
}

static wbt_inline void wbt_list_add_tail(wbt_list_t *list, wbt_list_t *head) {
    __wbt_list_add(list, head->prev, head);
}  

static wbt_inline void wbt_list_del(wbt_list_t *entry) {
    __wbt_list_del(entry->prev, entry->next);
}

static wbt_inline int wbt_list_empty(const wbt_list_t *head) {
	return head->next == head;
}

static wbt_inline void wbt_list_move_tail(wbt_list_t *list, wbt_list_t *head) {
	wbt_list_del(list);
	wbt_list_add_tail(list, head);
}

#ifdef	__cplusplus
}
#endif

#endif	/* __WBT_LIST_H__ */

