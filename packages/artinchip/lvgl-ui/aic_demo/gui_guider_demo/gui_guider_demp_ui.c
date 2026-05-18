#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "lvgl.h"
#include "aic_ui.h"
#include "lv_port_disp.h"
#include "mpp_fb.h"
#include "meter_ui.h"

/* gui guider head file */
#include "./custom/custom.h"
#include "./generated/gui_guider.h"
#include "./generated/events_init.h"
#include "./generated/widgets_init.h"
#include "./generated/guider_customer_fonts/guider_customer_fonts.h"
#include "D:\new_SDK\bluetouch\luban-lite-master\application\rt-thread\helloworld\main.h"

lv_ui guider_ui;

void gui_guider_demo_ui_init()
{
	/* 用户APP 入口 */
    setup_ui(&guider_ui);
    events_init(&guider_ui);
    custom_init(&guider_ui);
}
