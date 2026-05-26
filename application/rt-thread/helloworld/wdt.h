#ifndef __WDT_H__
#define __WDT_H__

#include "main.h"

#define WDT_DEVICE_NAME "wdt"
#define WDT_TIMEOUT 3      // 超时时间30秒（需>喂狗间隔）
#define WDT_FEED_INTERVAL 1 // 喂狗间隔5秒（需<超时时间）

int wdt_init(void);
void idle_hook(void);
void wdt_immediate_reset(void);

#endif /* __WDT_H__ */
