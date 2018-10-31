/* 
 * File:   wbt_event_epoll.h
 * Author: fcten
 *
 * Created on 2016年4月7日, 下午2:59
 */

#ifndef WBT_EVENT_EPOLL_H
#define	WBT_EVENT_EPOLL_H

#ifdef	__cplusplus
extern "C" {
#endif

#define WBT_EV_READ     (EPOLLIN  | EPOLLRDHUP)
#define WBT_EV_WRITE    EPOLLOUT

#define WBT_EV_ET       EPOLLET

#ifdef	__cplusplus
}
#endif

#endif	/* WBT_EVENT_EPOLL_H */

