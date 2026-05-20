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

lv_timer_t *power_timer = NULL;
static int32_t current_percent = 0;

static void power_callback(lv_timer_t *timer)
{
    if(current_percent < 100)
    {
        current_percent++;

        lv_bar_set_value(guider_ui.screen_power_bar_progress, current_percent, LV_ANIM_OFF);

        // 更新百分比文本显示
        char buf[16];
        lv_snprintf(buf, sizeof(buf), "%d%%", current_percent);
        lv_label_set_text(guider_ui.screen_power_label_progress, buf);
    }
    #if POWER_ON_MODE == 1
    // 到100%时停止定时器
    if (current_percent >= 100) {
        power_connect_flag = true;
    }
    rt_mutex_take(goto_mutex, RT_WAITING_FOREVER);
    bool goto_home_temp = goto_home_flag;
    if(goto_home_temp) goto_home_flag = false;
    rt_mutex_release(goto_mutex);

    if(goto_home_temp)
    {
        lv_timer_del(timer);
        setup_scr_screen_home(&guider_ui);
        lv_scr_load_anim(guider_ui.screen_home, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
    }
    #else
        if (current_percent >= 100) {
            lv_timer_del(timer);
            setup_scr_screen_home(&guider_ui);
            lv_scr_load_anim(guider_ui.screen_home, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
        }
    #endif
}

void setup_scr_screen_power(lv_ui *ui)
{
    //Write codes screen_power
    ui->screen_power = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_power, 480, 480);
    lv_obj_set_scrollbar_mode(ui->screen_power, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_power, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_power, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    if(language == LANGUAGE_CN)
    {
        lv_obj_set_style_bg_image_src(ui->screen_power, LVGL_PATH(bg/bg.png), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    else
    {
        lv_obj_set_style_bg_image_src(ui->screen_power, LVGL_PATH(bg/bg_en.png), LV_PART_MAIN|LV_STATE_DEFAULT);
    }
    lv_obj_set_style_bg_image_opa(ui->screen_power, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_recolor_opa(ui->screen_power, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_power_label_progress
    ui->screen_power_label_progress = lv_label_create(ui->screen_power);
    lv_obj_set_pos(ui->screen_power_label_progress, 347, 443);
    lv_obj_set_size(ui->screen_power_label_progress, 80, 21);
    lv_label_set_text(ui->screen_power_label_progress, " ");
    lv_label_set_long_mode(ui->screen_power_label_progress, LV_LABEL_LONG_CLIP);

    //Write style for screen_power_label_progress, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_power_label_progress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_power_label_progress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_power_label_progress, lv_color_hex(0x7b8fae), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_power_label_progress, &lv_font_Dengb_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_power_label_progress, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_power_label_progress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_power_label_progress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_power_label_progress, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_power_label_progress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_power_label_progress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_power_label_progress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_power_label_progress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_power_label_progress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_power_label_progress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_power_bar_progress
    ui->screen_power_bar_progress = lv_bar_create(ui->screen_power);
    lv_obj_set_pos(ui->screen_power_bar_progress, 40, 424);
    lv_obj_set_size(ui->screen_power_bar_progress, 390, 2);
    lv_obj_set_style_anim_duration(ui->screen_power_bar_progress, 2500, 0);
    lv_bar_set_mode(ui->screen_power_bar_progress, LV_BAR_MODE_NORMAL);
    lv_bar_set_range(ui->screen_power_bar_progress, 0, 100);
    lv_bar_set_value(ui->screen_power_bar_progress, 0, LV_ANIM_OFF);

    //Write style for screen_power_bar_progress, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_power_bar_progress, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_power_bar_progress, lv_color_hex(0xffc522), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_power_bar_progress, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_power_bar_progress, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_power_bar_progress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_power_bar_progress, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_power_bar_progress, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_power_bar_progress, lv_color_hex(0xd78810), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_power_bar_progress, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_power_bar_progress, 10, LV_PART_INDICATOR|LV_STATE_DEFAULT);


    //The custom code of screen_power.
    if (power_timer == NULL)
        power_timer = lv_timer_create(power_callback, 25, 0);
    else
        lv_timer_resume(power_timer);

    //Update current screen layout.
    lv_obj_update_layout(ui->screen_power);

}
