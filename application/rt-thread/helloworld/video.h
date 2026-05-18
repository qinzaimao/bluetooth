#ifndef VIDEO_H
#define VIDEO_H


#include "main.h"

#define LVGL_PLAYER_STATE_PLAY 1
#define LVGL_PLAYER_STATE_PAUSE 2
#define LVGL_PLAYER_STATE_STOP 3



#define VIDEO_WIDTH 480
#define VIDEO_HEIGHT 480

#define VIDEO_DISP_X 0
#define VIDEO_DISP_Y 0


void video_thread_entry(void *parameter);

#endif
