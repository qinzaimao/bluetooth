#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"
#include "my_uart.h"
#include <string.h>
#include "D:\new_SDK\bluetouch\luban-lite-master\application\rt-thread\helloworld\main.h"

// #define BLUETOOTH_MAX_ITEMS 10

// typedef struct {
//     char mac[20];
//     char name[32];
// } ble_device_t;

// ble_device_t g_ble_list[BLUETOOTH_MAX_ITEMS];
// int g_ble_count = 0;

// static lv_obj_t *ble_list = NULL;
// static lv_timer_t *blue_timer = NULL;

// // 全局标志：是否点击过（点击后就停止扫描）
// static bool g_ble_clicked = false;

// // ===================== 【重连机制】 =====================
// static char  g_reconnect_mac[20]  = {0};    // 保存要重连的MAC
// static bool  g_need_reconnect     = false; // 是否需要重连
// static int   g_retry_count        = 0;     // 重试次数

// typedef struct {
//     char mac[20];
//     char name[32];
// } ble_item_t;

// extern ble_item_t g_ble_queue[];
// extern int g_ble_queue_cnt;

// // 发送连接指令
// void ble_send_connect(const char *mac)
// {
//     char cmd[40];
//     sprintf(cmd, "AT+CONNI:%s\r\n", mac);
//     uart_send_at(cmd);
//     rt_kprintf("[蓝牙] 正在连接: %s (重试:%d)\n", mac, g_retry_count);
// }

// // 点击就发连接，并且永久停止扫描
// static void ble_btn_click_cb(lv_event_t *e)
// {
//     lv_obj_t *btn = lv_event_get_target(e);
//     const char *mac = lv_obj_get_user_data(btn);

//     if (mac && *mac)
//     {
//         rt_kprintf("[点击] 连接: %s\n", mac);

//         // 只要点过一次，就永久停止扫描
//         g_ble_clicked = true;
//         ble_con_begin = true;
//         // 保存MAC，开启重连
//         strcpy(g_reconnect_mac, mac);
//         g_need_reconnect = true;
//         g_retry_count = 0;

//         // 发送第一次连接
//         ble_send_connect(mac);
//     }
// }
// void add_ble_to_list(const char *mac, const char *name)
// {
//     // 已经点击过，不再添加新设备
//     if (g_ble_clicked)
//         return;

//     // 🔴 关键：没有名字的设备，直接跳过，不添加
//     if (name == NULL || *name == '\0')
//         return;

//     // 检查是否已经存在该设备
//     for (int i = 0; i < g_ble_count; i++) {
//         if (!strcmp(g_ble_list[i].mac, mac))
//             return;
//     }

//     // 列表满了，不再添加
//     if (g_ble_count >= BLUETOOTH_MAX_ITEMS)
//         return;

//     // 保存设备信息
//     strcpy(g_ble_list[g_ble_count].mac, mac);
//     strcpy(g_ble_list[g_ble_count].name, name);
//     g_ble_count++;

//     // 只显示名字（你要求：只要名字）
//     char buf[20];
//     snprintf(buf, sizeof(buf), "%s", name);

//     // 添加到UI列表
//     lv_obj_t *btn = lv_list_add_btn(ble_list, NULL, buf);
//     lv_obj_set_style_text_font(btn, &lv_font_Dengb_24, LV_PART_MAIN);
//     lv_obj_set_user_data(btn, g_ble_list[g_ble_count - 1].mac);
//     lv_obj_add_event_cb(btn, ble_btn_click_cb, LV_EVENT_CLICKED, NULL);
// }
// // 定时器：没点击就扫描，点击后彻底停止 + 重连
// static void blue_callback(lv_timer_t *timer)
// {
//     // 已连接 → 停止定时器，跳主页
//     if(g_ble_connected)
//     {
//         lv_timer_pause(timer);
//         g_need_reconnect = false; // 停止重连
//         setup_scr_screen(&guider_ui);
//         lv_scr_load_anim(guider_ui.screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
//         return;
//     }

//     // ===================== 【连接失败自动重试】 =====================
//     if (g_need_reconnect && !g_ble_connected)
//     {
//         g_retry_count++;
//         ble_send_connect(g_reconnect_mac);
//         return;
//     }

//     // 已经点击过 → 不扫描
//     if (g_ble_clicked)
//         return;

//     // 加载扫描到的设备
//     while (g_ble_queue_cnt > 0)
//     {
//         add_ble_to_list(g_ble_queue[0].mac, g_ble_queue[0].name);
//         for (int i = 0; i < g_ble_queue_cnt - 1; i++) {
//             g_ble_queue[i] = g_ble_queue[i + 1];
//         }
//         g_ble_queue_cnt--;
//     }

//     // 发送扫描
//     uart_send_at("AT+SCANI\r\n");
// }

// 页面初始化
void setup_scr_screen_blue(lv_ui *ui)
{
    ui->screen_blue = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_blue, 480, 480);
    lv_obj_set_scrollbar_mode(ui->screen_blue, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(ui->screen_blue, 255, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui->screen_blue, lv_color_hex(0xffffff), LV_PART_MAIN);

    // ble_list = lv_list_create(ui->screen_blue);
    // lv_obj_set_size(ble_list, 440, 400);
    // lv_obj_align(ble_list, LV_ALIGN_CENTER, 0, 0);
    // lv_list_add_btn(ble_list, NULL, "wait blue...");
    // lv_obj_set_style_text_font(ble_list, &lv_font_Dengb_24, LV_PART_MAIN);

    // g_ble_clicked = false;
    // g_need_reconnect = false;
    // g_reconnect_mac[0] = 0;

    // if (blue_timer == NULL)
    //     blue_timer = lv_timer_create(blue_callback, 3000, 0);
    // else
    //     lv_timer_resume(blue_timer);

    lv_obj_update_layout(ui->screen_blue);
}
