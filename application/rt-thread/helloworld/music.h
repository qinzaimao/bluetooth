#ifndef __MUSIC_H__
#define __MUSIC_H__


#include "main.h"

struct my_aic_audio_frame {
    s32  sample_rate;
    s32  bits_per_sample;
    s32  channels;
    s64  pts;
    s32  id;
    void  *data;
    u32  size;
    u32  flag;
};

struct my_aic_audio_decode_config {
    s32 packet_buffer_size;				// video bytestream size
    s32 packet_count;				// packet buffer count
    s32 frame_count;				// packet buffer count
};

struct music_audio_info {
    s64 file_size;
    s64 duration;
    s32 nb_channel;
    s32 bits_per_sample;
    s32 sample_rate;
};

struct mini_audio_player {
    char uri[128];
    int type;
    int fd;
    int state;
    int force_stop;
    int volume;
    char *wav_buff;
    int wav_buff_size;
    struct my_aic_audio_frame frame_info;
    struct aic_audio_decoder *decoder;
    struct my_aic_audio_decode_config dec_cfg;
    struct aic_audio_render *render;
    struct aic_parser *parser;
    struct music_audio_info audio_info;
    aicos_thread_t tid;
    aicos_queue_t mq;
    aicos_mutex_t lock;
    aicos_sem_t sem_thread_exit;
    aicos_sem_t sem_ack;
    aicos_sem_t stop_ack;
    int loop;
};

void music_thread_entry(void *parameter);


#endif /*__MUSIC_H__*/
