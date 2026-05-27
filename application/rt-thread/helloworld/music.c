#include "music.h"

static char music_filename[][256] = {
    MUSIC_PATH(sun.mp3),         // 造福人类
    MUSIC_PATH(sun_en.mp3),      // 造福人类
    MUSIC_PATH(blue.mp3),        // 蓝牙已连接
    MUSIC_PATH(blue_en.mp3),     // 蓝牙已连接
    MUSIC_PATH(starting.mp3),    // 启动灯光
    MUSIC_PATH(starting_en.mp3), // 启动灯光
    MUSIC_PATH(renew.mp3),       // 重启
    MUSIC_PATH(renew_en.mp3)     // 重启
};

struct mini_audio_player *music_audio_player = NULL; // 音乐播放器

// ====================== 新增：播放触发信号量 ======================
static rt_sem_t play_sem = NULL;

uint32_t pin_sound = 0;
uint32_t pin_en = 0;

//只控制视频的声音，不控制系统声音
void Sound_Init(uint8_t level)
{
    pin_sound = rt_pin_get("PE.12");
    rt_pin_mode(pin_sound, PIN_MODE_OUTPUT);
    rt_pin_write(pin_sound, level);
    pin_en = rt_pin_get("PA.4");
    rt_pin_mode(pin_en, PIN_MODE_OUTPUT);
    rt_pin_write(pin_en, level);
}

int music_set_volume(struct mini_audio_player *player, int vol)
{
    if (vol < 0)
    {
        vol = 0;
    }
    if (vol > 100)
    {
        vol = 100;
    }
    player->volume = vol;
    if (player->render)
    {
        aic_audio_render_control(player->render, AUDIO_RENDER_CMD_SET_VOL, &player->volume);
    }
    return 0;
}

//销毁音乐播放器
static void music_destroy_player(void)
{
    if (!music_audio_player) return;

    mini_audio_player_stop(music_audio_player);
    int ret = mini_audio_player_destroy(music_audio_player);
    if (ret == 0) {
        rt_kprintf("[音乐] 播放器销毁成功\n");
        music_audio_player = NULL;
    } else {
        rt_kprintf("[音乐] 播放器销毁失败\n");
    }
}

//创建音乐播放器
static void music_create_player(void)
{
    if (music_audio_player) return;

    music_audio_player = mini_audio_player_create();
    if (music_audio_player) {
        rt_kprintf("[音乐] 播放器创建成功\n");
        // 初始化音量
    } else {
        rt_kprintf("[音乐] 播放器创建失败\n");
    }
}


void music_thread_entry(void *parameter)
{
    static uint8_t temp = 0;
    static uint8_t last_music_num = 10;
    Sound_Init(PIN_HIGH);
    rt_thread_delay(1000);
    rt_kprintf("[音乐] 播放器线程启动\n");
    music_create_player();
    music_set_volume(music_audio_player, 80);
    while (1)
    {
        if(music_select)
        {
            if ((mini_audio_player_get_state(music_audio_player) == MINI_AUDIO_PLAYER_STATE_STOPED ||
            mini_audio_player_get_state(music_audio_player) == MINI_AUDIO_PLAYER_STATE_INIT))
            {
                mini_audio_player_play(music_audio_player, music_filename[music_select - 1]);
                music_select = MUSIC_NONE;
            }
        }


        // if ((mini_audio_player_get_state(music_audio_player) == MINI_AUDIO_PLAYER_STATE_STOPED ||
        // mini_audio_player_get_state(music_audio_player) == MINI_AUDIO_PLAYER_STATE_INIT))
        // {
        //     temp ++;
        //     mini_audio_player_play(music_audio_player, music_filename[temp % 8]);
        // }


        rt_thread_delay(200);

    }
}
