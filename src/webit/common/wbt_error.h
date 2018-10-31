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

#ifdef	__cplusplus
}
#endif

#endif // __WBT_ERROR_H__