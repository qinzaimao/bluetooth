/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"
#include "D:\new_SDK\bluetouch\luban-lite-master\application\rt-thread\helloworld\main.h"

// ===================== 全局变量 =====================
lv_timer_t *blue_timer = NULL;
lv_timer_t *arc_timer = NULL;


// 蓝牙状态
static char  g_reconnect_mac[20] = {0};
static bool  g_need_reconnect    = false;
static int   g_retry_count       = 0;


static void arc_callback(lv_timer_t *timer)
{

    static uint16_t angle = 0;
    static uint16_t opa = 0;
    static uint16_t knob = 0;
    static bool state = false;
    static bool moving_forward = true;  // 移动方向标志

    if(!goto_blue) return;

    angle ++;
    if(opa <= 240 && !state)
    {
        opa += 4;
    } else state = true;
    if(opa > 50 && state)
    {
        opa -= 4;
    } else state = false;
    if(angle >= 360) angle = 0;
    if(moving_forward) {
         knob += 4;  // 前进速度
         if(knob >= 300) {  // 达到最大角度
             moving_forward = false;
         }
     } else {
         knob -= 4;  // 后退速度
         if(knob <= 0) {   // 回到起始角度
             moving_forward = true;
         }
     }
    // 保留旋转并让knob跟随移动
    lv_arc_set_rotation(guider_ui.screen_system_set_arc_blue_1, angle);
    lv_arc_set_value(guider_ui.screen_system_set_arc_blue_1, knob);  // knob跟随旋转
    lv_obj_set_style_arc_opa(guider_ui.screen_system_set_arc_blue_1, opa, LV_PART_MAIN|LV_STATE_DEFAULT);
}

// 标记是否有有效设备（1=有，0=无）
void ble_send_connect(const char *mac)
{
    char cmd[40];
    sprintf(cmd, "AT+CONNI:%s\r\n", mac);
    uart_send_at(cmd);
    rt_kprintf("[蓝牙] 正在连接: %s (重试:%d)\n", mac, g_retry_count);
}

// ===================== 蓝牙扫描/显示/重连 定时器 =====================
static void blue_callback(lv_timer_t *timer)
{
    static int scan_cnt = 0;
    if(!goto_blue) return;
    // 已连接 → 跳转主页
    if (g_ble_connected)
    {
        rt_kprintf("[蓝牙] 连接成功\n");
        g_retry_count = 0;
        goto_blue = false;
        lv_timer_pause(blue_timer);
        lv_timer_pause(arc_timer);
        setup_scr_screen_home(&guider_ui);
        lv_scr_load_anim(guider_ui.screen_home, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
        g_need_reconnect = false;
        return;
    }

    // 自动重连
    if (g_need_reconnect && !g_ble_connected)
    {
        g_retry_count++;
        ble_send_connect(g_reconnect_mac);
        return;
    }


    // ===================== 显示设备 =====================
    if (g_ble_valid)
    {
        char buf[20] = {0};
        sprintf(buf, "       %s", g_ble_queue.name);
        lv_label_set_text(guider_ui.screen_system_set_label_blue_4, buf);
    }
    else
    {
        lv_label_set_text(guider_ui.screen_system_set_label_blue_4, "");
    }
    scan_cnt++;
    if(scan_cnt >= 2) // 每两秒发送一次扫描指令
    {
        scan_cnt = 0;
        uart_send_at("AT+SCANI\r\n");
    }
}

// ===================== 连接按钮点击 =====================
static void blue_connect_btn_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        if(g_ble_connected) return;
        g_need_reconnect = true;   // 开启重连

        // 保存当前设备MAC
        if (g_ble_valid)
        {
            strncpy(g_reconnect_mac, g_ble_queue.mac, 19);
        }

        uart_send_at("AT+CONN\r\n");
        rt_kprintf("开始连接蓝牙...\n");
    }
}

void set_language_state(void)
{
    if(language == LANGUAGE_CN)
    {
        /*************************系统设置页***********************************/
        lv_label_set_text(guider_ui.screen_system_set_label_set_top, "系统设置");
        lv_label_set_text(guider_ui.screen_system_set_btn_1_label, "          蓝牙配对");
        lv_label_set_text(guider_ui.screen_system_set_btn_2_label, "          屏幕设置");
        lv_label_set_text(guider_ui.screen_system_set_btn_3_label, "          语言切换");
        lv_label_set_text(guider_ui.screen_system_set_btn_4_label, "          产品信息");
        lv_label_set_text(guider_ui.screen_system_set_btn_5_label, "          售后指引");
        lv_label_set_text(guider_ui.screen_system_set_btn_return_label, "返回");
        /*************************系统设置页***********************************/
        /*************************蓝牙连接页***********************************/
        lv_label_set_text(guider_ui.screen_system_set_label_blue_1, "        蓝牙连接");
        lv_label_set_text(guider_ui.screen_system_set_label_blue_2, "正在搜索设备...\n请确保设备蓝牙已开启");
        lv_label_set_text(guider_ui.screen_system_set_label_blue_3, "可用设备:");
        lv_label_set_text(guider_ui.screen_system_set_btn_blue_connect_label, "连接");
        lv_obj_set_pos(guider_ui.screen_system_set_label_blue_5, 55, 383);
        lv_label_set_text(guider_ui.screen_system_set_label_blue_5, "未找到设备?请重试或检查设备蓝牙状态");
        lv_label_set_text(guider_ui.screen_system_set_btn_blue_back_label, "" LV_SYMBOL_LEFT " 返回");
        /*************************蓝牙连接页***********************************/
        /*************************产品信息页***********************************/
        lv_label_set_text(guider_ui.screen_system_set_label_info_top, "        产品信息");
        lv_label_set_text(guider_ui.screen_system_set_label_info_1, "产品名称:");
        lv_label_set_text(guider_ui.screen_system_set_label_info_2, "产品编号:");
        lv_label_set_text(guider_ui.screen_system_set_label_info_3, "生产日期:");
        lv_label_set_text(guider_ui.screen_system_set_label_info_4, "认证标志:");
        lv_label_set_text(guider_ui.screen_system_set_label_info_5, "人造太阳");
        lv_label_set_text(guider_ui.screen_system_set_btn_info_back_label, "" LV_SYMBOL_LEFT " 返回");
        /*************************产品信息页***********************************/
        /*************************语言页***********************************/
        lv_label_set_text(guider_ui.screen_system_set_label_language_top, "       语言切换");
        lv_label_set_text(guider_ui.screen_system_set_btn_language_back_label, "" LV_SYMBOL_LEFT " 返回");
        lv_label_set_text(guider_ui.screen_system_set_btn_language_cn_label, "中文简体        ");
        lv_obj_set_pos(guider_ui.screen_system_set_label_language_ok, 148, 190);
        lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_language_cn, lv_color_hex(0x431f1f), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(guider_ui.screen_system_set_btn_language_cn, lv_color_hex(0xe34646), LV_PART_MAIN|LV_STATE_DEFAULT);

        lv_label_set_text(guider_ui.screen_system_set_btn_language_en_label, "英文");
        lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_language_en, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(guider_ui.screen_system_set_btn_language_en, lv_color_hex(0x2b2b2b), LV_PART_MAIN|LV_STATE_DEFAULT);
        /*************************语言页***********************************/
        /*************************屏幕设置页***********************************/
        lv_label_set_text(guider_ui.screen_system_set_label_screen_top, "       屏幕设置");
        lv_label_set_text(guider_ui.screen_system_set_label_screen_show, "显示亮度");
        lv_label_set_text(guider_ui.screen_system_set_label_screen_auto, "自动息屏时间");
        lv_label_set_text(guider_ui.screen_system_set_btn_screen_30sec_label, "30秒");
        lv_label_set_text(guider_ui.screen_system_set_btn_screen_1min_label, "1分钟");
        lv_label_set_text(guider_ui.screen_system_set_btn_screen_5min_label, "5分钟");
        lv_label_set_text(guider_ui.screen_system_set_btn_screen_back_label, "" LV_SYMBOL_LEFT " 返回");
        /*************************屏幕设置页***********************************/
        /*************************售后指引页***********************************/
        lv_image_set_src(guider_ui.screen_system_set_img_after_bg, LVGL_PATH(bg/after_bg.png));
        lv_label_set_text(guider_ui.screen_system_set_btn_after_back_label, "" LV_SYMBOL_LEFT " 返回");
        /*************************售后指引页***********************************/

    }else{
        /*************************系统设置页***********************************/
        lv_label_set_text(guider_ui.screen_system_set_label_set_top, "System Settings");
        lv_label_set_text(guider_ui.screen_system_set_btn_1_label, "          Bluetooth");
        lv_label_set_text(guider_ui.screen_system_set_btn_2_label, "          Display Settings");
        lv_label_set_text(guider_ui.screen_system_set_btn_3_label, "          Language");
        lv_label_set_text(guider_ui.screen_system_set_btn_4_label, "          Product Info.");
        lv_label_set_text(guider_ui.screen_system_set_btn_5_label, "          After Sales");
        lv_label_set_text(guider_ui.screen_system_set_btn_return_label, "Back");
        /*************************系统设置页***********************************/
        /*************************蓝牙连接页***********************************/
        lv_label_set_text(guider_ui.screen_system_set_label_blue_1, "        Bluetooth");
        lv_label_set_text(guider_ui.screen_system_set_label_blue_2, "Search Device...\nmake sure the bluetooth is turned on");
        lv_label_set_text(guider_ui.screen_system_set_label_blue_3, "Available devices:");
        lv_label_set_text(guider_ui.screen_system_set_btn_blue_connect_label, "Connect");
        lv_obj_set_pos(guider_ui.screen_system_set_label_blue_5, 55, 373);
        lv_label_set_text(guider_ui.screen_system_set_label_blue_5, "Unable to find device?\nPlease retry or check the bluetooth status");
        lv_label_set_text(guider_ui.screen_system_set_btn_blue_back_label, "" LV_SYMBOL_LEFT " Back");
        /*************************蓝牙连接页***********************************/
        /*************************产品信息页***********************************/
        lv_label_set_text(guider_ui.screen_system_set_label_info_top, "        Product Info.");
        lv_label_set_text(guider_ui.screen_system_set_label_info_1, "Name:");
        lv_label_set_text(guider_ui.screen_system_set_label_info_2, "Number:");
        lv_label_set_text(guider_ui.screen_system_set_label_info_3, "Production Date:");
        lv_label_set_text(guider_ui.screen_system_set_label_info_4, "Certificates:");
        lv_label_set_text(guider_ui.screen_system_set_label_info_5, "Artificial Sun");
        lv_label_set_text(guider_ui.screen_system_set_btn_info_back_label, "" LV_SYMBOL_LEFT " Back");
        /*************************产品信息页***********************************/
        /*************************语言页***********************************/
        lv_label_set_text(guider_ui.screen_system_set_label_language_top, "       Language");
        lv_label_set_text(guider_ui.screen_system_set_btn_language_back_label, "" LV_SYMBOL_LEFT " Back");
        lv_label_set_text(guider_ui.screen_system_set_btn_language_cn_label, "Chinese");
        lv_obj_set_pos(guider_ui.screen_system_set_label_language_ok, 350, 190);
        lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_language_cn, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(guider_ui.screen_system_set_btn_language_cn, lv_color_hex(0x2b2b2b), LV_PART_MAIN|LV_STATE_DEFAULT);

        lv_label_set_text(guider_ui.screen_system_set_btn_language_en_label, "English        ");
        lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_language_en, lv_color_hex(0x431f1f), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(guider_ui.screen_system_set_btn_language_en, lv_color_hex(0xe34646), LV_PART_MAIN|LV_STATE_DEFAULT);
        /*************************语言页***********************************/
        /*************************屏幕设置页***********************************/
        lv_label_set_text(guider_ui.screen_system_set_label_screen_top, "       Display Settings");
        lv_label_set_text(guider_ui.screen_system_set_label_screen_show, "Brightness");
        lv_label_set_text(guider_ui.screen_system_set_label_screen_auto, "Screen Timeout");
        lv_label_set_text(guider_ui.screen_system_set_btn_screen_30sec_label, "30sec");
        lv_label_set_text(guider_ui.screen_system_set_btn_screen_1min_label, "1min");
        lv_label_set_text(guider_ui.screen_system_set_btn_screen_5min_label, "5min");
        lv_label_set_text(guider_ui.screen_system_set_btn_screen_back_label, "" LV_SYMBOL_LEFT " Back");
        /*************************屏幕设置页***********************************/
        /*************************售后指引页***********************************/
        lv_image_set_src(guider_ui.screen_system_set_img_after_bg, LVGL_PATH(bg/after_bg_en.png));
        lv_label_set_text(guider_ui.screen_system_set_btn_after_back_label, "" LV_SYMBOL_LEFT " Back");
        /*************************售后指引页***********************************/
    }

}

static void language_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED)
    {
        if(target == guider_ui.screen_system_set_btn_language_cn)
        {
            language = LANGUAGE_CN;
            lv_label_set_text(guider_ui.screen_system_set_label_language_top, "       语言切换");
            lv_label_set_text(guider_ui.screen_system_set_btn_language_back_label, "" LV_SYMBOL_LEFT " 返回");
            lv_label_set_text(guider_ui.screen_system_set_btn_language_cn_label, "中文简体        ");
            lv_obj_set_pos(guider_ui.screen_system_set_label_language_ok, 148, 190);
            lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_language_cn, lv_color_hex(0x431f1f), LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(guider_ui.screen_system_set_btn_language_cn, lv_color_hex(0xe34646), LV_PART_MAIN|LV_STATE_DEFAULT);

            lv_label_set_text(guider_ui.screen_system_set_btn_language_en_label, "英文");
            lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_language_en, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(guider_ui.screen_system_set_btn_language_en, lv_color_hex(0x2b2b2b), LV_PART_MAIN|LV_STATE_DEFAULT);
        }
        else if(target == guider_ui.screen_system_set_btn_language_en)
        {
            language = LANGUAGE_EN;
            lv_label_set_text(guider_ui.screen_system_set_label_language_top, "Language");
            lv_label_set_text(guider_ui.screen_system_set_btn_language_back_label, "" LV_SYMBOL_LEFT " Back");
            lv_label_set_text(guider_ui.screen_system_set_btn_language_cn_label, "Chinese");
            lv_obj_set_pos(guider_ui.screen_system_set_label_language_ok, 350, 190);
            lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_language_cn, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(guider_ui.screen_system_set_btn_language_cn, lv_color_hex(0x2b2b2b), LV_PART_MAIN|LV_STATE_DEFAULT);

            lv_label_set_text(guider_ui.screen_system_set_btn_language_en_label, "English        ");
            lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_language_en, lv_color_hex(0x431f1f), LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(guider_ui.screen_system_set_btn_language_en, lv_color_hex(0xe34646), LV_PART_MAIN|LV_STATE_DEFAULT);
        }
        set_language_state();
        save_begin();
    }
}
/**
 * @brief 亮度滑块事件处理函数
 * 当滑块值变化时，更新 label_light 显示当前亮度百分比
 */
static void time_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED)
    {
        lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_screen_30sec, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_screen_1min, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_screen_5min, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);

        if(target == guider_ui.screen_system_set_btn_screen_30sec)
        {
            screen_off_mode = TIMEOUT_MODE_30sec;
            lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_screen_30sec, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
        }
        else if(target == guider_ui.screen_system_set_btn_screen_1min)
        {
            screen_off_mode = TIMEOUT_MODE_1min;
            lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_screen_1min, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
        }
        else if(target == guider_ui.screen_system_set_btn_screen_5min)
        {
            screen_off_mode = TIMEOUT_MODE_5min;
            lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_screen_5min, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
        }
        save_begin();
    }
}
static void slider_light_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        // 获取滑块当前值
        int32_t value = lv_slider_get_value(guider_ui.screen_system_set_slider_screen_light);
        light_value = value; // 更新全局亮度值
        // 格式化并更新标签文本
        char buf[8];
        lv_snprintf(buf, sizeof(buf), "%d%%", light_value);
        lv_label_set_text(guider_ui.screen_system_set_label_screen_light, buf);
        backlight_set(light_value); // 调用函数设置背光亮度

        save_begin();
    }
}


void setup_scr_screen_system_set(lv_ui *ui)
{
    //Write codes screen_system_set
    ui->screen_system_set = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_system_set, 480, 480);
    lv_obj_set_scrollbar_mode(ui->screen_system_set, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_system_set, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(ui->screen_system_set, LVGL_PATH(bg/set_bg.png), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_opa(ui->screen_system_set, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_recolor_opa(ui->screen_system_set, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_btn_1
    ui->screen_system_set_btn_1 = lv_button_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_btn_1, 21, 102);
    lv_obj_set_size(ui->screen_system_set_btn_1, 432, 50);
    ui->screen_system_set_btn_1_label = lv_label_create(ui->screen_system_set_btn_1);
    lv_label_set_text(ui->screen_system_set_btn_1_label, "       蓝牙配对");
    lv_label_set_long_mode(ui->screen_system_set_btn_1_label, LV_LABEL_LONG_CLIP);
    lv_obj_align(ui->screen_system_set_btn_1_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_1_label, LV_PCT(100));

    //Write style for screen_system_set_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_1, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_1, &lv_font_Dengb_23, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_btn_2
    ui->screen_system_set_btn_2 = lv_button_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_btn_2, 21, 162);
    lv_obj_set_size(ui->screen_system_set_btn_2, 432, 50);
    ui->screen_system_set_btn_2_label = lv_label_create(ui->screen_system_set_btn_2);
    lv_label_set_text(ui->screen_system_set_btn_2_label, "       屏幕设置");
    lv_label_set_long_mode(ui->screen_system_set_btn_2_label, LV_LABEL_LONG_CLIP);
    lv_obj_align(ui->screen_system_set_btn_2_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_2, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_2_label, LV_PCT(100));

    //Write style for screen_system_set_btn_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_2, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_btn_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_2, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_2, &lv_font_Dengb_23, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_btn_3
    ui->screen_system_set_btn_3 = lv_button_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_btn_3, 21, 222);
    lv_obj_set_size(ui->screen_system_set_btn_3, 432, 50);
    ui->screen_system_set_btn_3_label = lv_label_create(ui->screen_system_set_btn_3);
    lv_label_set_text(ui->screen_system_set_btn_3_label, "       语言切换");
    lv_label_set_long_mode(ui->screen_system_set_btn_3_label, LV_LABEL_LONG_CLIP);
    lv_obj_align(ui->screen_system_set_btn_3_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_3, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_3_label, LV_PCT(100));

    //Write style for screen_system_set_btn_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_3, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_btn_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_3, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_3, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_3, &lv_font_Dengb_23, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_3, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_btn_4
    ui->screen_system_set_btn_4 = lv_button_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_btn_4, 21, 285);
    lv_obj_set_size(ui->screen_system_set_btn_4, 432, 50);
    ui->screen_system_set_btn_4_label = lv_label_create(ui->screen_system_set_btn_4);
    lv_label_set_text(ui->screen_system_set_btn_4_label, "       产品信息");
    lv_label_set_long_mode(ui->screen_system_set_btn_4_label, LV_LABEL_LONG_CLIP);
    lv_obj_align(ui->screen_system_set_btn_4_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_4, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_4_label, LV_PCT(100));

    //Write style for screen_system_set_btn_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_4, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_4, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_btn_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_4, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_4, &lv_font_Dengb_23, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_4, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_btn_5
    ui->screen_system_set_btn_5 = lv_button_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_btn_5, 21, 346);
    lv_obj_set_size(ui->screen_system_set_btn_5, 432, 50);
    ui->screen_system_set_btn_5_label = lv_label_create(ui->screen_system_set_btn_5);
    lv_label_set_text(ui->screen_system_set_btn_5_label, "       售后指引");
    lv_label_set_long_mode(ui->screen_system_set_btn_5_label, LV_LABEL_LONG_CLIP);
    lv_obj_align(ui->screen_system_set_btn_5_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_5, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_5_label, LV_PCT(100));

    //Write style for screen_system_set_btn_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_5, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_5, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_btn_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_5, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_5, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_5, &lv_font_Dengb_23, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_5, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_btn_return
    ui->screen_system_set_btn_return = lv_button_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_btn_return, 133, 415);
    lv_obj_set_size(ui->screen_system_set_btn_return, 215, 42);
    ui->screen_system_set_btn_return_label = lv_label_create(ui->screen_system_set_btn_return);
    lv_label_set_text(ui->screen_system_set_btn_return_label, "返回");
    lv_label_set_long_mode(ui->screen_system_set_btn_return_label, LV_LABEL_LONG_CLIP);
    lv_obj_align(ui->screen_system_set_btn_return_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_return, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_return_label, LV_PCT(100));

    //Write style for screen_system_set_btn_return, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_return, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_return, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_return, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_btn_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_return, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_return, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_return, &lv_font_Dengb_23, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_return, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_return, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_return
    ui->screen_system_set_label_return = lv_label_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_label_return, 192, 426);
    lv_obj_set_size(ui->screen_system_set_label_return, 24, 21);
    lv_label_set_text(ui->screen_system_set_label_return, "" LV_SYMBOL_LEFT " ");
    lv_label_set_long_mode(ui->screen_system_set_label_return, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_return, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_label_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_return, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_return, &lv_font_Dengb_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_return, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_return, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_1
    ui->screen_system_set_label_1 = lv_label_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_label_1, 421, 116);
    lv_obj_set_size(ui->screen_system_set_label_1, 24, 21);
    lv_label_set_text(ui->screen_system_set_label_1, "" LV_SYMBOL_RIGHT " ");
    lv_label_set_long_mode(ui->screen_system_set_label_1, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_1, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_1, &lv_font_Dengb_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_2
    ui->screen_system_set_label_2 = lv_label_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_label_2, 421, 178);
    lv_obj_set_size(ui->screen_system_set_label_2, 24, 21);
    lv_label_set_text(ui->screen_system_set_label_2, "" LV_SYMBOL_RIGHT " ");
    lv_label_set_long_mode(ui->screen_system_set_label_2, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_2, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_2, &lv_font_Dengb_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_3
    ui->screen_system_set_label_3 = lv_label_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_label_3, 421, 238);
    lv_obj_set_size(ui->screen_system_set_label_3, 24, 21);
    lv_label_set_text(ui->screen_system_set_label_3, "" LV_SYMBOL_RIGHT " ");
    lv_label_set_long_mode(ui->screen_system_set_label_3, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_3, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_3, &lv_font_Dengb_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_4
    ui->screen_system_set_label_4 = lv_label_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_label_4, 421, 301);
    lv_obj_set_size(ui->screen_system_set_label_4, 24, 21);
    lv_label_set_text(ui->screen_system_set_label_4, "" LV_SYMBOL_RIGHT " ");
    lv_label_set_long_mode(ui->screen_system_set_label_4, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_4, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_4, &lv_font_Dengb_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_5
    ui->screen_system_set_label_5 = lv_label_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_label_5, 421, 362);
    lv_obj_set_size(ui->screen_system_set_label_5, 24, 21);
    lv_label_set_text(ui->screen_system_set_label_5, "" LV_SYMBOL_RIGHT " ");
    lv_label_set_long_mode(ui->screen_system_set_label_5, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_5, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_5, &lv_font_Dengb_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_5, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_set_top
    ui->screen_system_set_label_set_top = lv_label_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_label_set_top, 204, 61);
    lv_obj_set_size(ui->screen_system_set_label_set_top, 250, 26);
    lv_label_set_text(ui->screen_system_set_label_set_top, "系统设置");
    lv_label_set_long_mode(ui->screen_system_set_label_set_top, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_set_top, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_set_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_set_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_set_top, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_set_top, &lv_font_Dengb_23, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_set_top, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_set_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_set_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_set_top, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_set_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_set_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_set_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_set_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_set_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_set_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_icon_1
    ui->screen_system_set_label_icon_1 = lv_label_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_label_icon_1, 37, 111);
    lv_obj_set_size(ui->screen_system_set_label_icon_1, 33, 33);
    lv_label_set_text(ui->screen_system_set_label_icon_1, "");
    lv_label_set_long_mode(ui->screen_system_set_label_icon_1, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_icon_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_icon_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_icon_1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_icon_1, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_icon_1, &lv_font_iconfont_sys_set_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_icon_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_icon_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_icon_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_icon_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_icon_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_icon_1, lv_color_hex(0x4c2828), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_icon_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_icon_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_icon_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_icon_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_icon_1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_icon_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_icon_2
    ui->screen_system_set_label_icon_2 = lv_label_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_label_icon_2, 37, 170);
    lv_obj_set_size(ui->screen_system_set_label_icon_2, 33, 33);
    lv_label_set_text(ui->screen_system_set_label_icon_2, "");
    lv_label_set_long_mode(ui->screen_system_set_label_icon_2, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_icon_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_icon_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_icon_2, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_icon_2, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_icon_2, &lv_font_iconfont_sys_set_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_icon_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_icon_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_icon_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_icon_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_icon_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_icon_2, lv_color_hex(0x4c2828), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_icon_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_icon_2, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_icon_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_icon_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_icon_2, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_icon_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_icon_3
    ui->screen_system_set_label_icon_3 = lv_label_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_label_icon_3, 37, 232);
    lv_obj_set_size(ui->screen_system_set_label_icon_3, 33, 33);
    lv_label_set_text(ui->screen_system_set_label_icon_3, "");
    lv_label_set_long_mode(ui->screen_system_set_label_icon_3, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_icon_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_icon_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_icon_3, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_icon_3, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_icon_3, &lv_font_iconfont_sys_set_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_icon_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_icon_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_icon_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_icon_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_icon_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_icon_3, lv_color_hex(0x4c2828), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_icon_3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_icon_3, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_icon_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_icon_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_icon_3, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_icon_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_icon_4
    ui->screen_system_set_label_icon_4 = lv_label_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_label_icon_4, 37, 293);
    lv_obj_set_size(ui->screen_system_set_label_icon_4, 33, 33);
    lv_label_set_text(ui->screen_system_set_label_icon_4, "");
    lv_label_set_long_mode(ui->screen_system_set_label_icon_4, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_icon_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_icon_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_icon_4, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_icon_4, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_icon_4, &lv_font_iconfont_sys_set_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_icon_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_icon_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_icon_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_icon_4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_icon_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_icon_4, lv_color_hex(0x4c2828), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_icon_4, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_icon_4, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_icon_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_icon_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_icon_4, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_icon_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_icon_5
    ui->screen_system_set_label_icon_5 = lv_label_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_label_icon_5, 37, 354);
    lv_obj_set_size(ui->screen_system_set_label_icon_5, 33, 33);
    lv_label_set_text(ui->screen_system_set_label_icon_5, "");
    lv_label_set_long_mode(ui->screen_system_set_label_icon_5, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_icon_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_icon_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_icon_5, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_icon_5, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_icon_5, &lv_font_iconfont_sys_set_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_icon_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_icon_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_icon_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_icon_5, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_icon_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_icon_5, lv_color_hex(0x4c2828), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_icon_5, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_icon_5, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_icon_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_icon_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_icon_5, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_icon_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);


    //Write codes screen_system_set_tileview_1
    ui->screen_system_set_tileview_1 = lv_tileview_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_tileview_1, 481, 0);
    lv_obj_set_size(ui->screen_system_set_tileview_1, 480, 480);
    lv_obj_set_scrollbar_mode(ui->screen_system_set_tileview_1, LV_SCROLLBAR_MODE_OFF);
    ui->screen_system_set_tileview_1_tile_screen_set = lv_tileview_add_tile(ui->screen_system_set_tileview_1, 0, 0, LV_DIR_RIGHT);

    //Write style for screen_system_set_tileview_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_tileview_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_tileview_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_tileview_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_system_set_tileview_1, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_tileview_1, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_tileview_1, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_tileview_1, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_tileview_1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes screen_system_set_img_screen_bg
    ui->screen_system_set_img_screen_bg = lv_image_create(ui->screen_system_set_tileview_1_tile_screen_set);
    lv_obj_set_pos(ui->screen_system_set_img_screen_bg, 0, 0);
    lv_obj_set_size(ui->screen_system_set_img_screen_bg, 480, 480);
    lv_obj_add_flag(ui->screen_system_set_img_screen_bg, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->screen_system_set_img_screen_bg, LVGL_PATH(bg/home_bg.png));
    lv_image_set_pivot(ui->screen_system_set_img_screen_bg, 50,50);
    lv_image_set_rotation(ui->screen_system_set_img_screen_bg, 0);

    //Write style for screen_system_set_img_screen_bg, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_system_set_img_screen_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_system_set_img_screen_bg, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_screen_b2
    ui->screen_system_set_label_screen_b2 = lv_label_create(ui->screen_system_set_tileview_1_tile_screen_set);
    lv_obj_set_pos(ui->screen_system_set_label_screen_b2, 15, 140);
    lv_obj_set_size(ui->screen_system_set_label_screen_b2, 447, 140);
    lv_label_set_text(ui->screen_system_set_label_screen_b2, "");
    lv_label_set_long_mode(ui->screen_system_set_label_screen_b2, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_screen_b2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_screen_b2, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_label_screen_b2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_label_screen_b2, lv_color_hex(0x262626), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_label_screen_b2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_screen_b2, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_screen_b2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_screen_b2, &lv_font_Dengb_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_screen_b2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_screen_b2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_screen_b2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_screen_b2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_screen_b2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_screen_b2, lv_color_hex(0x131313), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_screen_b2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_screen_b2, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_screen_b2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_screen_b2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_screen_b2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_screen_b2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_btn_screen_30sec
    ui->screen_system_set_btn_screen_30sec = lv_button_create(ui->screen_system_set_tileview_1_tile_screen_set);
    lv_obj_set_pos(ui->screen_system_set_btn_screen_30sec, 42, 239);
    lv_obj_set_size(ui->screen_system_set_btn_screen_30sec, 118, 31);
    ui->screen_system_set_btn_screen_30sec_label = lv_label_create(ui->screen_system_set_btn_screen_30sec);
    lv_label_set_text(ui->screen_system_set_btn_screen_30sec_label, "30秒");
    lv_label_set_long_mode(ui->screen_system_set_btn_screen_30sec_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_system_set_btn_screen_30sec_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_screen_30sec, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_screen_30sec_label, LV_PCT(100));

    //Write style for screen_system_set_btn_screen_30sec, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_screen_30sec, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_screen_30sec, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_screen_30sec, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_btn_screen_30sec, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_btn_screen_30sec, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_btn_screen_30sec, lv_color_hex(0x2b2b2b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_btn_screen_30sec, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_screen_30sec, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_screen_30sec, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_screen_30sec, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_screen_30sec, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_screen_30sec, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_screen_30sec, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_btn_screen_1min
    ui->screen_system_set_btn_screen_1min = lv_button_create(ui->screen_system_set_tileview_1_tile_screen_set);
    lv_obj_set_pos(ui->screen_system_set_btn_screen_1min, 180, 239);
    lv_obj_set_size(ui->screen_system_set_btn_screen_1min, 118, 31);
    ui->screen_system_set_btn_screen_1min_label = lv_label_create(ui->screen_system_set_btn_screen_1min);
    lv_label_set_text(ui->screen_system_set_btn_screen_1min_label, "1分钟");
    lv_label_set_long_mode(ui->screen_system_set_btn_screen_1min_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_system_set_btn_screen_1min_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_screen_1min, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_screen_1min_label, LV_PCT(100));

    //Write style for screen_system_set_btn_screen_1min, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_screen_1min, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_screen_1min, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_screen_1min, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_btn_screen_1min, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_btn_screen_1min, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_btn_screen_1min, lv_color_hex(0x2b2b2b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_btn_screen_1min, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_screen_1min, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_screen_1min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_screen_1min, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_screen_1min, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_screen_1min, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_screen_1min, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_btn_screen_5min
    ui->screen_system_set_btn_screen_5min = lv_button_create(ui->screen_system_set_tileview_1_tile_screen_set);
    lv_obj_set_pos(ui->screen_system_set_btn_screen_5min, 318, 239);
    lv_obj_set_size(ui->screen_system_set_btn_screen_5min, 118, 31);
    ui->screen_system_set_btn_screen_5min_label = lv_label_create(ui->screen_system_set_btn_screen_5min);
    lv_label_set_text(ui->screen_system_set_btn_screen_5min_label, "5分钟");
    lv_label_set_long_mode(ui->screen_system_set_btn_screen_5min_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_system_set_btn_screen_5min_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_screen_5min, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_screen_5min_label, LV_PCT(100));

    //Write style for screen_system_set_btn_screen_5min, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_screen_5min, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_screen_5min, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_screen_5min, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_btn_screen_5min, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_btn_screen_5min, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_btn_screen_5min, lv_color_hex(0x2b2b2b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_btn_screen_5min, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_screen_5min, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_screen_5min, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_screen_5min, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_screen_5min, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_screen_5min, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_screen_5min, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_slider_screen_light
    ui->screen_system_set_slider_screen_light = lv_slider_create(ui->screen_system_set_tileview_1_tile_screen_set);
    lv_obj_set_pos(ui->screen_system_set_slider_screen_light, 28, 188);
    lv_obj_set_size(ui->screen_system_set_slider_screen_light, 419, 6);
    lv_slider_set_range(ui->screen_system_set_slider_screen_light, 0, 100);
    lv_slider_set_mode(ui->screen_system_set_slider_screen_light, LV_SLIDER_MODE_NORMAL);
    lv_slider_set_value(ui->screen_system_set_slider_screen_light, light_value, LV_ANIM_OFF);

    //Write style for screen_system_set_slider_screen_light, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_slider_screen_light, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_slider_screen_light, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_slider_screen_light, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_slider_screen_light, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->screen_system_set_slider_screen_light, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_slider_screen_light, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_system_set_slider_screen_light, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_slider_screen_light, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_slider_screen_light, lv_color_hex(0x1e2937), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_slider_screen_light, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_slider_screen_light, 8, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for screen_system_set_slider_screen_light, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_slider_screen_light, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_slider_screen_light, lv_color_hex(0xdc2726), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_slider_screen_light, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_slider_screen_light, 8, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_screen_light
    ui->screen_system_set_label_screen_light = lv_label_create(ui->screen_system_set_tileview_1_tile_screen_set);
    lv_obj_set_pos(ui->screen_system_set_label_screen_light, 404, 165);
    lv_obj_set_size(ui->screen_system_set_label_screen_light, 43, 17);
    lv_label_set_text_fmt(ui->screen_system_set_label_screen_light, "%d%%", light_value);
    lv_label_set_long_mode(ui->screen_system_set_label_screen_light, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_screen_light, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_screen_light, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_screen_light, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_screen_light, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_screen_light, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_screen_light, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_screen_light, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_screen_light, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_screen_light, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_screen_light, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_screen_light, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_screen_light, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_screen_light, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_screen_light, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_screen_light, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_btn_screen_back
    ui->screen_system_set_btn_screen_back = lv_button_create(ui->screen_system_set_tileview_1_tile_screen_set);
    lv_obj_set_pos(ui->screen_system_set_btn_screen_back, 180, 330);
    lv_obj_set_size(ui->screen_system_set_btn_screen_back, 118, 31);
    ui->screen_system_set_btn_screen_back_label = lv_label_create(ui->screen_system_set_btn_screen_back);
    lv_label_set_text(ui->screen_system_set_btn_screen_back_label, "" LV_SYMBOL_LEFT " 返回");
    lv_label_set_long_mode(ui->screen_system_set_btn_screen_back_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_system_set_btn_screen_back_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_screen_back, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_screen_back_label, LV_PCT(100));

    //Write style for screen_system_set_btn_screen_back, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_screen_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_screen_back, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_screen_back, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_btn_screen_back, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_btn_screen_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_btn_screen_back, lv_color_hex(0x414141), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_btn_screen_back, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_screen_back, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_screen_back, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_screen_back, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_screen_back, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_screen_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_screen_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_screen_show
    ui->screen_system_set_label_screen_show = lv_label_create(ui->screen_system_set_tileview_1_tile_screen_set);
    lv_obj_set_pos(ui->screen_system_set_label_screen_show, 60, 164);
    lv_obj_set_size(ui->screen_system_set_label_screen_show, 227, 18);
    lv_label_set_text(ui->screen_system_set_label_screen_show, "显示亮度");
    lv_label_set_long_mode(ui->screen_system_set_label_screen_show, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_screen_show, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_screen_show, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_screen_show, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_screen_show, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_screen_show, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_screen_show, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_screen_show, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_screen_show, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_screen_show, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_screen_show, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_screen_show, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_screen_show, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_screen_show, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_screen_show, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_screen_show, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_screen_auto
    ui->screen_system_set_label_screen_auto = lv_label_create(ui->screen_system_set_tileview_1_tile_screen_set);
    lv_obj_set_pos(ui->screen_system_set_label_screen_auto, 61, 212);
    lv_obj_set_size(ui->screen_system_set_label_screen_auto, 227, 18);
    lv_label_set_text(ui->screen_system_set_label_screen_auto, "自动息屏时间");
    lv_label_set_long_mode(ui->screen_system_set_label_screen_auto, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_screen_auto, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_screen_auto, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_label_screen_auto, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_screen_auto, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_screen_auto, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_screen_auto, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_screen_auto, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_screen_auto, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_screen_auto, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_screen_auto, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_screen_auto, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_screen_auto, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_screen_auto, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_screen_auto, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_screen_auto, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_screen_top
    ui->screen_system_set_label_screen_top = lv_label_create(ui->screen_system_set_tileview_1_tile_screen_set);
    lv_obj_set_pos(ui->screen_system_set_label_screen_top, 16, 101);
    lv_obj_set_size(ui->screen_system_set_label_screen_top, 447, 52);
    lv_label_set_text(ui->screen_system_set_label_screen_top, "       屏幕设置");
    lv_label_set_long_mode(ui->screen_system_set_label_screen_top, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_screen_top, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_screen_top, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_label_screen_top, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_label_screen_top, lv_color_hex(0x262626), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_label_screen_top, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_screen_top, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_screen_top, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_screen_top, &lv_font_Dengb_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_screen_top, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_screen_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_screen_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_screen_top, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_screen_top, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_screen_top, lv_color_hex(0x1b1b1b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_screen_top, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_screen_top, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_screen_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_screen_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_screen_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_screen_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_screen_i1
    ui->screen_system_set_label_screen_i1 = lv_label_create(ui->screen_system_set_tileview_1_tile_screen_set);
    lv_obj_set_pos(ui->screen_system_set_label_screen_i1, 29, 113);
    lv_obj_set_size(ui->screen_system_set_label_screen_i1, 27, 27);
    lv_label_set_text(ui->screen_system_set_label_screen_i1, "");
    lv_label_set_long_mode(ui->screen_system_set_label_screen_i1, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_screen_i1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_screen_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_screen_i1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_screen_i1, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_screen_i1, &lv_font_iconfont_sys_set_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_screen_i1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_screen_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_screen_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_screen_i1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_screen_i1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_screen_i1, lv_color_hex(0x4c2828), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_screen_i1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_screen_i1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_screen_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_screen_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_screen_i1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_screen_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_screen_i2
    ui->screen_system_set_label_screen_i2 = lv_label_create(ui->screen_system_set_tileview_1_tile_screen_set);
    lv_obj_set_pos(ui->screen_system_set_label_screen_i2, 34, 157);
    lv_obj_set_size(ui->screen_system_set_label_screen_i2, 21, 30);
    lv_label_set_text(ui->screen_system_set_label_screen_i2, "");
    lv_label_set_long_mode(ui->screen_system_set_label_screen_i2, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_screen_i2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_screen_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_screen_i2, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_screen_i2, lv_color_hex(0x8b919b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_screen_i2, &lv_font_iconfont_screen_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_screen_i2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_screen_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_screen_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_screen_i2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_screen_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_screen_i2, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_screen_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_screen_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_screen_i2, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_screen_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_screen_i3
    ui->screen_system_set_label_screen_i3 = lv_label_create(ui->screen_system_set_tileview_1_tile_screen_set);
    lv_obj_set_pos(ui->screen_system_set_label_screen_i3, 34, 207);
    lv_obj_set_size(ui->screen_system_set_label_screen_i3, 21, 30);
    lv_label_set_text(ui->screen_system_set_label_screen_i3, "");
    lv_label_set_long_mode(ui->screen_system_set_label_screen_i3, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_screen_i3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_screen_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_screen_i3, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_screen_i3, lv_color_hex(0x8b919b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_screen_i3, &lv_font_iconfont_screen_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_screen_i3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_screen_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_screen_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_screen_i3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_screen_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_screen_i3, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_screen_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_screen_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_screen_i3, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_screen_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_tileview_2
    ui->screen_system_set_tileview_2 = lv_tileview_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_tileview_2, -481, 0);
    lv_obj_set_size(ui->screen_system_set_tileview_2, 480, 480);
    lv_obj_set_scrollbar_mode(ui->screen_system_set_tileview_2, LV_SCROLLBAR_MODE_OFF);
    ui->screen_system_set_tileview_2_tile_language_set = lv_tileview_add_tile(ui->screen_system_set_tileview_2, 0, 0, LV_DIR_RIGHT);

    //Write style for screen_system_set_tileview_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_system_set_tileview_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_tileview_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_tileview_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_system_set_tileview_2, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_system_set_tileview_2, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_tileview_2, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_tileview_2, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_tileview_2, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes screen_system_set_img_language_bg
    ui->screen_system_set_img_language_bg = lv_image_create(ui->screen_system_set_tileview_2_tile_language_set);
    lv_obj_set_pos(ui->screen_system_set_img_language_bg, 0, 0);
    lv_obj_set_size(ui->screen_system_set_img_language_bg, 480, 480);
    lv_obj_add_flag(ui->screen_system_set_img_language_bg, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->screen_system_set_img_language_bg, LVGL_PATH(bg/home_bg.png));
    lv_image_set_pivot(ui->screen_system_set_img_language_bg, 50,50);
    lv_image_set_rotation(ui->screen_system_set_img_language_bg, 0);

    //Write style for screen_system_set_img_language_bg, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_system_set_img_language_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_system_set_img_language_bg, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

     //Write codes screen_system_set_label_language_b1
    ui->screen_system_set_label_language_b1 = lv_label_create(ui->screen_system_set_tileview_2_tile_language_set);
    lv_obj_set_pos(ui->screen_system_set_label_language_b1, 15, 155);
    lv_obj_set_size(ui->screen_system_set_label_language_b1, 450, 77);
    lv_label_set_text(ui->screen_system_set_label_language_b1, "");
    lv_label_set_long_mode(ui->screen_system_set_label_language_b1, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_language_b1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_language_b1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_label_language_b1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_label_language_b1, lv_color_hex(0x262626), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_label_language_b1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_language_b1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_language_b1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_language_b1, &lv_font_Dengb_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_language_b1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_language_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_language_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_language_b1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_language_b1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_language_b1, lv_color_hex(0x131313), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_language_b1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_language_b1, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_language_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_language_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_language_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_language_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);


           //Write codes screen_system_set_label_language_top
    ui->screen_system_set_label_language_top = lv_label_create(ui->screen_system_set_tileview_2_tile_language_set);
    lv_obj_set_pos(ui->screen_system_set_label_language_top, 15, 112);
    lv_obj_set_size(ui->screen_system_set_label_language_top, 450, 55);
    lv_label_set_text(ui->screen_system_set_label_language_top, "       语言切换");
    lv_label_set_long_mode(ui->screen_system_set_label_language_top, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_language_top, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_language_top, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_label_language_top, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_label_language_top, lv_color_hex(0x262626), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_label_language_top, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_language_top, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_language_top, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_language_top, &lv_font_Dengb_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_language_top, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_language_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_language_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_language_top, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_language_top, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_language_top, lv_color_hex(0x1b1b1b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_language_top, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_language_top, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_language_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_language_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_language_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_language_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);


    //Write codes screen_system_set_btn_language_en
    ui->screen_system_set_btn_language_en = lv_button_create(ui->screen_system_set_tileview_2_tile_language_set);
    lv_obj_set_pos(ui->screen_system_set_btn_language_en, 242, 179);
    lv_obj_set_size(ui->screen_system_set_btn_language_en, 209, 41);
    ui->screen_system_set_btn_language_en_label = lv_label_create(ui->screen_system_set_btn_language_en);
    lv_label_set_text(ui->screen_system_set_btn_language_en_label, "英文");
    lv_label_set_long_mode(ui->screen_system_set_btn_language_en_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_system_set_btn_language_en_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_language_en, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_language_en_label, LV_PCT(100));

    //Write style for screen_system_set_btn_language_en, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_language_en, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_language_en, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_language_en, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_btn_language_en, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_btn_language_en, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_btn_language_en, lv_color_hex(0x2b2b2b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_btn_language_en, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_language_en, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_language_en, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_language_en, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_language_en, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_language_en, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_language_en, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_btn_language_cn
    ui->screen_system_set_btn_language_cn = lv_button_create(ui->screen_system_set_tileview_2_tile_language_set);
    lv_obj_set_pos(ui->screen_system_set_btn_language_cn, 28, 179);
    lv_obj_set_size(ui->screen_system_set_btn_language_cn, 209, 41);
    ui->screen_system_set_btn_language_cn_label = lv_label_create(ui->screen_system_set_btn_language_cn);
    lv_label_set_text(ui->screen_system_set_btn_language_cn_label, "中文简体        ");
    lv_label_set_long_mode(ui->screen_system_set_btn_language_cn_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_system_set_btn_language_cn_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_language_cn, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_language_cn_label, LV_PCT(100));

    //Write style for screen_system_set_btn_language_cn, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_language_cn, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_language_cn, lv_color_hex(0x431f1f), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_language_cn, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_btn_language_cn, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_btn_language_cn, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_btn_language_cn, lv_color_hex(0xe34646), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_btn_language_cn, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_language_cn, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_language_cn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_language_cn, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_language_cn, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_language_cn, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_language_cn, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_language_ok
    ui->screen_system_set_label_language_ok = lv_label_create(ui->screen_system_set_tileview_2_tile_language_set);
    lv_obj_set_pos(ui->screen_system_set_label_language_ok, 148, 190);
    lv_obj_set_size(ui->screen_system_set_label_language_ok, 50, 17);
    lv_label_set_text(ui->screen_system_set_label_language_ok, "" LV_SYMBOL_OK " ");
    lv_label_set_long_mode(ui->screen_system_set_label_language_ok, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_language_ok, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_language_ok, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_language_ok, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_language_ok, lv_color_hex(0xe34646), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_language_ok, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_language_ok, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_language_ok, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_language_ok, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_language_ok, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_language_ok, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_language_ok, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_language_ok, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_language_ok, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_language_ok, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_language_ok, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_btn_language_back
    ui->screen_system_set_btn_language_back = lv_button_create(ui->screen_system_set_tileview_2_tile_language_set);
    lv_obj_set_pos(ui->screen_system_set_btn_language_back, 180, 274);
    lv_obj_set_size(ui->screen_system_set_btn_language_back, 118, 31);
    ui->screen_system_set_btn_language_back_label = lv_label_create(ui->screen_system_set_btn_language_back);
    lv_label_set_text(ui->screen_system_set_btn_language_back_label, "" LV_SYMBOL_LEFT " 返回");
    lv_label_set_long_mode(ui->screen_system_set_btn_language_back_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_system_set_btn_language_back_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_language_back, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_language_back_label, LV_PCT(100));

    //Write style for screen_system_set_btn_language_back, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_language_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_language_back, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_language_back, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_btn_language_back, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_btn_language_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_btn_language_back, lv_color_hex(0x414141), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_btn_language_back, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_language_back, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_language_back, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_language_back, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_language_back, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_language_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_language_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

  //Write codes screen_system_set_label_language_i1
    ui->screen_system_set_label_language_i1 = lv_label_create(ui->screen_system_set_tileview_2_tile_language_set);
    lv_obj_set_pos(ui->screen_system_set_label_language_i1, 31, 126);
    lv_obj_set_size(ui->screen_system_set_label_language_i1, 27, 27);
    lv_label_set_text(ui->screen_system_set_label_language_i1, "");
    lv_label_set_long_mode(ui->screen_system_set_label_language_i1, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_language_i1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_language_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_language_i1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_language_i1, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_language_i1, &lv_font_iconfont_sys_set_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_language_i1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_language_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_language_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_language_i1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_language_i1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_language_i1, lv_color_hex(0x4c2828), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_language_i1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_language_i1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_language_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_language_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_language_i1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_language_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_tileview_3
    ui->screen_system_set_tileview_3 = lv_tileview_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_tileview_3, -481, 0);
    lv_obj_set_size(ui->screen_system_set_tileview_3, 480, 480);
    lv_obj_set_scrollbar_mode(ui->screen_system_set_tileview_3, LV_SCROLLBAR_MODE_OFF);
    ui->screen_system_set_tileview_3_tile_product_info = lv_tileview_add_tile(ui->screen_system_set_tileview_3, 0, 0, LV_DIR_RIGHT);

    //Write style for screen_system_set_tileview_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_system_set_tileview_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_tileview_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_tileview_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_system_set_tileview_3, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_system_set_tileview_3, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_tileview_3, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_tileview_3, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_tileview_3, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



 //Write codes screen_system_set_img_info_bg
    ui->screen_system_set_img_info_bg = lv_image_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_img_info_bg, 0, 0);
    lv_obj_set_size(ui->screen_system_set_img_info_bg, 480, 480);
    lv_obj_add_flag(ui->screen_system_set_img_info_bg, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->screen_system_set_img_info_bg, LVGL_PATH(bg/home_bg.png));
    lv_image_set_pivot(ui->screen_system_set_img_info_bg, 50,50);
    lv_image_set_rotation(ui->screen_system_set_img_info_bg, 0);

    //Write style for screen_system_set_img_info_bg, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_system_set_img_info_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_system_set_img_info_bg, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_b1
    ui->screen_system_set_label_info_b1 = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_b1, 19, 154);
    lv_obj_set_size(ui->screen_system_set_label_info_b1, 448, 185);
    lv_label_set_text(ui->screen_system_set_label_info_b1, "");
    lv_label_set_long_mode(ui->screen_system_set_label_info_b1, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_info_b1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_b1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_label_info_b1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_label_info_b1, lv_color_hex(0x262626), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_label_info_b1, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_b1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_b1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_b1, &lv_font_Dengb_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_b1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_b1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_b1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_info_b1, lv_color_hex(0x131313), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_info_b1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_top
    ui->screen_system_set_label_info_top = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_top, 19, 115);
    lv_obj_set_size(ui->screen_system_set_label_info_top, 448, 55);
    lv_label_set_text(ui->screen_system_set_label_info_top, "        产品信息");
    lv_label_set_long_mode(ui->screen_system_set_label_info_top, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_info_top, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_top, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_label_info_top, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_label_info_top, lv_color_hex(0x242424), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_label_info_top, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_top, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_top, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_top, &lv_font_Dengb_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_top, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_top, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_top, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_info_top, lv_color_hex(0x1b1b1b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_info_top, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_top, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_top, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_1
    ui->screen_system_set_label_info_1 = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_1, 42, 176);
    lv_obj_set_size(ui->screen_system_set_label_info_1, 156, 20);
    lv_label_set_text(ui->screen_system_set_label_info_1, "产品名称:");
    lv_label_set_long_mode(ui->screen_system_set_label_info_1, LV_LABEL_LONG_CLIP);

    //Write style for screen_system_set_label_info_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_1, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_2
    ui->screen_system_set_label_info_2 = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_2, 42, 208);
    lv_obj_set_size(ui->screen_system_set_label_info_2, 156, 20);
    lv_label_set_text(ui->screen_system_set_label_info_2, "产品编号:");
    lv_label_set_long_mode(ui->screen_system_set_label_info_2, LV_LABEL_LONG_CLIP);

    //Write style for screen_system_set_label_info_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_2, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_3
    ui->screen_system_set_label_info_3 = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_3, 42, 241);
    lv_obj_set_size(ui->screen_system_set_label_info_3, 156, 20);
    lv_label_set_text(ui->screen_system_set_label_info_3, "生产日期:");
    lv_label_set_long_mode(ui->screen_system_set_label_info_3, LV_LABEL_LONG_CLIP);

    //Write style for screen_system_set_label_info_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_3, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_3, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_3, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_4
    ui->screen_system_set_label_info_4 = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_4, 42, 271);
    lv_obj_set_size(ui->screen_system_set_label_info_4, 156, 20);
    lv_label_set_text(ui->screen_system_set_label_info_4, "认证标志:");
    lv_label_set_long_mode(ui->screen_system_set_label_info_4, LV_LABEL_LONG_CLIP);

    //Write style for screen_system_set_label_info_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_4, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_4, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_5
    ui->screen_system_set_label_info_5 = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_5, 180, 176);
    lv_obj_set_size(ui->screen_system_set_label_info_5, 269, 20);
    lv_label_set_text(ui->screen_system_set_label_info_5, "人造太阳");
    lv_label_set_long_mode(ui->screen_system_set_label_info_5, LV_LABEL_LONG_CLIP);

    //Write style for screen_system_set_label_info_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_5, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_5, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_5, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_6
    ui->screen_system_set_label_info_6 = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_6, 180, 208);
    lv_obj_set_size(ui->screen_system_set_label_info_6, 269, 20);
    lv_label_set_text(ui->screen_system_set_label_info_6, "3KMLEP-JYMT330-100277");
    lv_label_set_long_mode(ui->screen_system_set_label_info_6, LV_LABEL_LONG_CLIP);

    //Write style for screen_system_set_label_info_6, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_6, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_6, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_6, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_7
    ui->screen_system_set_label_info_7 = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_7, 180, 241);
    lv_obj_set_size(ui->screen_system_set_label_info_7, 269, 20);
    lv_label_set_text(ui->screen_system_set_label_info_7, "2026-6-25");
    lv_label_set_long_mode(ui->screen_system_set_label_info_7, LV_LABEL_LONG_CLIP);

    //Write style for screen_system_set_label_info_7, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_7, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_7, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_7, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_7, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_btn_info_back
    ui->screen_system_set_btn_info_back = lv_button_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_btn_info_back, 179, 369);
    lv_obj_set_size(ui->screen_system_set_btn_info_back, 118, 31);
    ui->screen_system_set_btn_info_back_label = lv_label_create(ui->screen_system_set_btn_info_back);
    lv_label_set_text(ui->screen_system_set_btn_info_back_label, "" LV_SYMBOL_LEFT " Back");
    lv_label_set_long_mode(ui->screen_system_set_btn_info_back_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_system_set_btn_info_back_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_info_back, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_info_back_label, LV_PCT(100));

    //Write style for screen_system_set_btn_info_back, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_btn_info_back, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_btn_info_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_btn_info_back, lv_color_hex(0x414141), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_btn_info_back, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_info_back, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_info_back, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_info_back, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_info_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_info_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_info_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_info_back, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_info_back, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_info_back, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_l1
    ui->screen_system_set_label_info_l1 = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_l1, 33, 199);
    lv_obj_set_size(ui->screen_system_set_label_info_l1, 413, 1);
    lv_label_set_text(ui->screen_system_set_label_info_l1, "");
    lv_label_set_long_mode(ui->screen_system_set_label_info_l1, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_info_l1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_l1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_l1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_l1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_l1, &lv_font_Dengb_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_l1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_l1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_l1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_l1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_l1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_info_l1, lv_color_hex(0x242424), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_info_l1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_l1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_l1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_l1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_l1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_l1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_l2
    ui->screen_system_set_label_info_l2 = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_l2, 33, 230);
    lv_obj_set_size(ui->screen_system_set_label_info_l2, 413, 1);
    lv_label_set_text(ui->screen_system_set_label_info_l2, "");
    lv_label_set_long_mode(ui->screen_system_set_label_info_l2, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_info_l2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_l2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_l2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_l2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_l2, &lv_font_Dengb_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_l2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_l2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_l2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_l2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_l2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_info_l2, lv_color_hex(0x242424), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_info_l2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_l2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_l2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_l2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_l2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_l2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_l3
    ui->screen_system_set_label_info_l3 = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_l3, 33, 263);
    lv_obj_set_size(ui->screen_system_set_label_info_l3, 413, 1);
    lv_label_set_text(ui->screen_system_set_label_info_l3, "");
    lv_label_set_long_mode(ui->screen_system_set_label_info_l3, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_info_l3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_l3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_l3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_l3, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_l3, &lv_font_Dengb_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_l3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_l3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_l3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_l3, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_l3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_info_l3, lv_color_hex(0x242424), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_info_l3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_l3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_l3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_l3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_l3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_l3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_l4
    ui->screen_system_set_label_info_l4 = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_l4, 33, 294);
    lv_obj_set_size(ui->screen_system_set_label_info_l4, 413, 1);
    lv_label_set_text(ui->screen_system_set_label_info_l4, "");
    lv_label_set_long_mode(ui->screen_system_set_label_info_l4, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_info_l4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_l4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_l4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_l4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_l4, &lv_font_Dengb_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_l4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_l4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_l4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_l4, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_l4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_info_l4, lv_color_hex(0x242424), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_info_l4, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_l4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_l4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_l4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_l4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_l4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_i1
    ui->screen_system_set_label_info_i1 = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_i1, 33, 128);
    lv_obj_set_size(ui->screen_system_set_label_info_i1, 27, 27);
    lv_label_set_text(ui->screen_system_set_label_info_i1, "");
    lv_label_set_long_mode(ui->screen_system_set_label_info_i1, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_info_i1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_i1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_i1, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_i1, &lv_font_iconfont_sys_set_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_i1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_i1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_i1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_info_i1, lv_color_hex(0x4c2828), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_info_i1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_i1, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_i1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_i2
    ui->screen_system_set_label_info_i2 = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_i2, 360, 266);
    lv_obj_set_size(ui->screen_system_set_label_info_i2, 40, 27);
    lv_label_set_text(ui->screen_system_set_label_info_i2, "");
    lv_label_set_long_mode(ui->screen_system_set_label_info_i2, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_info_i2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_i2, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_i2, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_i2, &lv_font_iconfont_info_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_i2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_i2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_i2, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_i2, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_i3
    ui->screen_system_set_label_info_i3 = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_i3, 393, 266);
    lv_obj_set_size(ui->screen_system_set_label_info_i3, 27, 27);
    lv_label_set_text(ui->screen_system_set_label_info_i3, "");
    lv_label_set_long_mode(ui->screen_system_set_label_info_i3, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_info_i3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_i3, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_i3, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_i3, &lv_font_iconfont_info_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_i3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_i3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_i3, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_i3, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_info_i4
    ui->screen_system_set_label_info_i4 = lv_label_create(ui->screen_system_set_tileview_3_tile_product_info);
    lv_obj_set_pos(ui->screen_system_set_label_info_i4, 425, 266);
    lv_obj_set_size(ui->screen_system_set_label_info_i4, 27, 27);
    lv_label_set_text(ui->screen_system_set_label_info_i4, "");
    lv_label_set_long_mode(ui->screen_system_set_label_info_i4, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_info_i4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_info_i4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_info_i4, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_info_i4, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_info_i4, &lv_font_iconfont_info_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_info_i4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_info_i4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_info_i4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_info_i4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_info_i4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_info_i4, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_info_i4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_info_i4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_info_i4, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_info_i4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_tileview_4
    ui->screen_system_set_tileview_4 = lv_tileview_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_tileview_4, 481, 0);
    lv_obj_set_size(ui->screen_system_set_tileview_4, 480, 480);
    lv_obj_set_scrollbar_mode(ui->screen_system_set_tileview_4, LV_SCROLLBAR_MODE_OFF);
    ui->screen_system_set_tileview_4_tile_after = lv_tileview_add_tile(ui->screen_system_set_tileview_4, 0, 0, LV_DIR_RIGHT);

    //Write style for screen_system_set_tileview_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_tileview_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_tileview_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_tileview_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_system_set_tileview_4, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_tileview_4, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_tileview_4, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_tileview_4, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_tileview_4, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes screen_system_set_img_after_bg
    ui->screen_system_set_img_after_bg = lv_image_create(ui->screen_system_set_tileview_4_tile_after);
    lv_obj_set_pos(ui->screen_system_set_img_after_bg, 0, 0);
    lv_obj_set_size(ui->screen_system_set_img_after_bg, 480, 480);
    lv_obj_add_flag(ui->screen_system_set_img_after_bg, LV_OBJ_FLAG_CLICKABLE);
    // lv_image_set_src(ui->screen_system_set_img_after_bg, &_after_bg_RGB565A8_480x480);
    lv_image_set_pivot(ui->screen_system_set_img_after_bg, 50,50);
    lv_image_set_rotation(ui->screen_system_set_img_after_bg, 0);

    //Write style for screen_system_set_img_after_bg, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_system_set_img_after_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_system_set_img_after_bg, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_btn_after_back
    ui->screen_system_set_btn_after_back = lv_button_create(ui->screen_system_set_tileview_4_tile_after);
    lv_obj_set_pos(ui->screen_system_set_btn_after_back, 180, 422);
    lv_obj_set_size(ui->screen_system_set_btn_after_back, 118, 31);
    ui->screen_system_set_btn_after_back_label = lv_label_create(ui->screen_system_set_btn_after_back);
    lv_label_set_text(ui->screen_system_set_btn_after_back_label, "" LV_SYMBOL_LEFT " Back");
    lv_label_set_long_mode(ui->screen_system_set_btn_after_back_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_system_set_btn_after_back_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_after_back, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_after_back_label, LV_PCT(100));

    //Write style for screen_system_set_btn_after_back, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_after_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_after_back, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_after_back, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_btn_after_back, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_btn_after_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_btn_after_back, lv_color_hex(0x414141), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_btn_after_back, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_after_back, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_after_back, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_after_back, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_after_back, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_after_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_after_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_tileview_5
    ui->screen_system_set_tileview_5 = lv_tileview_create(ui->screen_system_set);
    lv_obj_set_pos(ui->screen_system_set_tileview_5, -481, 0);
    lv_obj_set_size(ui->screen_system_set_tileview_5, 480, 480);
    lv_obj_set_scrollbar_mode(ui->screen_system_set_tileview_5, LV_SCROLLBAR_MODE_OFF);
    ui->screen_system_set_tileview_5_tile_blue = lv_tileview_add_tile(ui->screen_system_set_tileview_5, 0, 0, LV_DIR_RIGHT);

    //Write style for screen_system_set_tileview_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_tileview_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_tileview_5, lv_color_hex(0xf6f6f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_tileview_5, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_tileview_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_tileview_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_system_set_tileview_5, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_tileview_5, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_tileview_5, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_tileview_5, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_tileview_5, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes screen_system_set_img_blue_bg
    ui->screen_system_set_img_blue_bg = lv_image_create(ui->screen_system_set_tileview_5_tile_blue);
    lv_obj_set_pos(ui->screen_system_set_img_blue_bg, 0, 0);
    lv_obj_set_size(ui->screen_system_set_img_blue_bg, 480, 480);
    lv_obj_add_flag(ui->screen_system_set_img_blue_bg, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->screen_system_set_img_blue_bg, LVGL_PATH(bg/home_bg.png));
    lv_image_set_pivot(ui->screen_system_set_img_blue_bg, 50,50);
    lv_image_set_rotation(ui->screen_system_set_img_blue_bg, 0);

    //Write style for screen_system_set_img_blue_bg, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_system_set_img_blue_bg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_system_set_img_blue_bg, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_blue_b2
    ui->screen_system_set_label_blue_b2 = lv_label_create(ui->screen_system_set_tileview_5_tile_blue);
    lv_obj_set_pos(ui->screen_system_set_label_blue_b2, 15, 130);
    lv_obj_set_size(ui->screen_system_set_label_blue_b2, 452, 285);
    lv_label_set_text(ui->screen_system_set_label_blue_b2, "");
    lv_label_set_long_mode(ui->screen_system_set_label_blue_b2, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_blue_b2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_blue_b2, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_label_blue_b2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_label_blue_b2, lv_color_hex(0x262626), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_label_blue_b2, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_blue_b2, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_blue_b2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_blue_b2, &lv_font_Dengb_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_blue_b2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_blue_b2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_blue_b2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_blue_b2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_blue_b2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_blue_b2, lv_color_hex(0x131313), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_blue_b2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_blue_b2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_blue_b2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_blue_b2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_blue_b2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_blue_b2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);


    //Write codes screen_system_set_label_blue_b1
    ui->screen_system_set_label_blue_b1 = lv_label_create(ui->screen_system_set_tileview_5_tile_blue);
    lv_obj_set_pos(ui->screen_system_set_label_blue_b1, 15, 131);
    lv_obj_set_size(ui->screen_system_set_label_blue_b1, 452, 7);
    lv_label_set_text(ui->screen_system_set_label_blue_b1, "");
    lv_label_set_long_mode(ui->screen_system_set_label_blue_b1, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_blue_b1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_blue_b1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_label_blue_b1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_label_blue_b1, lv_color_hex(0x242424), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_label_blue_b1, LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_blue_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_blue_b1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_blue_b1, &lv_font_Dengb_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_blue_b1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_blue_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_blue_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_blue_b1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_blue_b1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_blue_b1, lv_color_hex(0x171715), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_blue_b1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_blue_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_blue_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_blue_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_blue_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_blue_b1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_blue_b3
    ui->screen_system_set_label_blue_b3 = lv_label_create(ui->screen_system_set_tileview_5_tile_blue);
    lv_obj_set_pos(ui->screen_system_set_label_blue_b3, 36, 293);
    lv_obj_set_size(ui->screen_system_set_label_blue_b3, 413, 1);
    lv_label_set_text(ui->screen_system_set_label_blue_b3, "");
    lv_label_set_long_mode(ui->screen_system_set_label_blue_b3, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_blue_b3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_blue_b3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_blue_b3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_blue_b3, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_blue_b3, &lv_font_Dengb_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_blue_b3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_blue_b3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_blue_b3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_blue_b3, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_blue_b3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_blue_b3, lv_color_hex(0x242424), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_blue_b3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_blue_b3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_blue_b3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_blue_b3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_blue_b3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_blue_b3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_blue_1
    ui->screen_system_set_label_blue_1 = lv_label_create(ui->screen_system_set_tileview_5_tile_blue);
    lv_obj_set_pos(ui->screen_system_set_label_blue_1, 15, 82);
    lv_obj_set_size(ui->screen_system_set_label_blue_1, 452, 60);
    lv_label_set_text(ui->screen_system_set_label_blue_1, "        蓝牙连接");
    lv_label_set_long_mode(ui->screen_system_set_label_blue_1, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_blue_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_blue_1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_label_blue_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_label_blue_1, lv_color_hex(0x242424), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_label_blue_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_blue_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_blue_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_blue_1, &lv_font_Dengb_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_blue_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_blue_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_blue_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_blue_1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_blue_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_blue_1, lv_color_hex(0x1b1b1b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_blue_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_blue_1, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_blue_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_blue_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_blue_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_blue_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);


    //Write codes screen_system_set_arc_blue_1
    ui->screen_system_set_arc_blue_1 = lv_arc_create(ui->screen_system_set_tileview_5_tile_blue);
    lv_obj_set_pos(ui->screen_system_set_arc_blue_1, 201, 157);
    lv_obj_set_size(ui->screen_system_set_arc_blue_1, 86, 80);
    lv_arc_set_mode(ui->screen_system_set_arc_blue_1, LV_ARC_MODE_NORMAL);
    lv_arc_set_range(ui->screen_system_set_arc_blue_1, 0, 334);
    lv_arc_set_bg_angles(ui->screen_system_set_arc_blue_1, 0, 300);
    lv_arc_set_value(ui->screen_system_set_arc_blue_1, 0);
    lv_arc_set_rotation(ui->screen_system_set_arc_blue_1, 0);

    //Write style for screen_system_set_arc_blue_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_arc_blue_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_arc_blue_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui->screen_system_set_arc_blue_1, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui->screen_system_set_arc_blue_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui->screen_system_set_arc_blue_1, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_rounded(ui->screen_system_set_arc_blue_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_arc_blue_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_arc_blue_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_arc_blue_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_arc_blue_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_arc_blue_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_arc_blue_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_system_set_arc_blue_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_arc_width(ui->screen_system_set_arc_blue_1, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for screen_system_set_arc_blue_1, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_arc_blue_1, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_arc_blue_1, lv_color_hex(0xdc2726), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_arc_blue_1, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui->screen_system_set_arc_blue_1, 5, LV_PART_KNOB|LV_STATE_DEFAULT);


    //Write codes screen_system_set_label_blue_2
    ui->screen_system_set_label_blue_2 = lv_label_create(ui->screen_system_set_tileview_5_tile_blue);
    lv_obj_set_pos(ui->screen_system_set_label_blue_2, 35, 239);
    lv_obj_set_size(ui->screen_system_set_label_blue_2, 429, 51);
    lv_label_set_text(ui->screen_system_set_label_blue_2, "正在搜索设备...\n请确保设备蓝牙已开启");
    lv_label_set_long_mode(ui->screen_system_set_label_blue_2, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_blue_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_blue_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_blue_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_blue_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_blue_2, &lv_font_Dengb_21, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_blue_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_blue_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_blue_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_blue_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_blue_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_blue_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_blue_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_blue_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_blue_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_blue_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_blue_3
    ui->screen_system_set_label_blue_3 = lv_label_create(ui->screen_system_set_tileview_5_tile_blue);
    lv_obj_set_pos(ui->screen_system_set_label_blue_3, 33, 302);
    lv_obj_set_size(ui->screen_system_set_label_blue_3, 223, 23);
    lv_label_set_text(ui->screen_system_set_label_blue_3, "可用设备:");
    lv_label_set_long_mode(ui->screen_system_set_label_blue_3, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_blue_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_blue_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_blue_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_blue_3, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_blue_3, &lv_font_Dengb_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_blue_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_blue_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_blue_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_blue_3, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_blue_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_blue_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_blue_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_blue_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_blue_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_blue_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_blue_4
    ui->screen_system_set_label_blue_4 = lv_label_create(ui->screen_system_set_tileview_5_tile_blue);
    lv_obj_set_pos(ui->screen_system_set_label_blue_4, 36, 328);
    lv_obj_set_size(ui->screen_system_set_label_blue_4, 413, 42);
    lv_label_set_text(ui->screen_system_set_label_blue_4, " ");
    lv_label_set_long_mode(ui->screen_system_set_label_blue_4, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_blue_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_blue_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_blue_4, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_blue_4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_blue_4, &lv_font_Dengb_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_blue_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_blue_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_blue_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_blue_4, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_blue_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_blue_4, 13, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_blue_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_blue_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_blue_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_blue_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_blue_5
    ui->screen_system_set_label_blue_5 = lv_label_create(ui->screen_system_set_tileview_5_tile_blue);
    lv_obj_set_pos(ui->screen_system_set_label_blue_5, 55, 383);
    lv_obj_set_size(ui->screen_system_set_label_blue_5, 372, 38);
    lv_label_set_text(ui->screen_system_set_label_blue_5, "未找到设备?请重试或检查设备蓝牙状态");
    lv_label_set_long_mode(ui->screen_system_set_label_blue_5, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_blue_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_blue_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_blue_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_blue_5, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_blue_5, &lv_font_Dengb_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_blue_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_blue_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_blue_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_blue_5, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_blue_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_blue_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_blue_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_blue_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_blue_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_blue_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_blue_i1
    ui->screen_system_set_label_blue_i1 = lv_label_create(ui->screen_system_set_tileview_5_tile_blue);
    lv_obj_set_pos(ui->screen_system_set_label_blue_i1, 30, 93);
    lv_obj_set_size(ui->screen_system_set_label_blue_i1, 33, 33);
    lv_label_set_text(ui->screen_system_set_label_blue_i1, "");
    lv_label_set_long_mode(ui->screen_system_set_label_blue_i1, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_blue_i1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_blue_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_blue_i1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_blue_i1, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_blue_i1, &lv_font_iconfont_sys_set_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_blue_i1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_blue_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_blue_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_blue_i1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_blue_i1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_blue_i1, lv_color_hex(0x4c2828), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_blue_i1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_blue_i1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_blue_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_blue_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_blue_i1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_blue_i1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_blue_i2
    ui->screen_system_set_label_blue_i2 = lv_label_create(ui->screen_system_set_tileview_5_tile_blue);
    lv_obj_set_pos(ui->screen_system_set_label_blue_i2, 209, 164);
    lv_obj_set_size(ui->screen_system_set_label_blue_i2, 65, 65);
    lv_label_set_text(ui->screen_system_set_label_blue_i2, "");
    lv_label_set_long_mode(ui->screen_system_set_label_blue_i2, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_blue_i2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_blue_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_blue_i2, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_blue_i2, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_blue_i2, &lv_font_iconfont_sys_set_50, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_blue_i2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_blue_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_blue_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_blue_i2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_blue_i2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_blue_i2, lv_color_hex(0x4c2828), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_blue_i2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_blue_i2, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_blue_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_blue_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_blue_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_blue_i2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_blue_i3
    ui->screen_system_set_label_blue_i3 = lv_label_create(ui->screen_system_set_tileview_5_tile_blue);
    lv_obj_set_pos(ui->screen_system_set_label_blue_i3, 40, 335);
    lv_obj_set_size(ui->screen_system_set_label_blue_i3, 30, 30);
    lv_label_set_text(ui->screen_system_set_label_blue_i3, "");
    lv_label_set_long_mode(ui->screen_system_set_label_blue_i3, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_blue_i3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_blue_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_blue_i3, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_blue_i3, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_blue_i3, &lv_font_iconfont_sys_set_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_blue_i3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_blue_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_blue_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_blue_i3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_blue_i3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_blue_i3, lv_color_hex(0x4c2828), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_blue_i3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_blue_i3, 3, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_blue_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_blue_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_blue_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_blue_i3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_label_blue_i4
    ui->screen_system_set_label_blue_i4 = lv_label_create(ui->screen_system_set_tileview_5_tile_blue);
    lv_obj_set_pos(ui->screen_system_set_label_blue_i4, 37, 387);
    lv_obj_set_size(ui->screen_system_set_label_blue_i4, 14, 14);
    lv_label_set_text(ui->screen_system_set_label_blue_i4, "?");
    lv_label_set_long_mode(ui->screen_system_set_label_blue_i4, LV_LABEL_LONG_WRAP);

    //Write style for screen_system_set_label_blue_i4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_system_set_label_blue_i4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_label_blue_i4, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_label_blue_i4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_label_blue_i4, &lv_font_Dengb_13, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_label_blue_i4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_system_set_label_blue_i4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_system_set_label_blue_i4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_label_blue_i4, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_system_set_label_blue_i4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_label_blue_i4, lv_color_hex(0x474747), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_label_blue_i4, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_system_set_label_blue_i4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_system_set_label_blue_i4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_system_set_label_blue_i4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_system_set_label_blue_i4, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_label_blue_i4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_btn_blue_back
    ui->screen_system_set_btn_blue_back = lv_button_create(ui->screen_system_set_tileview_5_tile_blue);
    lv_obj_set_pos(ui->screen_system_set_btn_blue_back, 181, 434);
    lv_obj_set_size(ui->screen_system_set_btn_blue_back, 122, 31);
    ui->screen_system_set_btn_blue_back_label = lv_label_create(ui->screen_system_set_btn_blue_back);
    lv_label_set_text(ui->screen_system_set_btn_blue_back_label, "" LV_SYMBOL_LEFT " 返回");
    lv_label_set_long_mode(ui->screen_system_set_btn_blue_back_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_system_set_btn_blue_back_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_blue_back, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_blue_back_label, LV_PCT(100));

    //Write style for screen_system_set_btn_blue_back, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_blue_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_blue_back, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_blue_back, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_btn_blue_back, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_btn_blue_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_btn_blue_back, lv_color_hex(0x414141), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_btn_blue_back, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_blue_back, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_blue_back, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_blue_back, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_blue_back, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_blue_back, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_blue_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_system_set_btn_blue_connect
    ui->screen_system_set_btn_blue_connect = lv_button_create(ui->screen_system_set_tileview_5_tile_blue);
    lv_obj_set_pos(ui->screen_system_set_btn_blue_connect, 371, 336);
    lv_obj_set_size(ui->screen_system_set_btn_blue_connect, 73, 26);
    ui->screen_system_set_btn_blue_connect_label = lv_label_create(ui->screen_system_set_btn_blue_connect);
    lv_label_set_text(ui->screen_system_set_btn_blue_connect_label, "连接");
    lv_label_set_long_mode(ui->screen_system_set_btn_blue_connect_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_system_set_btn_blue_connect_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_system_set_btn_blue_connect, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_system_set_btn_blue_connect_label, LV_PCT(100));

    //Write style for screen_system_set_btn_blue_connect, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_system_set_btn_blue_connect, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_system_set_btn_blue_connect, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_system_set_btn_blue_connect, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_system_set_btn_blue_connect, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_system_set_btn_blue_connect, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_system_set_btn_blue_connect, lv_color_hex(0x414141), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_system_set_btn_blue_connect, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_system_set_btn_blue_connect, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_system_set_btn_blue_connect, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_system_set_btn_blue_connect, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_system_set_btn_blue_connect, &lv_font_Dengb_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_system_set_btn_blue_connect, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_system_set_btn_blue_connect, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);


    /////////////////////////////////////////////
    /////////////////////////////////////////////
    /////////////////////////////////////////////
    /////////////////////////////////////////////
    /////////////////////////////////////////////
    /////////////////////////////////////////////
    /////////////////////////////////////////////
    /////////////////////////////////////////////
    /////////////////////////////////////////////
    /////////////////////////////////////////////
    /////////////////////////////////////////////
    //The custom code of screen_system_set.


    lv_obj_clear_flag(ui->screen_system_set_arc_blue_1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui->screen_system_set, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ui->screen_system_set_tileview_1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ui->screen_system_set_tileview_2, LV_OBJ_FLAG_SCROLLABLE);

    if(screen_off_mode == TIMEOUT_MODE_30sec)
    {
        lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_screen_30sec, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    else if(screen_off_mode == TIMEOUT_MODE_1min)
    {
        lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_screen_1min, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    else if(screen_off_mode == TIMEOUT_MODE_5min)
    {
        lv_obj_set_style_bg_color(guider_ui.screen_system_set_btn_screen_5min, lv_color_hex(0xdc2726), LV_PART_MAIN|LV_STATE_DEFAULT);
    }

    lv_label_set_text_fmt(ui->screen_system_set_label_info_6, "%s", VERSION_NUMBER);
    lv_label_set_text_fmt(ui->screen_system_set_label_info_7, "%s",VERSION_DATE);
    set_language_state(); //设置对应语言

    lv_obj_add_event_cb(ui->screen_system_set_btn_language_cn, language_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_system_set_btn_language_en, language_event_handler, LV_EVENT_ALL, ui);

    lv_obj_add_event_cb(ui->screen_system_set_btn_screen_30sec, time_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_system_set_btn_screen_1min, time_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_system_set_btn_screen_5min, time_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_system_set_slider_screen_light, slider_light_event_handler, LV_EVENT_ALL, ui);

    lv_obj_add_event_cb(guider_ui.screen_system_set_btn_blue_connect, blue_connect_btn_cb, LV_EVENT_CLICKED, NULL);


    // 启动定时器
    if(!g_ble_connected)
    {
        if (blue_timer == NULL)
            blue_timer = lv_timer_create(blue_callback, 1000, 0);
        else
            lv_timer_resume(blue_timer);

    }
    if(!g_ble_connected)
    {
        if (arc_timer == NULL)
            arc_timer = lv_timer_create(arc_callback, 20, 0);
        else
            lv_timer_resume(arc_timer);
    }


    //Update current screen layout.
    lv_obj_update_layout(ui->screen_system_set);

    //Init events for screen.
    events_init_screen_system_set(ui);
}
