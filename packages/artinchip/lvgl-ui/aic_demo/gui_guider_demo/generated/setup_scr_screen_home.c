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

#define NO_SHOW(obj) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
#define SHOW(obj)  lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);

lv_timer_t *timeout_timer = NULL;
lv_timer_t *blue_state_timer = NULL;

static void blue_state_callback(lv_timer_t *timer)
{
    static bool last_state = false;
    last_state = g_ble_connected;

    if(g_ble_connected == last_state) return;

    if(g_ble_connected)
    {
        if(language == LANGUAGE_CN) lv_label_set_text(guider_ui.screen_home_btn_3_label, "\n已配对       ");
        else lv_label_set_text(guider_ui.screen_home_btn_3_label, "\nON       ");
        lv_label_set_text(guider_ui.screen_home_btn_ly_label, "");
        lv_obj_set_style_text_font(guider_ui.screen_home_btn_ly, &lv_font_iconfont_blue_36, LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(guider_ui.screen_home_btn_3, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(guider_ui.screen_home_btn_3, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(guider_ui.screen_home_btn_ly, lv_color_hex(0x57afff), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(guider_ui.screen_home_btn_ly, lv_color_hex(0x404043), LV_PART_MAIN|LV_STATE_DEFAULT);
    }else{
        if(language == LANGUAGE_CN) lv_label_set_text(guider_ui.screen_home_btn_3_label, "\n待连接       ");
        else lv_label_set_text(guider_ui.screen_home_btn_3_label, "\nOFF       ");
        lv_label_set_text(guider_ui.screen_home_btn_ly_label, "");
        lv_obj_set_style_text_font(guider_ui.screen_home_btn_ly, &lv_font_iconfont_blue_32, LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(guider_ui.screen_home_btn_3, lv_color_hex(0x161618), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(guider_ui.screen_home_btn_3, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(guider_ui.screen_home_btn_ly, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(guider_ui.screen_home_btn_ly, lv_color_hex(0x262628), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
}

static void timeout_callback(lv_timer_t *timer)
{
    rt_mutex_take(timeout_mutex, RT_WAITING_FOREVER);
    bool timeout_temp = timeout_state;
    rt_mutex_release(timeout_mutex);
    if(timeout_temp) return;// 已经进入息屏状态，不再计时

    timeout_cnt++;

    if(timeout_cnt >= 30 * screen_off_mode) // m无操作，进入息屏
    {
        timeout_cnt = 0;
        rt_mutex_take(timeout_mutex, RT_WAITING_FOREVER);
        timeout_state = true;
        rt_mutex_release(timeout_mutex);
    }

}

// -------------------------
// 动画：img1 大小 + 居中（LVGL v9 专用）
// -------------------------
static void anim_img1_size(void *var, int32_t value)
{
        lv_obj_t *obj = (lv_obj_t *)var;

    // 固定中心点坐标（原始坐标 58,254，大小55）
    int center_x = 59 + 85/2;
    int center_y = 107 + 85/2;

    // 计算新位置（只保证中心点不变）
    int new_x = center_x - value / 2;
    int new_y = center_y - value / 2;

    lv_obj_set_pos(obj, new_x, new_y);
    lv_obj_set_size(obj, value, value);

    // lv_obj_t *obj = (lv_obj_t *)var;
    // lv_obj_set_size(obj, value, value);
    // lv_obj_set_pos(obj, 59 + (85 - value)/2, 107 + (85 - value)/2);
}


static void anim_img1_rot(void *var, int32_t value)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_image_set_rotation(obj, value);
}

static void anim_img1_opa(void *var, int32_t value)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_image_opa(obj, value, 0);
        // 同步缩放动画：透明度变化时，图片同步缩放
    // 当透明度从 255→0 时，缩放从 256(100%)→128(50%)
    int32_t zoom = 128 + (value * 128) / 255;
    lv_img_set_zoom(obj, zoom);
}

// -------------------------
// 按钮点击事件（最终版）
// -------------------------

static void btn_1_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);

    char at_cmd[64] = {0};
    const char *data_str = NULL;

    if (code == LV_EVENT_CLICKED) {
        home_open_state = !home_open_state;

        if (home_open_state)
        {
            data_str = "open";
            // ==================== 关闭 → 开启 ====================
            NO_SHOW(guider_ui.screen_home_line_1);
            if(language == LANGUAGE_CN)
            {
                lv_label_set_text(label, "        开启");
            }else{
                lv_label_set_text(label, "        Turn On");
            }
            lv_obj_set_style_text_color(guider_ui.screen_home_btn_power, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xe21d48), LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui->screen_home_btn_power, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(ui->screen_home_btn_power, lv_color_hex(0xe74a6d), LV_PART_MAIN|LV_STATE_DEFAULT);
            // ✅ 先设置图片
            // lv_image_set_src(guider_ui.screen_home_img_1, LVGL_PATH(bg/home_btn.png));
            // lv_image_set_src(ui->screen_home_img_2, LVGL_PATH(bg/home_sd.png));
            // lv_img_set_zoom(ui->screen_home_img_2, 256  ); // 缩放因子为256表示100%
            // ✅ 再开启动画：75 → 85
            lv_anim_t a1;
            lv_anim_init(&a1);
            lv_anim_set_var(&a1, ui->screen_home_btn_power);
            lv_anim_set_exec_cb(&a1, anim_img1_size);
            lv_anim_set_values(&a1, 78, 85);
            lv_anim_set_time(&a1, 350);
            lv_anim_start(&a1);

            // img2 从小到大 30→50
            // lv_anim_t a3;
            // lv_anim_init(&a3);
            // lv_anim_set_var(&a3, ui->screen_home_img_2);
            // lv_anim_set_exec_cb(&a3, anim_img2_size);
            // lv_anim_set_values(&a3, 20, 50);
            // lv_anim_set_time(&a3, 350);
            // lv_anim_start(&a3);

            lv_anim_t a2;
            lv_anim_init(&a2);
            lv_anim_set_var(&a2, ui->screen_home_img_2);
            lv_anim_set_exec_cb(&a2, anim_img1_rot);
            lv_anim_set_values(&a2, -80, 0);
            lv_anim_set_time(&a2, 350);
            lv_anim_start(&a2);

            lv_anim_t a4;
            lv_anim_init(&a4);
            lv_anim_set_var(&a4, ui->screen_home_img_2);
            lv_anim_set_exec_cb(&a4, anim_img1_opa);
            lv_anim_set_values(&a4, 0, 255);
            lv_anim_set_time(&a4, 350);
            lv_anim_start(&a4);
        }
        else
        {
            data_str = "close";
            // ==================== 开启 → 关闭 ====================
            SHOW(guider_ui.screen_home_line_1);
            if(language == LANGUAGE_CN)
            {
                lv_label_set_text(label, "        关闭");
            }else{
                lv_label_set_text(label, "        Turn Off");
            }
            lv_obj_set_style_text_color(guider_ui.screen_home_btn_power, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x1b181a), LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui->screen_home_btn_power, 140, LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(ui->screen_home_btn_power, lv_color_hex(0x3b383a), LV_PART_MAIN|LV_STATE_DEFAULT);
            // ✅ 先设置图片
            // lv_image_set_src(ui->screen_home_img_1, LVGL_PATH(bg/home_btn_no.png));
            // lv_image_set_src(ui->screen_home_img_2, LVGL_PATH(bg/home_sd.png));
            // lv_img_set_zoom(ui->screen_home_img_2, 128); // 缩放因子为128表示50%
            // ✅ 再开启动画：85 → 75
            lv_anim_t a1;
            lv_anim_init(&a1);
            lv_anim_set_var(&a1, ui->screen_home_btn_power);
            lv_anim_set_exec_cb(&a1, anim_img1_size);
            lv_anim_set_values(&a1, 85, 78);
            lv_anim_set_time(&a1, 350);
            lv_anim_start(&a1);

            // img2 缩小
            // lv_anim_t a3;
            // lv_anim_init(&a3);
            // lv_anim_set_var(&a3, ui->screen_home_img_2);
            // lv_anim_set_exec_cb(&a3, anim_img2_size);
            // lv_anim_set_values(&a3, 50, 20);
            // lv_anim_set_time(&a3, 350);
            // lv_anim_start(&a3);

            lv_anim_t a2;
            lv_anim_init(&a2);
            lv_anim_set_var(&a2, ui->screen_home_img_2);
            lv_anim_set_exec_cb(&a2, anim_img1_rot);
            lv_anim_set_values(&a2, 0, -80);
            lv_anim_set_time(&a2, 350);
            lv_anim_start(&a2);

            lv_anim_t a4;
            lv_anim_init(&a4);
            lv_anim_set_var(&a4, ui->screen_home_img_2);
            lv_anim_set_exec_cb(&a4, anim_img1_opa);
            lv_anim_set_values(&a4, 255, 0);
            lv_anim_set_time(&a4, 350);
            lv_anim_start(&a4);
        }
        if(g_ble_connected)
        {
            int data_len = strlen(data_str);
            rt_sprintf(at_cmd, "AT+TDAT:%s,%03d,%s\r\n",
                ble_mac,
                data_len,  // 自动计算长度
                data_str); // 自动填充内容

            // 发送

            uart_send_at(at_cmd);
            rt_kprintf("[发送] %s", at_cmd);
            save_begin();
        }
    }
}


static void anim_btn_ly_size(void *var, int32_t value)
{
    lv_obj_t *obj = (lv_obj_t *)var;

    // 固定中心点坐标（原始坐标 59,265，大小60）
    int center_x = 59 + 60/2;
    int center_y = 265 + 60/2;

    // 计算新位置（只保证中心点不变）
    int new_x = center_x - value / 2;
    int new_y = center_y - value / 2;

    lv_obj_set_pos(obj, new_x, new_y);
    lv_obj_set_size(obj, value, value);
}



// static void btn_3_event(lv_event_t *e)
// {
//     lv_event_code_t code = lv_event_get_code(e);
//     lv_obj_t *target = lv_event_get_target(e);

//     if(code == LV_EVENT_CLICKED)
//     {
//         home_blue_state = !home_blue_state;

//         if(home_blue_state == false)
//         {
//             if(language == LANGUAGE_CN)
//             {
//                 lv_label_set_text(guider_ui.screen_home_btn_3_label, "\n待连接       ");
//             }
//             else
//             {
//                 lv_label_set_text(guider_ui.screen_home_btn_3_label, "\nOFF       ");
//             }

//             lv_label_set_text(guider_ui.screen_home_btn_ly_label, "");
//             lv_obj_set_style_text_font(guider_ui.screen_home_btn_ly, &lv_font_iconfont_blue_32, LV_PART_MAIN|LV_STATE_DEFAULT);
//             lv_obj_set_style_bg_color(guider_ui.screen_home_btn_3, lv_color_hex(0x161618), LV_PART_MAIN|LV_STATE_DEFAULT);
//             lv_obj_set_style_text_color(guider_ui.screen_home_btn_3, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
//             lv_obj_set_style_text_color(guider_ui.screen_home_btn_ly, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
//             lv_obj_set_style_bg_color(guider_ui.screen_home_btn_ly, lv_color_hex(0x262628), LV_PART_MAIN|LV_STATE_DEFAULT);

//             lv_anim_t anim;
//             lv_anim_init(&anim);
//             lv_anim_set_var(&anim, guider_ui.screen_home_btn_ly);
//             lv_anim_set_exec_cb(&anim, anim_btn_ly_size);
//             lv_anim_set_values(&anim, 65, 60);
//             lv_anim_set_time(&anim, 200);
//             lv_anim_set_path_cb(&anim, lv_anim_path_linear);
//             lv_anim_start(&anim);
//         }
//         else
//         {
//             if(language == LANGUAGE_CN)
//             {
//                 lv_label_set_text(guider_ui.screen_home_btn_3_label, "\n已配对       ");
//             }else{
//                 lv_label_set_text(guider_ui.screen_home_btn_3_label, "\nON       ");
//             }
//             lv_label_set_text(guider_ui.screen_home_btn_ly_label, "");
//             lv_obj_set_style_text_font(guider_ui.screen_home_btn_ly, &lv_font_iconfont_blue_36, LV_PART_MAIN|LV_STATE_DEFAULT);
//             // lv_obj_set_style_text_font(guider_ui.screen_home_btn_ly, &lv_font_iconfont_40, LV_PART_MAIN|LV_STATE_DEFAULT);
//             lv_obj_set_style_bg_color(guider_ui.screen_home_btn_3, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
//             lv_obj_set_style_text_color(guider_ui.screen_home_btn_3, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
//             lv_obj_set_style_text_color(guider_ui.screen_home_btn_ly, lv_color_hex(0x57afff), LV_PART_MAIN|LV_STATE_DEFAULT);
//             lv_obj_set_style_bg_color(guider_ui.screen_home_btn_ly, lv_color_hex(0x404043), LV_PART_MAIN|LV_STATE_DEFAULT);

//             lv_anim_t anim;
//             lv_anim_init(&anim);
//             lv_anim_set_var(&anim, guider_ui.screen_home_btn_ly);
//             lv_anim_set_exec_cb(&anim, anim_btn_ly_size);
//             lv_anim_set_values(&anim, 60, 65);
//             lv_anim_set_time(&anim, 200);
//             lv_anim_set_path_cb(&anim, lv_anim_path_linear);
//             lv_anim_start(&anim);
//         }
//         save_begin();
//     }
// }

void setup_scr_screen_home(lv_ui *ui)
{
    //Write codes screen_home
    ui->screen_home = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_home, 480, 480);
    lv_obj_set_scrollbar_mode(ui->screen_home, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_home, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_home, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(ui->screen_home, LVGL_PATH(bg/home_bg.png), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_opa(ui->screen_home, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_recolor_opa(ui->screen_home, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_home_btn_1
    ui->screen_home_btn_1 = lv_button_create(ui->screen_home);
    lv_obj_set_pos(ui->screen_home_btn_1, 37, 82);
    lv_obj_set_size(ui->screen_home_btn_1, 418, 133);
    ui->screen_home_btn_1_label = lv_label_create(ui->screen_home_btn_1);
    lv_label_set_text(ui->screen_home_btn_1_label, "        开启");
    lv_label_set_long_mode(ui->screen_home_btn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_home_btn_1_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_home_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_home_btn_1_label, LV_PCT(100));

    //Write style for screen_home_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_home_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_home_btn_1, lv_color_hex(0xe21d48), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_home_btn_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_home_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_home_btn_1, 30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_home_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_home_btn_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_home_btn_1, &lv_font_Dengb_52, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_home_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_home_btn_1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);


//Write codes screen_home_btn_3
    ui->screen_home_btn_3 = lv_button_create(ui->screen_home);
    lv_obj_set_pos(ui->screen_home_btn_3, 34, 238);
    lv_obj_set_size(ui->screen_home_btn_3, 419, 113);
    ui->screen_home_btn_3_label = lv_label_create(ui->screen_home_btn_3);
    lv_label_set_text(ui->screen_home_btn_3_label, "\n待连接       ");
    lv_label_set_long_mode(ui->screen_home_btn_3_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_home_btn_3_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_home_btn_3, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_home_btn_3_label, LV_PCT(100));

//Write style for screen_home_btn_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_home_btn_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_home_btn_3, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_home_btn_3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_home_btn_3, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_home_btn_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_home_btn_3, lv_color_hex(0x414141), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_home_btn_3, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_home_btn_3, 30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_home_btn_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_home_btn_3, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_home_btn_3, &lv_font_Dengb_46, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_home_btn_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_home_btn_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);


    //Write codes screen_home_btn_4
    ui->screen_home_btn_4 = lv_button_create(ui->screen_home);
    lv_obj_set_pos(ui->screen_home_btn_4, 34, 385);
    lv_obj_set_size(ui->screen_home_btn_4, 420, 70);
    ui->screen_home_btn_4_label = lv_label_create(ui->screen_home_btn_4);
    lv_label_set_text(ui->screen_home_btn_4_label, "             高级设置");
    lv_label_set_long_mode(ui->screen_home_btn_4_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_home_btn_4_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_home_btn_4, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_home_btn_4_label, LV_PCT(100));

    //Write style for screen_home_btn_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_home_btn_4, 119, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_home_btn_4, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_home_btn_4, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_home_btn_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_home_btn_4, 27, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_home_btn_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_home_btn_4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_home_btn_4, &lv_font_Dengb_23, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_home_btn_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_home_btn_4, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);


    //Write codes screen_home_label_1
    ui->screen_home_label_1 = lv_label_create(ui->screen_home);
    lv_obj_set_pos(ui->screen_home_label_1, 411, 410);
    lv_obj_set_size(ui->screen_home_label_1, 26, 19);
    lv_label_set_text(ui->screen_home_label_1, " " LV_SYMBOL_RIGHT " ");
    lv_label_set_long_mode(ui->screen_home_label_1, LV_LABEL_LONG_WRAP);
    lv_obj_clear_flag(ui->screen_home_label_1, LV_OBJ_FLAG_CLICKABLE);

    //Write style for screen_home_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_home_label_1, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_home_label_1, &lv_font_Dengb_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_home_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_home_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_home_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_home_img_1
    // ui->screen_home_img_1 = lv_image_create(ui->screen_home);
    // lv_obj_set_pos(ui->screen_home_img_1, 59, 107);
    // lv_obj_set_size(ui->screen_home_img_1, 85, 85);
    // lv_image_set_src(ui->screen_home_img_1, LVGL_PATH(bg/home_btn.png));

    // //Write style for screen_home_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    // lv_obj_set_style_image_recolor_opa(ui->screen_home_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    // lv_obj_set_style_image_opa(ui->screen_home_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    // // 👉👉👉 在这里加圆角 + 裁剪（你要的圆角模式）
    // lv_obj_set_style_radius(ui->screen_home_img_1, 75, LV_PART_MAIN|LV_STATE_DEFAULT);    // 圆角大小
    // lv_obj_set_style_clip_corner(ui->screen_home_img_1, true, LV_PART_MAIN|LV_STATE_DEFAULT); // 开启裁剪

    //Write codes screen_home_img_2
    ui->screen_home_img_2 = lv_image_create(ui->screen_home);
    lv_obj_set_pos(ui->screen_home_img_2, 382, 123);
    lv_obj_set_size(ui->screen_home_img_2, 50, 50);
    lv_image_set_src(ui->screen_home_img_2, LVGL_PATH(bg/home_sd.png));

    //Write style for screen_home_img_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_home_img_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_home_img_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);



    //Write codes screen_home_label_ly
    ui->screen_home_label_ly = lv_label_create(ui->screen_home);
    lv_obj_set_pos(ui->screen_home_label_ly, 144, 259);
    lv_obj_set_size(ui->screen_home_label_ly, 285, 30);
    lv_label_set_text(ui->screen_home_label_ly, "蓝牙状态");
    lv_label_set_long_mode(ui->screen_home_label_ly, LV_LABEL_LONG_WRAP);

    //Write style for screen_home_label_ly, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_home_label_ly, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_home_label_ly, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_home_label_ly, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_home_label_ly, &lv_font_Dengb_27, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_home_label_ly, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_home_label_ly, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_home_label_ly, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_home_label_ly, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_home_label_ly, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_home_label_ly, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_home_label_ly, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_home_label_ly, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_home_label_ly, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_home_label_ly, 0, LV_PART_MAIN|LV_STATE_DEFAULT);



    //Write codes screen_home_btn_ly
    ui->screen_home_btn_ly = lv_button_create(ui->screen_home);
    lv_obj_set_pos(ui->screen_home_btn_ly, 59, 265);
    lv_obj_set_size(ui->screen_home_btn_ly, 60, 60);
    ui->screen_home_btn_ly_label = lv_label_create(ui->screen_home_btn_ly);
    lv_label_set_text(ui->screen_home_btn_ly_label, "");
    lv_label_set_long_mode(ui->screen_home_btn_ly_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_home_btn_ly_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_home_btn_ly, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_home_btn_ly_label, LV_PCT(100));

    //Write style for screen_home_btn_ly, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_home_btn_ly, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_home_btn_ly, lv_color_hex(0x262628), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_home_btn_ly, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_home_btn_ly, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_home_btn_ly, 13, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_home_btn_ly, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_home_btn_ly, lv_color_hex(0x57afff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_home_btn_ly, &lv_font_iconfont_blue_36, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_home_btn_ly, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_home_btn_ly, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_home_btn_power
    ui->screen_home_btn_power = lv_button_create(ui->screen_home);
    lv_obj_set_pos(ui->screen_home_btn_power, 58, 107);
    lv_obj_set_size(ui->screen_home_btn_power, 85, 85);
    ui->screen_home_btn_power_label = lv_label_create(ui->screen_home_btn_power);
    lv_label_set_text(ui->screen_home_btn_power_label, "");
    lv_label_set_long_mode(ui->screen_home_btn_power_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_home_btn_power_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_home_btn_power, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_home_btn_power_label, LV_PCT(100));
    //Write style for screen_home_btn_power, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_color(ui->screen_home_btn_power, lv_color_hex(0xe74a6d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_home_btn_power, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_home_btn_power, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_home_btn_power, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_home_btn_power, 85, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_home_btn_power, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_home_btn_power, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_home_btn_power, &lv_font_iconfont_power_36, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_home_btn_power, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_home_btn_power, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_home_btn_set
    ui->screen_home_btn_set = lv_button_create(ui->screen_home);
    lv_obj_set_pos(ui->screen_home_btn_set, 53, 398);
    lv_obj_set_size(ui->screen_home_btn_set, 47, 47);
    ui->screen_home_btn_set_label = lv_label_create(ui->screen_home_btn_set);
    lv_label_set_text(ui->screen_home_btn_set_label, "");
    lv_label_set_long_mode(ui->screen_home_btn_set_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_home_btn_set_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_home_btn_set, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_home_btn_set_label, LV_PCT(100));

    //Write style for screen_home_btn_set, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_home_btn_set, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_home_btn_set, lv_color_hex(0x262628), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_home_btn_set, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_home_btn_set, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_home_btn_set, 13, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_home_btn_set, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_home_btn_set, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_home_btn_set, &lv_font_iconfont_set_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_home_btn_set, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_home_btn_set, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);


     //Write codes screen_home_line_1
    ui->screen_home_line_1 = lv_line_create(ui->screen_home);
    lv_obj_set_pos(ui->screen_home_line_1, 85, 135);
    lv_obj_set_size(ui->screen_home_line_1, 43, 40);
    static lv_point_precise_t screen_home_line_1[] = {{0, 0},{33, 30}};
    lv_line_set_points(ui->screen_home_line_1, screen_home_line_1, 2);

    //Write style for screen_home_line_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_line_width(ui->screen_home_line_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->screen_home_line_1, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->screen_home_line_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->screen_home_line_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    NO_SHOW(ui->screen_home_line_1);
    lv_obj_clear_flag(ui->screen_home_btn_3, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui->screen_home_btn_ly, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui->screen_home_btn_set, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui->screen_home_btn_power, LV_OBJ_FLAG_CLICKABLE);
    // 对齐模式
    lv_image_set_inner_align(ui->screen_home_img_2, LV_IMAGE_ALIGN_CENTER);


    if(home_open_state)
    {
        if(language == LANGUAGE_CN) lv_label_set_text(ui->screen_home_btn_1_label, "        开启");
        else lv_label_set_text(ui->screen_home_btn_1_label, "        Turn On");
        NO_SHOW(guider_ui.screen_home_line_1);
        lv_obj_set_style_text_color(guider_ui.screen_home_btn_power, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui->screen_home_btn_1, lv_color_hex(0xe21d48), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui->screen_home_btn_power, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui->screen_home_btn_power, lv_color_hex(0xe74a6d), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_image_opa(ui->screen_home_img_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    else{
        if(language == LANGUAGE_CN) lv_label_set_text(ui->screen_home_btn_1_label, "        关闭");
        else lv_label_set_text(ui->screen_home_btn_1_label, "        Turn Off");
        SHOW(guider_ui.screen_home_line_1);
        lv_obj_set_style_text_color(guider_ui.screen_home_btn_power, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui->screen_home_btn_1, lv_color_hex(0x1b181a), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui->screen_home_btn_power, 140, LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui->screen_home_btn_power, lv_color_hex(0x3b383a), LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_image_opa(ui->screen_home_img_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    }

    if(g_ble_connected)
    {
        if(language == LANGUAGE_CN) lv_label_set_text(guider_ui.screen_home_btn_3_label, "\n已配对       ");
        else lv_label_set_text(guider_ui.screen_home_btn_3_label, "\nON       ");
            lv_label_set_text(guider_ui.screen_home_btn_ly_label, "");
            lv_obj_set_style_text_font(guider_ui.screen_home_btn_ly, &lv_font_iconfont_blue_36, LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(guider_ui.screen_home_btn_3, lv_color_hex(0x232324), LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(guider_ui.screen_home_btn_3, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(guider_ui.screen_home_btn_ly, lv_color_hex(0x57afff), LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(guider_ui.screen_home_btn_ly, lv_color_hex(0x404043), LV_PART_MAIN|LV_STATE_DEFAULT);
    }else{
        if(language == LANGUAGE_CN) lv_label_set_text(guider_ui.screen_home_btn_3_label, "\n待连接       ");
        else lv_label_set_text(guider_ui.screen_home_btn_3_label, "\nOFF       ");
            lv_label_set_text(guider_ui.screen_home_btn_ly_label, "");
            lv_obj_set_style_text_font(guider_ui.screen_home_btn_ly, &lv_font_iconfont_blue_32, LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(guider_ui.screen_home_btn_3, lv_color_hex(0x161618), LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(guider_ui.screen_home_btn_3, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(guider_ui.screen_home_btn_ly, lv_color_hex(0x64748b), LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(guider_ui.screen_home_btn_ly, lv_color_hex(0x262628), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    if(language == LANGUAGE_CN)
    {
        lv_label_set_text(ui->screen_home_label_ly, "蓝牙状态");
        lv_label_set_text(ui->screen_home_btn_4_label, "             高级设置");
    }else{

        lv_label_set_text(ui->screen_home_label_ly, "Bluetooth Status");
        lv_label_set_text(ui->screen_home_btn_4_label, "             Advanced Settings");
    }

    // 添加点击事件
    lv_obj_add_event_cb(ui->screen_home_btn_1, btn_1_event, LV_EVENT_CLICKED, ui);

    // lv_obj_add_event_cb(ui->screen_home_btn_3, btn_3_event, LV_EVENT_CLICKED, ui);


    if (timeout_timer == NULL)
        timeout_timer = lv_timer_create(timeout_callback, 1000, 0);
    else
        lv_timer_resume(timeout_timer);
    if (blue_state_timer == NULL)
        blue_state_timer = lv_timer_create(blue_state_callback, 2000, 0);
    else
        lv_timer_resume(blue_state_timer);

    //Update current screen layout.
    lv_obj_update_layout(ui->screen_home);
    events_init_screen_home(ui);
}
