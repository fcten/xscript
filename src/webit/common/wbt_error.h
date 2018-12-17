/* 
 * File:   wbt_error.h
 * Author: Fcten
 *
 * Created on 2018年10月18日, 下午4:25
 */

#ifndef __WBT_ERROR_H__
#define	__WBT_ERROR_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "../webit.h"

wbt_status wbt_error(const char *fmt, ...);
wbt_status _wbt_debug(const char *fmt, ...);

#ifdef WBT_DEBUG
    #ifndef WIN32
        #define wbt_debug(fmt, ...) \
            _wbt_debug("%s@%d " fmt, \
                strrchr (__FILE__, '/') == 0 ?  \
                    __FILE__ : strrchr (__FILE__, '/') + 1, \
                __LINE__, \
                ##__VA_ARGS__);
    #else
        #define wbt_debug(fmt, ...) \
            _wbt_debug("%s@%d " fmt, \
                strrchr (__FILE__, '\\') == 0 ?  \
                    __FILE__ : strrchr (__FILE__, '\\') + 1, \
                __LINE__, \
                ##__VA_ARGS__);
    #endif
#else
    #define wbt_debug(fmt, ...) ((void)0);
#endif

#ifdef	__cplusplus
}
#endif

#endif // __WBT_ERROR_H__