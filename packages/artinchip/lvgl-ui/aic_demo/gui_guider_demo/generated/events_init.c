/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "events_init.h"
#include <stdio.h>
#include "lvgl.h"
#include "D:\new_SDK\bluetouch\luban-lite-master\application\rt-thread\helloworld\main.h"
#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif


static void screen_home_btn_4_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_timer_pause(blue_state_timer);
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_system_set, guider_ui.screen_system_set_del, &guider_ui.screen_home_del, setup_scr_screen_system_set, LV_SCR_LOAD_ANIM_NONE, 0, 0, true, true);
        break;
    }
    default:
        break;
    }
}

void events_init_screen_home (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_home_btn_4, screen_home_btn_4_event_handler, LV_EVENT_ALL, ui);
}

static void screen_system_set_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        goto_blue = true;
        lv_obj_set_x(guider_ui.screen_system_set_tileview_5, 0);
        break;
    }
    default:
        break;
    }
}

static void screen_system_set_btn_2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_set_x(guider_ui.screen_system_set_tileview_1, 0);
        break;
    }
    default:
        break;
    }
}

static void screen_system_set_btn_3_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_set_x(guider_ui.screen_system_set_tileview_2, 0);
        break;
    }
    default:
        break;
    }
}

static void screen_system_set_btn_4_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_set_x(guider_ui.screen_system_set_tileview_3, 0);
        break;
    }
    default:
        break;
    }
}

static void screen_system_set_btn_5_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_set_x(guider_ui.screen_system_set_tileview_4, 0);
        break;
    }
    default:
        break;
    }
}

static void screen_system_set_btn_return_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_home, guider_ui.screen_home_del, &guider_ui.screen_system_set_del, setup_scr_screen_home, LV_SCR_LOAD_ANIM_NONE, 0, 0, true, true);
        break;
    }
    default:
        break;
    }
}

static void screen_system_set_btn_screen_back_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_set_x(guider_ui.screen_system_set_tileview_1, 481);
        break;
    }
    default:
        break;
    }
}

static void screen_system_set_btn_language_back_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_set_x(guider_ui.screen_system_set_tileview_2, 481);
        break;
    }
    default:
        break;
    }
}

static void screen_system_set_btn_info_back_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_set_x(guider_ui.screen_system_set_tileview_3, 481);
        break;
    }
    default:
        break;
    }
}

static void screen_system_set_btn_after_back_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_set_x(guider_ui.screen_system_set_tileview_4, 481);
        break;
    }
    default:
        break;
    }
}

static void screen_system_set_btn_blue_back_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        goto_blue = false;
        lv_timer_pause(blue_timer);
        lv_timer_pause(arc_timer);
        lv_obj_set_x(guider_ui.screen_system_set_tileview_5, -481);
        break;
    }
    default:
        break;
    }
}

void events_init_screen_system_set (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_system_set_btn_1, screen_system_set_btn_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_system_set_btn_2, screen_system_set_btn_2_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_system_set_btn_3, screen_system_set_btn_3_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_system_set_btn_4, screen_system_set_btn_4_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_system_set_btn_5, screen_system_set_btn_5_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_system_set_btn_return, screen_system_set_btn_return_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_system_set_btn_screen_back, screen_system_set_btn_screen_back_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_system_set_btn_language_back, screen_system_set_btn_language_back_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_system_set_btn_info_back, screen_system_set_btn_info_back_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_system_set_btn_after_back, screen_system_set_btn_after_back_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_system_set_btn_blue_back, screen_system_set_btn_blue_back_event_handler, LV_EVENT_ALL, ui);
}


void events_init(lv_ui *ui)
{

}
