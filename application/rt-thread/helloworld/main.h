#ifndef MAIN_H_
#define MAIN_H_

#include <drivers/pin.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <aic_drv_wdt.h>

#include "lvgl.h"
#include "gui_guider.h"

#include "../../packages/artinchip/mpp/middle_media/base/include/aic_audio_render_manager.h"
#include "mini_audio_player.h"
#include "mpp_fb.h"
#include "aic_player.h"
#include "aic_ui.h"

#include "my_uart.h"
#include "video.h"
#include "cfgsave.h"
#include "music.h"
#include "wdt.h"

#define CONN(x, y) x#y
#define LVGL_DIRR "L:" LVGL_STORAGE_PATH "/"
#define LVGL_MUSIC_PATH LVGL_STORAGE_PATH"/music/"
#define MUSIC_PATH(y) CONN(LVGL_MUSIC_PATH, y)

#define POWER_ON_MODE 0      //0 直接跳转， 1等待点击跳转

#define VERSION_NUMBER "3KMLEP-JYMT300-100277"
#define VERSION_DATE "2026-6-25"

#define LANGUAGE_CN 0
#define LANGUAGE_EN 1

typedef struct
{
    char mac[20];
    char name[32];
} ble_item_t;


typedef enum{
    MUSIC_NONE= 0,
    MUSIC_SUN= 1,
    MUSIC_SUN_EN,
    MUSIC_BLUE,
    MUSIC_BLUE_EN,
    MUSIC_START,
    MUSIC_START_EN,
    MUSIC_RENEW,
    MUSIC_RENEW_EN,
}MUSIC_T;

typedef enum{
    TIMEOUT_MODE_30sec = 1,
    TIMEOUT_MODE_1min = 2,
    TIMEOUT_MODE_5min = 10,
}TIMEOUT_MODE_T;

/*RT_THREAD 变量*/
extern rt_mutex_t timeout_mutex;
extern rt_mutex_t goto_mutex;
extern rt_mutex_t cfg_mutex;
/*RT_THREAD 变量*/

extern lv_timer_t *blue_timer;
extern lv_timer_t *arc_timer;
extern lv_timer_t *blue_state_timer;

extern ble_item_t g_ble_queue;

extern volatile uint8_t language;
extern volatile uint8_t music_select;
extern volatile uint8_t light_value;
extern volatile uint8_t screen_off_mode;
extern volatile uint16_t timeout_cnt;


extern volatile char ble_mac[32];
extern volatile char ble_found_name[20];
extern volatile bool power_connect_flag;            // 保存数据标志
extern volatile bool goto_home_flag;            // 保存数据标志
extern volatile bool save_flag;            // 保存数据标志

extern volatile bool blue_show;
extern volatile bool goto_blue;
extern volatile bool g_ble_valid;
extern volatile bool timeout_state;

extern volatile bool home_open_state;

extern volatile bool video_play_connected;
extern volatile bool ble_con_begin;
extern volatile bool g_ble_connected;
extern volatile bool home_blue_state;

extern uint32_t gpio_fan_pin;


void backlight_set(uint8_t level);
void backlight_off(void);

#endif /* MAIN_H_ */
