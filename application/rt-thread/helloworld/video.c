#include "video.h"

// 播放器上下文结构体，用于存储播放器相关的各种状态和参数信息
struct lvgl_player_context
{
    int file_cnt;
    int file_index;
    int player_state;
    struct aic_player *player;
    int sync_flag;
    struct av_media_info media_info;
    int demuxer_detected_flag;
    int player_end;
    struct mpp_size screen_size;
    struct mpp_rect disp_rect;
};

volatile bool video_continue_flag = false;      // 视频继续播放
volatile static bool create_player_flag = true; // 视频播放器初始化标志
// 增加静态变量保存图层原始alpha配置
static struct aicfb_alpha_config ui_alpha_bak = {0};
static struct aicfb_alpha_config video_alpha_bak = {0};
static bool alpha_saved = false;
struct lvgl_player_context my_lvgl_player_ctx;
struct lvgl_player_context my_ctx;

static int set_disp_rect(struct lvgl_player_context *ctx)
{
    int ret = 0;

    if (!ctx->media_info.has_video)
    {
        return 0;
    }
    aic_player_get_screen_size(ctx->player, &ctx->screen_size);
    // 反转90度后视频宽高互换
    if (ctx->media_info.video_stream.width > VIDEO_WIDTH)
    {
        rt_kprintf("1\n");
        ctx->disp_rect.x = 0;
        ctx->disp_rect.width = VIDEO_WIDTH;
    }
    else
    {
        rt_kprintf("2\n");
        ctx->disp_rect.x = (ctx->screen_size.width - ctx->media_info.video_stream.width) / 2;
        ctx->disp_rect.width = ctx->media_info.video_stream.width;
    }

    if (ctx->media_info.video_stream.height > VIDEO_HEIGHT)
    {
        rt_kprintf("3\n");
        ctx->disp_rect.y = 0;
        ctx->disp_rect.height = VIDEO_HEIGHT;
    }
    else
    {
        rt_kprintf("4\n");
        ctx->disp_rect.y = (ctx->screen_size.height - ctx->media_info.video_stream.height) / 2;
        ctx->disp_rect.height = ctx->media_info.video_stream.height;
    }

    // ctx->disp_rect.x += ctx->disp_rect.x;

    ctx->disp_rect.x = VIDEO_DISP_X;
    ctx->disp_rect.y = VIDEO_DISP_Y;
    ctx->disp_rect.width = VIDEO_WIDTH;
    ctx->disp_rect.height = VIDEO_HEIGHT;

    printf("Media size %d x %d, Display offset (%d, %d) size %d x %d\n",
           ctx->media_info.video_stream.width, ctx->media_info.video_stream.height,
           ctx->disp_rect.x, ctx->disp_rect.y, ctx->disp_rect.width, ctx->disp_rect.height);

    ret = aic_player_set_disp_rect(ctx->player, &ctx->disp_rect);
    if (ret != 0)
    {
        printf("aic_player_set_disp_rect error\n");
        return -1;
    }
    return ret;
}

static s32 event_handle(void *app_data, s32 event, s32 data1, s32 data2)
{
    int ret = 0;
    struct lvgl_player_context *ctx = (struct lvgl_player_context *)app_data;

    switch (event)
    {
    case AIC_PLAYER_EVENT_PLAY_END:
        rt_kprintf("Play end\n");
        video_continue_flag = true;
        ctx->player_end = 1;
        break;
    default:
        break;
    }
    return ret;
}

// 播放函数，设置要播放的文件、准备、获取媒体信息、设置显示区域并开始播放
int lvgl_play(struct lvgl_player_context *ctx)
{
    int ret = 0;

    aic_player_set_uri(ctx->player, LVGL_FILE_LIST_PATH(sp.mp4));

    ctx->sync_flag = AIC_PLAYER_PREPARE_SYNC;
    if (ctx->sync_flag == AIC_PLAYER_PREPARE_ASYNC)
    {
        ret = aic_player_prepare_async(ctx->player);
    }
    else
    {
        ret = aic_player_prepare_sync(ctx->player);
    }
    if (ret)
    {
        return -1;
    }
    if (ctx->sync_flag == AIC_PLAYER_PREPARE_SYNC)
    {
        ret = aic_player_get_media_info(ctx->player, &ctx->media_info);
        if (ret != 0)
        {
            rt_kprintf("aic_player_get_media_info error\n");
            return -1;
        }
        ret = aic_player_start(ctx->player);
        if (ret != 0)
        {
            rt_kprintf("aic_player_start error\n");
            return -1;
        }
        ret = set_disp_rect(ctx);
        if (ret != 0)
        {
            rt_kprintf("set_disp_rect error\n");
            return -1;
        }
        ret = aic_player_play(ctx->player);
        if (ret != 0)
        {
            rt_kprintf("aic_player_play error\n");
            return -1;
        }
    }
    return 0;
}
// 暂停和恢复播放视频文件sadf
static int lvgl_pause(struct lvgl_player_context *ctx)
{
    return aic_player_pause(ctx->player);
}

int lvgl_stop(struct lvgl_player_context *ctx)
{
    return aic_player_stop(ctx->player);
}
int lvgl_play_next(struct lvgl_player_context *ctx)
{
    ctx->file_index++;
    ctx->file_index = (ctx->file_index > ctx->file_cnt - 1) ? 0 : ctx->file_index;
    lvgl_stop(ctx);
    if (lvgl_play(ctx) != 0)
    {
        return -1;
    }
    return 0;
}

void create_player()
{
    memset(&my_lvgl_player_ctx, 0x00, sizeof(struct lvgl_player_context));
    // lv_memset_00(&my_lvgl_player_ctx, sizeof(struct lvgl_player_context));
    my_lvgl_player_ctx.player = aic_player_create(NULL);
    if (my_lvgl_player_ctx.player == NULL)
    {
        rt_kprintf("视频创建失败，重启系统\n");
        // 调用重启系统函数（需根据实际系统环境实现）
        // wdt_immediate_reset();
        return;
    }
    rt_kprintf("视频播放器创建成功\n");
    // my_lvgl_player_ctx.file_cnt = sizeof(g_filename) / sizeof(g_filename[0]);
    my_lvgl_player_ctx.file_cnt = 1;
    my_lvgl_player_ctx.file_index = 0;
    my_lvgl_player_ctx.player_state = LVGL_PLAYER_STATE_STOP;
    aic_player_set_event_callback(my_lvgl_player_ctx.player, &my_lvgl_player_ctx, event_handle);
    if (lvgl_play_next(&my_lvgl_player_ctx) != 0)
    { // if play fail,it is considered play finsh.play the next one
        my_lvgl_player_ctx.player_state = LVGL_PLAYER_STATE_STOP;
        my_lvgl_player_ctx.player_end = 1;
    }
    else
    {
        my_lvgl_player_ctx.player_state = LVGL_PLAYER_STATE_PLAY;
        my_lvgl_player_ctx.player_end = 0;
    }
}
void video_init(bool play, uint8_t select)
{
    rt_device_t render_dev = RT_NULL;
    struct aicfb_alpha_config alpha = {0};

    render_dev = rt_device_find("aicfb");
    if (!render_dev)
    {
        rt_kprintf("rt_device_find aicfb failed!");
        return;
    }

    // 首次运行时保存原始alpha配置
    if (!alpha_saved)
    {
        ui_alpha_bak.layer_id = AICFB_LAYER_TYPE_UI;
        rt_device_control(render_dev, AICFB_GET_ALPHA_CONFIG, &ui_alpha_bak);

        video_alpha_bak.layer_id = AICFB_LAYER_TYPE_VIDEO;
        rt_device_control(render_dev, AICFB_GET_ALPHA_CONFIG, &video_alpha_bak);

        alpha_saved = true;
    }

    // UI层显示在视频层之上 (select=0)
    if (select == 0)
    {
        // 恢复UI层原始配置
        rt_device_control(render_dev, AICFB_UPDATE_ALPHA_CONFIG, &ui_alpha_bak);

        // 设置视频层alpha
        alpha.layer_id = AICFB_LAYER_TYPE_VIDEO;
        alpha.enable = 1;
        alpha.mode = 1;
        alpha.value = play ? 255 : 0;
        rt_device_control(render_dev, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    }
    // 只显示视频层 (select=1)
    else if (select == 1)
    {
        // 恢复视频层原始配置
        rt_device_control(render_dev, AICFB_UPDATE_ALPHA_CONFIG, &video_alpha_bak);

        // 设置UI层alpha
        alpha.layer_id = AICFB_LAYER_TYPE_UI;
        alpha.enable = 1;
        alpha.mode = 1;
        alpha.value = play ? 0 : 255;
        rt_device_control(render_dev, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    }

    // 初始化播放器
    if (create_player_flag)
    {
        create_player();
        create_player_flag = false;
    }
}
void seek_to_start_play_video(void)
{
    struct lvgl_player_context *ctx = &my_lvgl_player_ctx;
    ctx->player_end = 0;
    int ret;
    ret = aic_player_seek(ctx->player, 0);
    // aic_player_pause(ctx->player);
    // ret = avi_seek(ctx->player, 0);
    if (ret == 0)
    {
        // aic_player_play(ctx->player);
        rt_kprintf("seek success\n");
    }
    else if (ret == -1)
    {
        // destroy_player(PRINTF_SEEK);
    }
}

void video_thread_entry(void *parameter)
{
    video_init(true, 1);
    while (1)
    {
        if (video_continue_flag)
        {
            struct lvgl_player_context *ctx = &my_lvgl_player_ctx;
            if (ctx->player != NULL)
            {
                int state = aic_player_destroy(ctx->player);
                if (state == 0)
                {
                    rt_kprintf("视频播放器器销毁成功\n");
                    ctx->player = NULL;
                }
            }
            video_continue_flag = false;

            setup_scr_screen(&guider_ui);
            lv_scr_load_anim(guider_ui.screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
        }



        // if (video_continue_flag)
        // {
        //     video_continue_flag = false;

        //     seek_to_start_play_video();
        // }
        rt_thread_mdelay(1000);
    }
}
