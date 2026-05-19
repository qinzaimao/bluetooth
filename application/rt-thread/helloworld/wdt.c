#include "wdt.h"

static rt_device_t wdt_dev = RT_NULL;
static rt_timer_t wdt_feed_timer = RT_NULL;

/* 定时器喂狗回调 */
static void wdt_feed_callback(void *parameter)
{
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL);
    // rt_kprintf("[WDT] Feed at tick %d\n", rt_tick_get());
}

/* 看门狗中断回调 */
static irqreturn_t aic_wdt_irq(int irq, void *arg)
{
    rt_kprintf("[WDT] Pretimeout IRQ! Feeding and trying to recover...\n");
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL);
    return IRQ_HANDLED;
}

/* 空闲线程钩子 */
void idle_hook(void)
{
    static rt_tick_t last_idle_tick = 0;
    rt_tick_t current_tick = rt_tick_get();

    // 仅在空闲时间足够长时喂狗，避免频繁操作
    if (current_tick - last_idle_tick > 100)
    { // 约100ms
        rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL);
        last_idle_tick = current_tick;
    }
}

/* 立即触发看门狗复位函数 */
void wdt_immediate_reset(void)
{
    // 检查看门狗设备是否初始化
    if (wdt_dev == RT_NULL)
    {
        rt_kprintf("[WDT] Device not initialized, cannot reset!\n");
        return;
    }

    rt_kprintf("[WDT] Triggering immediate reset...\n");

    // 1. 禁用预超时中断，防止中断处理函数喂狗
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_IRQ_DISABLE, NULL);

    // 2. 停止并删除喂狗定时器
    if (wdt_feed_timer != RT_NULL)
    {
        rt_timer_stop(wdt_feed_timer);
        rt_timer_delete(wdt_feed_timer);
        wdt_feed_timer = RT_NULL;
    }

    // 3. 注销空闲线程的喂狗钩子
    rt_thread_idle_delhook(idle_hook);

    // 4. 设置看门狗超时为最小值（通常为1ms），使其立即超时
    rt_uint32_t min_timeout = 0; // 1ms超时
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &min_timeout);

    // 5. 不再喂狗，等待看门狗立即复位
    while (1)
    {
        // 空循环等待复位
    }
}

/* 初始化看门狗 */
int wdt_init(void)
{
    rt_err_t ret = RT_EOK;

    // 查找设备
    wdt_dev = rt_device_find(WDT_DEVICE_NAME);
    if (!wdt_dev)
    {
        rt_kprintf("[WDT] Device not found!\n");
        return -RT_ENOSYS;
    }
    else
        rt_kprintf("[WDT] Device found ok!\n");

    // 初始化设备
    if ((ret = rt_device_init(wdt_dev)) != RT_EOK)
    {
        rt_kprintf("[WDT] Init failed: %d\n", ret);
        return ret;
    }
    else
        rt_kprintf("[WDT] Init ok\n");

    // 设置超时
    rt_uint32_t timeout = WDT_TIMEOUT;
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);

    // 启用预超时中断
    rt_uint32_t pretimeout = WDT_FEED_INTERVAL; // 提前5秒触发中断
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_IRQ_TIMEOUT, &pretimeout);
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_IRQ_ENABLE, aic_wdt_irq);

    // 启动看门狗
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL);
    rt_kprintf("[WDT] Started, timeout=%ds\n", timeout);

    // 创建喂狗定时器
    wdt_feed_timer = rt_timer_create("wdt_feed", wdt_feed_callback,
                                     RT_NULL,
                                     WDT_FEED_INTERVAL * 1000,
                                     RT_TIMER_FLAG_PERIODIC);
    if (wdt_feed_timer)
    {
        rt_timer_start(wdt_feed_timer);
        rt_kprintf("[WDT] Auto-feeder started\n");
    }

    return RT_EOK;
}
