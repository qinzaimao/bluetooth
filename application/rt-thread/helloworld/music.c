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
    play_sem = rt_sem_create("music_play", 0, RT_IPC_FLAG_FIFO);
    if(play_sem != NULL ) rt_kprintf("[音乐] 播放触发信号量创建成功\n");
    else rt_kprintf("[音乐] 播放触发信号量创建失败\n");

    while (1)
    {
        rt_sem_take(play_sem, RT_WAITING_FOREVER);
        rt_sem_release(play_sem);

        music_create_player();
        music_set_volume(music_audio_player, 50);
        mini_audio_player_play(music_audio_player, music_filename[0]);
    }
}
