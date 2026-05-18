#include "cfgsave.h"

volatile uint8_t save_num = 0;

/* 互斥锁：保护所有配置相关的全局变量 */
static rt_mutex_t cfg_mutex = NULL;

void cfgSave()
{
    FILE *cfgfile;
    char buff[save_byte] = {0};

    /* 初始化魔数 */
    buff[0] = 0x55;
    buff[1] = 0xAA;

    /* 加锁读取全局变量 */
    rt_mutex_take(cfg_mutex, RT_WAITING_FOREVER);
    buff[2] = language;
    buff[3] = light_value;
    buff[4] = screen_off_mode;
    buff[5] = home_open_state;
    buff[6] = home_blue_state;
    rt_mutex_release(cfg_mutex);

    /* 打开文件 */
    cfgfile = fopen("/data/config.bin", "wb");
    if (cfgfile == NULL)
    {
        rt_kprintf("[配置] 打开文件失败\n");
        return;
    }

    size_t bytes_written = fwrite(buff, 1, save_byte, cfgfile);
    if (bytes_written != save_byte)
    {
        rt_kprintf("[配置] 写入失败，只写入了 %d 字节\n", bytes_written);
        fclose(cfgfile);
        return;
    }
    /* 强制刷盘：确保数据真正写入Flash，断电不丢失 */
    fflush(cfgfile);
    fsync(fileno(cfgfile));
    /* 关闭文件 */
    fclose(cfgfile);
    rt_kprintf("config save ok\n");
}

void cfgRead()
{
    FILE *cfgfile;
    char buff[save_byte];
    size_t bytes_read;

    cfgfile = fopen("/data/config.bin", "rb");
    if (cfgfile == NULL)
    {
        rt_kprintf("no config found, using defaults\n");
        return;
    }

    // 修复：正确调用fread，读取16个字节
    bytes_read = fread(buff, 1, save_byte, cfgfile);
    fclose(cfgfile);

    rt_kprintf(">>>>>>>>>>%02x %02x %02x\n", buff[0] & 0xFF, buff[1] & 0xFF, buff[2] & 0xFF);

    // 验证读取的字节数和魔数
    if (bytes_read == save_byte && (buff[0] == 0x55) && (buff[1] == 0xAA))
    {
       /* 加锁写入全局变量 */
        rt_mutex_take(cfg_mutex, RT_WAITING_FOREVER);
        language = buff[2];
        light_value = buff[3];
        screen_off_mode = buff[4];
        home_open_state = buff[5];
        home_blue_state = buff[6];
        rt_mutex_release(cfg_mutex);

        rt_kprintf("Config loaded from file\n");
    }
}

void save_begin(void)
{
    rt_mutex_take(cfg_mutex, RT_WAITING_FOREVER);
    save_flag = true;
    save_num++;
    rt_mutex_release(cfg_mutex);
}

void cfgsave_thread_entry(void *parameter)
{
    static uint8_t save_counter = 0, last_save_num = 0;
    static bool timeout_logged = false; // 添加标志，确保只记录一次
        /* 初始化互斥锁 */
    cfg_mutex = rt_mutex_create("cfg_mutex", RT_IPC_FLAG_FIFO);
    if (cfg_mutex == NULL)
    {
        rt_kprintf("cfg_mutex create failed\n");
        return;
    }
    while (1)
    {
        rt_mutex_take(cfg_mutex, RT_WAITING_FOREVER);
        bool save_temp = save_flag;
        rt_mutex_release(cfg_mutex);
        if (save_temp)
        {
            if ((++save_counter) >= 1 && last_save_num == save_num)
            {
                cfgSave();
                rt_mutex_take(cfg_mutex, RT_WAITING_FOREVER);
                save_num = 0;
                save_counter = 0;
                save_flag = false;
                rt_mutex_release(cfg_mutex);
            }
            else if (last_save_num != save_num)
            {
                rt_mutex_take(cfg_mutex, RT_WAITING_FOREVER);
                save_counter = 0;
                last_save_num = save_num;
                rt_mutex_release(cfg_mutex);
            }
        }

        if(timeout_state && !timeout_logged)
        {
            backlight_off();
            timeout_logged = true; // 设置标志，表示已经记录过一次
            rt_kprintf("进入息屏状态\n");
        }else if(!timeout_state && timeout_logged)
        {
            rt_mutex_take(cfg_mutex, RT_WAITING_FOREVER);
            uint8_t current_light = light_value;
            rt_mutex_release(cfg_mutex);

            backlight_set(current_light);
            rt_kprintf("退出息屏状态\n");
            timeout_logged = false; // 恢复标志，允许下次记录
        }

        rt_thread_mdelay(500);
    }
}
