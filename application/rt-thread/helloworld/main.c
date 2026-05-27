/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: weilin.peng@artinchip.com
 */

#include <rtthread.h>
#ifdef RT_USING_ULOG
#include <ulog.h>
#endif
#include "main.h"

/*RT_THREAD 变量*/
rt_mutex_t timeout_mutex = NULL;
rt_mutex_t goto_mutex = NULL;
 rt_mutex_t cfg_mutex = NULL;
/*RT_THREAD 变量*/
//
volatile bool power_connect_flag = false;            // 保存数据标志
volatile bool goto_home_flag = false;            // 保存数据标志

volatile bool save_flag = false;            // 保存数据标志
volatile bool video_play_connected = false;
volatile bool ble_con_begin = false;
volatile bool g_ble_connected = false;
volatile bool blue_show = false;
volatile bool goto_blue = false;

volatile bool g_ble_valid = false;

volatile bool home_open_state = false;   // 首页开关状态
volatile bool home_blue_state = false;
volatile bool timeout_state = false;

ble_item_t g_ble_queue;

volatile uint8_t music_select = MUSIC_NONE;
volatile uint8_t language = LANGUAGE_CN;
volatile uint8_t light_value = 80;
volatile uint8_t screen_off_mode = TIMEOUT_MODE_30sec;

volatile uint16_t timeout_cnt = 0;

uint32_t gpio_fan_pin = 0;

volatile char ble_mac[32] = {0};
volatile char ble_found_name[20] = {0};
// volatile char name[32];

void BLEN_Init(void)
{
    backlight_set(light_value);
}

void backlight_set(uint8_t level)
{
#if defined(KERNEL_RTTHREAD) && defined(AIC_PWM_BACKLIGHT_CHANNEL)
    struct rt_device_pwm *pwm_dev;

    if (level >= 95)
        level = 90;
    if (level <= 5)
        level = 5;

    pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");
    if (pwm_dev == NULL)
    {
        rt_kprintf("backlight_set: find pwm device failed!\n");
        return; // 设备不存在，直接返回，避免空指针
    }
    /* pwm frequency: 1KHz = 1000000ns */
    /* pwm frequency: 10KHz = 100000ns */
    /* pwm frequency: 50KHz = 20000ns */
    /* pwm frequency: 100KHz = 10000ns */
    rt_uint32_t period_ns = 20000;                    // 50KHz
    rt_uint32_t pulse_ns = (period_ns * level) / 100; // 直接按百分比计算占空比（更直观）

    if (rt_pwm_set(pwm_dev, AIC_PWM_BACKLIGHT_CHANNEL, period_ns, pulse_ns) != RT_EOK)
    {
        rt_kprintf("backlight_set: set pwm failed!\n");
        return;
    }
    rt_pwm_enable(pwm_dev, AIC_PWM_BACKLIGHT_CHANNEL);
#endif
}

void backlight_off(void)
{
#if defined(KERNEL_RTTHREAD) && defined(AIC_PWM_BACKLIGHT_CHANNEL)
    struct rt_device_pwm *pwm_dev;


    pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");
    if (pwm_dev == NULL)
    {
        rt_kprintf("backlight_off: find pwm device failed!\n");
        return; // 设备不存在，直接返回，避免空指针
    }
    /* pwm frequency: 1KHz = 1000000ns */
    /* pwm frequency: 10KHz = 100000ns */
    /* pwm frequency: 50KHz = 20000ns */
    /* pwm frequency: 100KHz = 10000ns */
    rt_uint32_t period_ns = 20000;                    // 50KHz
    rt_uint32_t pulse_ns = (period_ns * 0) / 100; // 直接按百分比计算占空比（更直观）

    if (rt_pwm_set(pwm_dev, AIC_PWM_BACKLIGHT_CHANNEL, period_ns, pulse_ns) != RT_EOK)
    {
        rt_kprintf("backlight_set: set pwm failed!\n");
        return;
    }

    rt_pwm_disable(pwm_dev, AIC_PWM_BACKLIGHT_CHANNEL);
#endif
}

static rt_thread_t uart_thread = RT_NULL;
static rt_thread_t video_thread = RT_NULL;
static rt_thread_t cfgsave_thread = RT_NULL;
static rt_thread_t music_thread = RT_NULL;

int main(void)
{
#ifdef ULOG_USING_FILTER
    ulog_global_filter_lvl_set(ULOG_OUTPUT_LVL);
#endif
    cfg_mutex = rt_mutex_create("cfg_mutex", RT_IPC_FLAG_FIFO);
    if (cfg_mutex == NULL)
    {
        rt_kprintf("cfg_mutex create failed\n");
        return;
    }
    goto_mutex = rt_mutex_create("goto_mutex", RT_IPC_FLAG_FIFO);
    if (goto_mutex == NULL)
    {
        rt_kprintf("goto_mutex create failed\n");
        return;
    }
    timeout_mutex = rt_mutex_create("timeout_mutex", RT_IPC_FLAG_FIFO);
    if (timeout_mutex == NULL)
    {
        rt_kprintf("timeout_mutex create failed\n");
        return;
    }
    cfgRead();
    music_select = (language == LANGUAGE_CN ? MUSIC_SUN : MUSIC_SUN_EN);
    gpio_fan_pin = rt_pin_get("PB.6");
    rt_pin_mode(gpio_fan_pin, PIN_MODE_OUTPUT);
    rt_pin_write(gpio_fan_pin, PIN_LOW);



    uart_thread = rt_thread_create("uart",            // 线程名字
                                   uart_thread_entry, // 线程入口函数
                                   RT_NULL,           // 线程入口参数
                                   1024 * 4,          // 线程堆栈大小
                                   16,                // 线程优先级
                                   20);
    video_thread = rt_thread_create("video",            // 线程名字
                                    video_thread_entry, // 线程入口函数
                                    RT_NULL,            // 线程入口参数
                                    1024 * 10,          // 线程堆栈大小
                                    17,                 // 线程优先级
                                    20);
    cfgsave_thread = rt_thread_create("cfgsave",            // 线程名字
                                      cfgsave_thread_entry, // 线程入口函数
                                      RT_NULL,              // 线程入口参数
                                      1024 * 8,             // 线程堆栈大小
                                      20,                   // 线程优先级
                                      20);
    music_thread = rt_thread_create("music",            // 线程名字
                                      music_thread_entry, // 线程入口函数
                                      RT_NULL,              // 线程入口参数
                                      1024 * 8,             // 线程堆栈大小
                                      20,                   // 线程优先级
                                      20);

    if (uart_thread != RT_NULL)    rt_thread_startup(uart_thread);
    if (cfgsave_thread != RT_NULL) rt_thread_startup(cfgsave_thread);
    if (music_thread != RT_NULL) rt_thread_startup(music_thread);

    if (wdt_init() != RT_EOK)
        rt_kprintf("[WDT] Init failed, system halted!");
    else
        rt_kprintf("wDT init success\n");

    rt_thread_mdelay(1000);
    BLEN_Init();
    rt_kprintf("[MAIN] 开始配置蓝牙...\n");
    char temp[40] = {0};
    // 设置主机名
    strcpy(temp, "AT+NAME:main\r\n");
    uart_send_data((uint8_t *)temp, strlen(temp));
    rt_thread_mdelay(200);

    strcpy(temp, "AT+RESET:1\r\n");
    uart_send_data((uint8_t *)temp, strlen(temp));
    rt_thread_mdelay(1000); // 重启需要久一点

    strcpy(temp, "AT+SCANR:-90\r\n");
    uart_send_data((uint8_t *)temp, strlen(temp));
    rt_thread_mdelay(200); // 重启需要久一点

    strcpy(temp, "AT+SCANN:\r\n");
    uart_send_data((uint8_t *)temp, strlen(temp));
    rt_thread_mdelay(200); // 重启需要久一点
    rt_kprintf("[MAIN] 蓝牙配置完成！\n");
    return 0;
}
