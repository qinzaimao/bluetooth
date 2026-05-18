#include "my_uart.h"
#include <rtthread.h>
#include <string.h>

#define SAMPLE_UART_NAME "uart3"
#define UART_RX_BUFFER_SIZE 256

static rt_device_t uart_dev = RT_NULL;
static struct rt_semaphore rx_sem;
static uint8_t line_buf[UART_RX_BUFFER_SIZE];
static uint16_t line_len = 0;

// ================= 蓝牙：只存储 1 个 qin 开头设备 =================

// 标记是否有效（1=有qin设备，0=无）

// ===================== 蓝牙连接状态 =====================
static char ble_recv_data[64] = {0};
static char ble_recv_mac[20] = {0};

bool is_ble_connected(void)
{
    return g_ble_connected;
}

void reset_ble_connect_status(void)
{
    g_ble_connected = false;
}

char *get_connected_mac(void)
{
    return ble_mac;
}

char *get_ble_received_data(void)
{
    return ble_recv_data;
}

// ===================== 【修改版】只存 qin 开头，只存一个 =====================
void ble_enqueue(const char *mac, const char *name)
{
    // ======================
    // 过滤：不是 qin 开头直接丢掉
    // ======================
    if (name == NULL || strncmp(name, "qin", 3) != 0)
    {
        return;
    }

    // ======================
    // 只保存这一个（覆盖）
    // ======================
    strcpy(g_ble_queue.mac, mac);
    strcpy(g_ble_queue.name, name);
    g_ble_valid = true; // 标记有效
}

// ===================== 串口底层 =====================
static rt_err_t uart_recv_callback(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&rx_sem);
    return RT_EOK;
}

void uart_send_data(uint8_t *data, uint16_t len)
{
    if (uart_dev && data && len)
    {
        rt_device_write(uart_dev, 0, data, len);
    }
}

void uart_send_at(const char *at)
{
    rt_err_t level;

    if (uart_dev == RT_NULL || at == RT_NULL)
        return;

    level = rt_hw_interrupt_disable();
    rt_device_write(uart_dev, 0, at, strlen(at));
    rt_thread_mdelay(1);
    rt_hw_interrupt_enable(level);
}

// ===================== 解析函数（不变） =====================
void parse_scan_result(const char *line)
{
    if (strstr(line, "AT-CONNECTED:") != NULL)
    {
        sscanf(line, "AT-CONNECTED:%s", ble_mac);
        rt_kprintf("[蓝牙] 连接成功！%s\n", line);
        g_ble_connected = true;
        return;
    }

    if (strstr(line, "AT-DISCONNECTED") != NULL)
    {
        rt_kprintf("[蓝牙] 断开连接\n");
        g_ble_connected = false;
        return;
    }

    if (strstr(line, "AT+ER:") != NULL && ble_con_begin)
    {
        rt_kprintf("[蓝牙] 连接失败 AT+ER:，等待重连...\n");
        g_ble_connected = false;
        ble_con_begin = false;
        return;
    }

    if (strstr(line, "AT-RDAT:") != NULL)
    {
        char len_str[8] = {0};
        sscanf(line, "AT-RDAT:%[^,],%[^,],%[^\r\n]", ble_recv_mac, len_str, ble_recv_data);
        rt_kprintf("[蓝牙数据] 来自:%s  内容:%s\n", ble_recv_mac, ble_recv_data);

        if (strcmp(ble_recv_data, "hello!") == 0)
        {
            rt_pin_write(gpio_fan_pin, PIN_HIGH);
        }
        else if (strcmp(ble_recv_data, "nihao!") == 0)
        {
            rt_pin_write(gpio_fan_pin, PIN_LOW);
        }
        return;
    }

    if (strstr(line, "AT-SCANRP:") == NULL)
        return;

    char mac[32] = {0};
    char name[32] = {0};

    if (sscanf(line, "AT-SCANRP:%[^,],%*[^,],%*[^,],%*[^,],%[^,]", mac, name) == 2)
    {
        // ======================
        // 只解析 qin 开头！！！
        // 不是直接丢掉，不解析、不存、不占CPU！
        // ======================
        if (strncmp(name, "qin", 3) != 0)
        {
            return; // 直接丢掉！速度暴快！
        }

        rt_kprintf("[解析] qin设备: %s -> %s\n", mac, name);
        ble_enqueue(mac, name);
    }
}

// ===================== 线程 =====================
void uart_thread_entry(void *parameter)
{
    uint8_t ch;
    uart_dev = rt_device_find(SAMPLE_UART_NAME);
    if (!uart_dev)
    {
        rt_kprintf("[UART] 找不到设备\n");
        return;
    }
    rt_kprintf("[UART] 设备找到，初始化中...\n");
    rt_sem_init(&rx_sem, "uart_rx", 0, RT_IPC_FLAG_FIFO);
    rt_device_open(uart_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    rt_device_set_rx_indicate(uart_dev, uart_recv_callback);

    while (1)
    {
        rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
        while (rt_device_read(uart_dev, 0, &ch, 1) == 1)
        {
            if (ch == '\r' || ch == '\n')
            {
                if (line_len > 0)
                {
                    line_buf[line_len] = '\0';
                    rt_kprintf("[接收完整行] %s\n", line_buf);
                    parse_scan_result((char *)line_buf);
                    line_len = 0;
                }
            }
            else
            {
                if (line_len < UART_RX_BUFFER_SIZE - 1)
                    line_buf[line_len++] = ch;
            }
        }
    }
}
