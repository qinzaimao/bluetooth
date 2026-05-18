/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"


typedef struct
{

	lv_obj_t *screen;
	bool screen_del;
	lv_obj_t *screen_label_blue;
	lv_obj_t *screen_btn_1;
	lv_obj_t *screen_btn_1_label;
	lv_obj_t *screen_btn_2;
	lv_obj_t *screen_btn_2_label;
	lv_obj_t *screen_btn_3;
	lv_obj_t *screen_btn_3_label;
	lv_obj_t *screen_btn_4;
	lv_obj_t *screen_btn_4_label;
	lv_obj_t *screen_blue;
	bool screen_blue_del;
	lv_obj_t *screen_power;
	bool screen_power_del;
	lv_obj_t *screen_power_label_progress;
	lv_obj_t *screen_power_bar_progress;
	lv_obj_t *screen_home;
	bool screen_home_del;
	lv_obj_t *screen_home_btn_1;
	lv_obj_t *screen_home_btn_1_label;
	lv_obj_t *screen_home_btn_2;
	lv_obj_t *screen_home_btn_2_label;
	lv_obj_t *screen_home_btn_3;
	lv_obj_t *screen_home_btn_3_label;
	lv_obj_t *screen_home_btn_4;
	lv_obj_t *screen_home_btn_4_label;
	lv_obj_t *screen_home_label_1;
	lv_obj_t *screen_home_label_ly;
	lv_obj_t *screen_home_label_fz;
	lv_obj_t *screen_home_img_1;
	lv_obj_t *screen_home_img_bg;
	lv_obj_t *screen_home_img_2;
	lv_obj_t *screen_home_btn_fz;
	lv_obj_t *screen_home_btn_fz_label;
	lv_obj_t *screen_home_btn_ly;
	lv_obj_t *screen_home_btn_ly_label;
	lv_obj_t *screen_home_btn_power;
	lv_obj_t *screen_home_btn_power_label;
	lv_obj_t *screen_home_btn_set;
	lv_obj_t *screen_home_btn_set_label;
	lv_obj_t *screen_home_line_1;
	lv_obj_t *screen_system_set;
bool screen_system_set_del;
	lv_obj_t *screen_system_set_btn_1;
	lv_obj_t *screen_system_set_btn_1_label;
	lv_obj_t *screen_system_set_btn_2;
	lv_obj_t *screen_system_set_btn_2_label;
	lv_obj_t *screen_system_set_btn_3;
	lv_obj_t *screen_system_set_btn_3_label;
	lv_obj_t *screen_system_set_btn_4;
	lv_obj_t *screen_system_set_btn_4_label;
	lv_obj_t *screen_system_set_btn_5;
	lv_obj_t *screen_system_set_btn_5_label;
	lv_obj_t *screen_system_set_btn_return;
	lv_obj_t *screen_system_set_btn_return_label;
	lv_obj_t *screen_system_set_label_return;
	lv_obj_t *screen_system_set_label_1;
	lv_obj_t *screen_system_set_label_2;
	lv_obj_t *screen_system_set_label_3;
	lv_obj_t *screen_system_set_label_4;
	lv_obj_t *screen_system_set_label_5;
	lv_obj_t *screen_system_set_label_set_top;
	lv_obj_t *screen_system_set_label_icon_1;
	lv_obj_t *screen_system_set_label_icon_2;
	lv_obj_t *screen_system_set_label_icon_3;
	lv_obj_t *screen_system_set_label_icon_4;
	lv_obj_t *screen_system_set_label_icon_5;
	lv_obj_t *screen_system_set_tileview_1;
	lv_obj_t *screen_system_set_tileview_1_tile_screen_set;
	lv_obj_t *screen_system_set_img_screen_bg;
	lv_obj_t *screen_system_set_label_screen_b2;
	lv_obj_t *screen_system_set_btn_screen_30sec;
	lv_obj_t *screen_system_set_btn_screen_30sec_label;
	lv_obj_t *screen_system_set_btn_screen_1min;
	lv_obj_t *screen_system_set_btn_screen_1min_label;
	lv_obj_t *screen_system_set_btn_screen_5min;
	lv_obj_t *screen_system_set_btn_screen_5min_label;
	lv_obj_t *screen_system_set_slider_screen_light;
	lv_obj_t *screen_system_set_label_screen_light;
	lv_obj_t *screen_system_set_btn_screen_back;
	lv_obj_t *screen_system_set_btn_screen_back_label;
	lv_obj_t *screen_system_set_label_screen_show;
	lv_obj_t *screen_system_set_label_screen_auto;
	lv_obj_t *screen_system_set_label_screen_top;
	lv_obj_t *screen_system_set_label_screen_i1;
	lv_obj_t *screen_system_set_label_screen_i2;
	lv_obj_t *screen_system_set_label_screen_i3;
	lv_obj_t *screen_system_set_tileview_2;
	lv_obj_t *screen_system_set_tileview_2_tile_language_set;
	lv_obj_t *screen_system_set_img_language_bg;
	lv_obj_t *screen_system_set_btn_language_en;
	lv_obj_t *screen_system_set_btn_language_en_label;
	lv_obj_t *screen_system_set_btn_language_cn;
	lv_obj_t *screen_system_set_btn_language_cn_label;
	lv_obj_t *screen_system_set_label_language_ok;
	lv_obj_t *screen_system_set_btn_language_back;
	lv_obj_t *screen_system_set_btn_language_back_label;
	lv_obj_t *screen_system_set_label_language_b1;
	lv_obj_t *screen_system_set_label_language_top;
	lv_obj_t *screen_system_set_label_language_i1;
	lv_obj_t *screen_system_set_tileview_3;
	lv_obj_t *screen_system_set_tileview_3_tile_product_info;
	lv_obj_t *screen_system_set_img_info_bg;
	lv_obj_t *screen_system_set_label_info_b1;
	lv_obj_t *screen_system_set_label_info_top;
	lv_obj_t *screen_system_set_label_info_1;
	lv_obj_t *screen_system_set_label_info_2;
	lv_obj_t *screen_system_set_label_info_3;
	lv_obj_t *screen_system_set_label_info_4;
	lv_obj_t *screen_system_set_label_info_5;
	lv_obj_t *screen_system_set_label_info_6;
	lv_obj_t *screen_system_set_label_info_7;
	lv_obj_t *screen_system_set_btn_info_back;
	lv_obj_t *screen_system_set_btn_info_back_label;
	lv_obj_t *screen_system_set_label_info_l1;
	lv_obj_t *screen_system_set_label_info_l2;
	lv_obj_t *screen_system_set_label_info_l3;
	lv_obj_t *screen_system_set_label_info_l4;
	lv_obj_t *screen_system_set_label_info_i1;
	lv_obj_t *screen_system_set_label_info_i2;
	lv_obj_t *screen_system_set_label_info_i3;
	lv_obj_t *screen_system_set_label_info_i4;
	lv_obj_t *screen_system_set_tileview_4;
	lv_obj_t *screen_system_set_tileview_4_tile_after;
	lv_obj_t *screen_system_set_img_after_bg;
	lv_obj_t *screen_system_set_btn_after_back;
	lv_obj_t *screen_system_set_btn_after_back_label;
		lv_obj_t *screen_system_set_tileview_5;
	lv_obj_t *screen_system_set_tileview_5_tile_blue;
	lv_obj_t *screen_system_set_img_blue_bg;
	lv_obj_t *screen_system_set_label_blue_b2;
	lv_obj_t *screen_system_set_label_blue_b1;
	lv_obj_t *screen_system_set_label_blue_b3;
	lv_obj_t *screen_system_set_label_blue_1;
	lv_obj_t *screen_system_set_label_blue_2;
	lv_obj_t *screen_system_set_arc_blue_1;
	lv_obj_t *screen_system_set_label_blue_3;
	lv_obj_t *screen_system_set_label_blue_4;
	lv_obj_t *screen_system_set_label_blue_5;
	lv_obj_t *screen_system_set_label_blue_i1;
	lv_obj_t *screen_system_set_label_blue_i2;
	lv_obj_t *screen_system_set_label_blue_i3;
	lv_obj_t *screen_system_set_label_blue_i4;
	lv_obj_t *screen_system_set_btn_blue_back;
	lv_obj_t *screen_system_set_btn_blue_back_label;
	lv_obj_t *screen_system_set_btn_blue_connect;
	lv_obj_t *screen_system_set_btn_blue_connect_label;
}lv_ui;

typedef void (*ui_setup_scr_t)(lv_ui * ui);

void ui_init_style(lv_style_t * style);

void ui_load_scr_animation(lv_ui *ui, lv_obj_t ** new_scr, bool new_scr_del, bool * old_scr_del, ui_setup_scr_t setup_scr,
                           lv_screen_load_anim_t anim_type, uint32_t time, uint32_t delay, bool is_clean, bool auto_del);

void ui_animation(void * var, uint32_t duration, int32_t delay, int32_t start_value, int32_t end_value, lv_anim_path_cb_t path_cb,
                  uint32_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                  lv_anim_exec_xcb_t exec_cb, lv_anim_start_cb_t start_cb, lv_anim_completed_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);


void init_scr_del_flag(lv_ui *ui);

void setup_bottom_layer(void);

void setup_ui(lv_ui *ui);

void video_play(lv_ui *ui);

void init_keyboard(lv_ui *ui);

extern lv_ui guider_ui;


void setup_scr_screen(lv_ui *ui);
void setup_scr_screen_blue(lv_ui *ui);
void setup_scr_screen_power(lv_ui *ui);
void setup_scr_screen_home(lv_ui *ui);
void setup_scr_screen_system_set(lv_ui *ui);


LV_FONT_DECLARE(lv_font_Dengb_13)
LV_FONT_DECLARE(lv_font_Dengb_18)
LV_FONT_DECLARE(lv_font_Dengb_20)
LV_FONT_DECLARE(lv_font_Dengb_21)
LV_FONT_DECLARE(lv_font_Dengb_23)
LV_FONT_DECLARE(lv_font_Dengb_24)
LV_FONT_DECLARE(lv_font_Dengb_27)
LV_FONT_DECLARE(lv_font_Dengb_46)
LV_FONT_DECLARE(lv_font_Dengb_52)
LV_FONT_DECLARE(lv_font_iconfont_40)
LV_FONT_DECLARE(lv_font_iconfont_blue_32)
LV_FONT_DECLARE(lv_font_iconfont_blue_36)
LV_FONT_DECLARE(lv_font_iconfont_set_32)
LV_FONT_DECLARE(lv_font_iconfont_power_36)
LV_FONT_DECLARE(lv_font_iconfont_sys_set_16)
LV_FONT_DECLARE(lv_font_iconfont_sys_set_20)
LV_FONT_DECLARE(lv_font_iconfont_sys_set_24)
LV_FONT_DECLARE(lv_font_iconfont_sys_set_50)
LV_FONT_DECLARE(lv_font_iconfont_info_20)
LV_FONT_DECLARE(lv_font_iconfont_screen_22)


#ifdef __cplusplus
}
#endif
#endif
