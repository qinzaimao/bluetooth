/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-10-18     Meco Man     The first version
 * 2021-12-17     Wayne        Add input event
 * 2023-05-02     Ning Fang    Add ArtInChip input event
 */

#include <rtconfig.h>
#ifdef KERNEL_RTTHREAD
#include <lvgl.h>
#include <stdbool.h>
#include <rtthread.h>
#include <../components/drivers/include/drivers/touch.h>
#include "D:\new_SDK\bluetouch\luban-lite-master\application\rt-thread\helloworld\main.h"

static lv_indev_state_t last_state = LV_INDEV_STATE_REL;
static rt_int16_t last_x = 0;
static rt_int16_t last_y = 0;

static void input_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    data->point.x = last_x;
    data->point.y = last_y;
    data->state = last_state;
}

void aic_touch_inputevent_cb(rt_int16_t x, rt_int16_t y, rt_uint8_t state)
{
    static bool power_indev = false;
    switch (state)
    {
    case RT_TOUCH_EVENT_UP:
        if(power_indev) power_indev = false;
        last_state = LV_INDEV_STATE_RELEASED;
        break;
    case RT_TOUCH_EVENT_MOVE:
    case RT_TOUCH_EVENT_DOWN:
        timeout_cnt = 0;
        rt_mutex_take(timeout_mutex, RT_WAITING_FOREVER);
        if(timeout_state)
        {
            timeout_state = false; // 触摸事件，退出息屏状态
            rt_mutex_release(timeout_mutex);
            power_indev  = true;
            break;
        }
        rt_mutex_release(timeout_mutex);
        if(power_indev) break;
        rt_mutex_take(goto_mutex, RT_WAITING_FOREVER);
        bool power_connect_temp = power_connect_flag;
        rt_mutex_release(goto_mutex);
        if(power_connect_temp)
        {
            rt_mutex_take(goto_mutex, RT_WAITING_FOREVER);
            goto_home_flag = true;
            power_connect_flag = false;
            rt_mutex_release(goto_mutex);
        }

        last_x = x;
        last_y = y;
        last_state = LV_INDEV_STATE_PRESSED;
        break;
#ifdef AIC_MONKEY_TEST
    case RT_TOUCH_MONKEY_TEST:
        last_x = x;
        last_y = y;
        last_state = LV_INDEV_STATE_PRESSED;
        break;
#endif
    }
}

void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;

    /* Basic initialization */
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = input_read;

    /* Register the driver in LVGL and save the created input device object */
    lv_indev_drv_register(&indev_drv);
}
#endif
