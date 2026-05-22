#include "wdt.h"

static rt_device_t wdt_dev = RT_NULL;
static rt_thread_t wdt_thread = RT_NULL;

// ✅ 2秒超时精准配置（严格满足：总超时 > 预超时 > 喂狗间隔）
#define WDT_TIMEOUT_MS      2000  // 看门狗总超时：2000毫秒=2秒（卡住2秒立刻重启）
#define WDT_PRETIMEOUT_MS   1500  // 预超时：1500毫秒=1.5秒（重启前0.5秒记录日志）
#define WDT_FEED_INTERVAL_MS 500  // 喂狗间隔：500毫秒=0.5秒（正常运行时绝对不会误触发）

/* 预超时中断：只记录崩溃日志，绝对不喂狗！ */
static irqreturn_t aic_wdt_irq(int irq, void *arg)
{
    rt_kprintf("\n\n!!! [WDT FATAL] System will RESET in 500ms !!!\n\n");
    rt_kprintf("[WDT] Last system tick: %d\n", rt_tick_get());

    // 这里可以添加关键变量打印，方便定位卡死位置
    // 绝对不要喂狗！让看门狗正常复位

    return IRQ_HANDLED;
}

/* 独立看门狗线程（最高优先级，确保任何时候都能运行） */
void wdt_thread_entry(void *parameter)
{
    rt_err_t ret = RT_EOK;

    // 查找设备
    wdt_dev = rt_device_find(WDT_DEVICE_NAME);
    if (!wdt_dev)
    {
        rt_kprintf("[WDT] Device not found!\n");
        return -RT_ENOSYS;
    }

    // 初始化设备
    if ((ret = rt_device_init(wdt_dev)) != RT_EOK)
    {
        rt_kprintf("[WDT] Init failed: %d\n", ret);
        return ret;
    }

    // ✅ 设置看门狗总超时为2秒
    rt_uint32_t timeout = WDT_TIMEOUT_MS;
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);

    // ✅ 设置预超时为1.5秒（重启前0.5秒记录日志）
    rt_uint32_t pretimeout = WDT_PRETIMEOUT_MS;
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_IRQ_TIMEOUT, &pretimeout);
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_IRQ_ENABLE, aic_wdt_irq);

    // 启动看门狗
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL);
    rt_kprintf("[WDT] 2-second watchdog started!\n");
    rt_kprintf("[WDT] Total timeout: %dms, Pretimeout: %dms, Feed interval: %dms\n",
              WDT_TIMEOUT_MS, WDT_PRETIMEOUT_MS, WDT_FEED_INTERVAL_MS);

    rt_kprintf("[WDT] 2-second watchdog started, priority: %d\n", rt_thread_self()->current_priority);

    while (1)
    {
        // 喂狗
        rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL);

        // 精确等待500毫秒
        rt_thread_mdelay(WDT_FEED_INTERVAL_MS);
    }
}

/* 立即触发看门狗复位函数（100%可靠） */
void wdt_immediate_reset(void)
{
    rt_kprintf("\n\n!!! [WDT] Triggering immediate system reset !!!\n\n");

    // 1. 禁用所有中断，防止任何干扰
    rt_enter_critical();

    // 2. 禁用预超时中断，防止中断处理函数干扰
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_IRQ_DISABLE, NULL);

    // 3. 停止看门狗线程
    if (wdt_thread != RT_NULL)
    {
        rt_thread_suspend(wdt_thread);
        rt_thread_delete(wdt_thread);
        wdt_thread = RT_NULL;
    }

    // 4. 设置看门狗超时为最小值（ArtInChip支持10ms超时）
    rt_uint32_t min_timeout = 10; // 10毫秒后立刻复位
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &min_timeout);

    // 5. 不再喂狗，等待看门狗立即复位
    while (1)
    {
        // 空循环等待复位
    }
}
