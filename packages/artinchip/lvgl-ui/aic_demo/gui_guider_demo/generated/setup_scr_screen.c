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

lv_timer_t *show_timer = NULL;




static void show_callback(lv_timer_t *timer)
{

    if (g_ble_connected)
        lv_label_set_text(guider_ui.screen_label_blue, "" LV_SYMBOL_BLUETOOTH " ");
    else
        lv_label_set_text(guider_ui.screen_label_blue, " ");
}

static void btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    char at_cmd[64] = {0};
    const char *data_str = NULL;

    if (code == LV_EVENT_CLICKED)
    {
        if (target == guider_ui.screen_btn_1)
        {
            lv_timer_pause(show_timer);
            setup_scr_screen_blue(&guider_ui);
            lv_scr_load_anim(guider_ui.screen_blue, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
            return;
        }
        else if (target == guider_ui.screen_btn_2)
        {
            data_str = "open"; // 内容随便改
        }
        else if (target == guider_ui.screen_btn_3)
        {
            data_str = "close"; // 内容随便改
        }
        else if (target == guider_ui.screen_btn_4)
        {
            data_str = "data:3"; // 内容随便改
        }
        else
            return;
        // ======================
        // ✅ 自适应长度（自动算，自动补0）
        // ======================
        int data_len = strlen(data_str);
        rt_sprintf(at_cmd, "AT+TDAT:%s,%03d,%s\r\n",
                   ble_mac,
                   data_len,  // 自动计算长度
                   data_str); // 自动填充内容

        // 发送

        uart_send_at(at_cmd);
        rt_kprintf("[发送] %s", at_cmd);
    }
}

void setup_scr_screen(lv_ui *ui)
{
    // Write codes screen
    ui->screen = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen, 480, 480);
    lv_obj_set_scrollbar_mode(ui->screen, LV_SCROLLBAR_MODE_OFF);

    // Write style for screen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen, lv_color_hex(0xc2faff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Write codes screen_label_blue
    ui->screen_label_blue = lv_label_create(ui->screen);
    lv_obj_set_pos(ui->screen_label_blue, 5, 5);
    lv_obj_set_size(ui->screen_label_blue, 37, 39);
    // if (g_ble_connected)
    //     lv_label_set_text(ui->screen_label_blue, "" LV_SYMBOL_BLUETOOTH " ");
    // else
    lv_label_set_text(ui->screen_label_blue, " ");
    lv_label_set_long_mode(ui->screen_label_blue, LV_LABEL_LONG_WRAP);

    // Write style for screen_label_blue, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_blue, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_blue, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_blue, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_blue, &lv_font_Dengb_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_blue, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_blue, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_blue, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_blue, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_blue, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_blue, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_blue, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_blue, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_blue, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_blue, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Write codes screen_btn_1
    ui->screen_btn_1 = lv_button_create(ui->screen);
    lv_obj_set_pos(ui->screen_btn_1, 372, 10);
    lv_obj_set_size(ui->screen_btn_1, 100, 50);
    ui->screen_btn_1_label = lv_label_create(ui->screen_btn_1);
    lv_label_set_text(ui->screen_btn_1_label, "蓝牙");
    lv_label_set_long_mode(ui->screen_btn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_btn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_btn_1_label, LV_PCT(100));

    // Write style for screen_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_btn_1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_btn_1, lv_color_hex(0x00ffd9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_btn_1, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_btn_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_btn_1, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_btn_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_btn_1, lv_color_hex(0xf00000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_btn_1, &lv_font_Dengb_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_btn_1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_btn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Write codes screen_btn_2
    ui->screen_btn_2 = lv_button_create(ui->screen);
    lv_obj_set_pos(ui->screen_btn_2, 78, 148);
    lv_obj_set_size(ui->screen_btn_2, 100, 50);
    ui->screen_btn_2_label = lv_label_create(ui->screen_btn_2);
    lv_label_set_text(ui->screen_btn_2_label, " 开灯 ");
    lv_label_set_long_mode(ui->screen_btn_2_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_btn_2_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_btn_2, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_btn_2_label, LV_PCT(100));

    // Write style for screen_btn_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_btn_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_btn_2, lv_color_hex(0x00ffd9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_btn_2, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_btn_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_btn_2, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_btn_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_btn_2, lv_color_hex(0xf00000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_btn_2, &lv_font_Dengb_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_btn_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_btn_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Write codes screen_btn_3
    ui->screen_btn_3 = lv_button_create(ui->screen);
    lv_obj_set_pos(ui->screen_btn_3, 194, 215);
    lv_obj_set_size(ui->screen_btn_3, 100, 50);
    ui->screen_btn_3_label = lv_label_create(ui->screen_btn_3);
    lv_label_set_text(ui->screen_btn_3_label, " 关灯 ");
    lv_label_set_long_mode(ui->screen_btn_3_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_btn_3_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_btn_3, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_btn_3_label, LV_PCT(100));

    // Write style for screen_btn_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_btn_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_btn_3, lv_color_hex(0x00ffd9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_btn_3, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_btn_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_btn_3, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_btn_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_btn_3, lv_color_hex(0xf00000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_btn_3, &lv_font_Dengb_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_btn_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_btn_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Write codes screen_btn_4
    ui->screen_btn_4 = lv_button_create(ui->screen);
    lv_obj_set_pos(ui->screen_btn_4, 312, 283);
    lv_obj_set_size(ui->screen_btn_4, 100, 50);
    ui->screen_btn_4_label = lv_label_create(ui->screen_btn_4);
    lv_label_set_text(ui->screen_btn_4_label, "其他");
    lv_label_set_long_mode(ui->screen_btn_4_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_btn_4_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_btn_4, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_btn_4_label, LV_PCT(100));

    // Write style for screen_btn_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_btn_4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_btn_4, lv_color_hex(0x00ffd9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_btn_4, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_btn_4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_btn_4, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_btn_4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_btn_4, lv_color_hex(0xf00000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_btn_4, &lv_font_Dengb_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_btn_4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_btn_4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    // The custom code of screen.
    lv_obj_add_event_cb(ui->screen_btn_1, btn_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui->screen_btn_2, btn_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui->screen_btn_3, btn_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui->screen_btn_4, btn_event_cb, LV_EVENT_ALL, NULL);

    if (show_timer == NULL)
        show_timer = lv_timer_create(show_callback, 1000, 0);
    else
        lv_timer_resume(show_timer);

    // Update current screen layout.
    lv_obj_update_layout(ui->screen);
}
