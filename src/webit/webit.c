/* 
 * File:   webit.c
 * Author: Fcten
 *
 * Created on 2014年8月20日, 下午2:10
 */

#include <stdio.h>
#include <stdlib.h>

#include "webit.h"
#include "common/wbt_time.h"
#include "common/wbt_timer.h"
#include "common/wbt_string.h"
#include "common/wbt_memory.h"
#include "common/wbt_error.h"
#include "event/wbt_event.h"

wbt_atomic_t wbt_wating_to_exit = 0;

#ifndef WIN32

void wbt_signal(int signo, siginfo_t *info, void *context) {
    switch(signo) {
        case SIGTERM:
            /* 停止事件循环并退出 */
            wbt_wating_to_exit = 1;
            break;
        case SIGINT:
            /* 仅调试模式，退出 */
            wbt_wating_to_exit = 1;
            break;
        case SIGPIPE:
            /* 对一个已经收到FIN包的socket调用write方法时, 如果发送缓冲没问题, 
             * 会返回正确写入(发送). 但发送的报文会导致对端发送RST报文, 因为对端的socket
             * 已经调用了close, 完全关闭, 既不发送, 也不接收数据. 所以, 第二次调用write方法
             * (假设在收到RST之后), 会生成SIGPIPE信号 */
            break;
        case SIGHUP:
            /* reload 信号 */
            break;
        case SIGUSR1:
            /* 自定义信号 */
            break;
        case SIGUSR2:
            /* 自定义信号 */
            break;
    }
}

#endif

int wbt_init() {
#ifdef WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) {
		return 1;
	}
#endif

    if (wbt_time_update() != WBT_OK) {
        return WBT_ERROR;
    }

#ifndef WIN32
    /* 设置需要监听的信号(后台模式) */
    struct sigaction act;
    sigset_t set;

    act.sa_sigaction = wbt_signal;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;

    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGPIPE);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);

    if (sigprocmask(SIG_UNBLOCK, &set, NULL) == -1) {
        wbt_error("sigprocmask() failed\n");
    }

    sigaction(SIGCHLD, &act, NULL); /* 退出信号 */
    sigaction(SIGTERM, &act, NULL); /* 退出信号 */
    sigaction(SIGPIPE, &act, NULL); /* 忽略 */
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);
#endif

    return 0;
}
