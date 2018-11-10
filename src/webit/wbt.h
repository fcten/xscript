#ifndef __WBT_H__
#define	__WBT_H__

#ifdef	__cplusplus
extern "C" {
#endif

int wbt_init();

#include "webit.h"
#include "common/wbt_error.h"
#include "common/wbt_timer.h"
#include "event/wbt_event.h"
#include "thread/wbt_thread.h"

#ifdef	__cplusplus
}
#endif

#endif	/* __WEBIT_H__ */

