/* 
 * File:   webit.h
 * Author: Fcten
 *
 * Created on 2014年8月20日, 下午3:21
 */

#ifndef __WEBIT_H__
#define	__WEBIT_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

#ifndef WIN32

#ifdef _DEBUG
#define WBT_DEBUG
#endif

#include "os/linux/wbt_os_util.h"

#else /* WIN32 */

#ifdef _DEBUG
#define WBT_DEBUG
#endif

#include "os/win32/wbt_os_util.h"

int wbt_main( int argc, char** argv );

#endif /* WIN32 */

#define WBT_VERSION         BUILD_VERSION " (build " BUILD_TIMESTAMP ")"

#define WBT_MAX_EVENTS      512
#define WBT_EVENT_LIST_SIZE 1024
#define WBT_CONN_BACKLOG    511

#define CR      "\r"
#define LF      "\n"
#define CRLF    CR LF

#ifndef WIN32
#define WBT_USE_ACCEPT4
#endif

typedef enum {
    WBT_OK,
    WBT_ERROR,
    WBT_AGAIN
} wbt_status;

typedef int wbt_atomic_t;

#if defined(_MSC_VER)
#define JL_SIZE_T_SPECIFIER "%Iu"
#define JL_SSIZE_T_SPECIFIER "%Id"
#define JL_PTRDIFF_T_SPECIFIER "%Id"
#elif defined(__GNUC__)
#define JL_SIZE_T_SPECIFIER "%zu"
#define JL_SSIZE_T_SPECIFIER "%zd"
#define JL_PTRDIFF_T_SPECIFIER "%zd"
#else
// TODO figure out which to use.
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* __WEBIT_H__ */

