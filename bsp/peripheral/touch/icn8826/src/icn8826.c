/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-01-13     RiceChen     the first version
 * 2023-04-30     Geo          modified for ArtInChip
 */

#include <rtthread.h>
#include <rtdevice.h>

#include <string.h>

#define DBG_TAG "icn8826"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>
#include "../../common/touch_common.h"
#include "icn8826.h"

static struct rt_i2c_client icn8826_client;


static rt_err_t icn8826_write_reg(struct rt_i2c_client *dev, rt_uint8_t *data,
                                rt_uint8_t len)
{
    struct rt_i2c_msg msgs;

    msgs.addr = dev->client_addr;
    msgs.flags = RT_I2C_WR;
    msgs.buf = data;
    msgs.len = len;

    if (rt_i2c_transfer(dev->bus, &msgs, 1) == 1) {
        return RT_EOK;
    } else {
        return -RT_ERROR;
    }
}

static rt_err_t icn8826_read_regs(struct rt_i2c_client *dev, rt_uint8_t *reg,
                                rt_uint8_t *data, rt_uint8_t len)
{
    struct rt_i2c_msg msgs[2];

    msgs[0].addr = dev->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = reg;
    msgs[0].len = ICN8826_REGITER_LEN;

    msgs[1].addr = dev->client_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = data;
    msgs[1].len = len;

    if (rt_i2c_transfer(dev->bus, msgs, 2) == 2) {
        return RT_EOK;
    } else {
        return -RT_ERROR;
    }
}

static rt_err_t icn8826_get_info(struct rt_i2c_client *dev,
                               struct rt_touch_info *info)
{
    rt_uint8_t reg[2];
    rt_uint8_t out_info[8];
    rt_uint8_t out_len = 8;

  //  reg[0] = (rt_uint8_t)(icn8826_CONFIG_REG >> 8);
 //   reg[1] = (rt_uint8_t)(icn8826_CONFIG_REG & 0xFF);

  //  if (icn8826_read_regs(dev, reg, out_info, out_len) != RT_EOK) {
   //     LOG_E("read info failed");
  //      return -RT_ERROR;
   // }
    
    info->range_x = 480;//(out_info[3] << 8) | out_info[4];
    info->range_y = 480;//(out_info[5] << 8) | out_info[6];
	info->point_num = 1;
    return RT_EOK;
}

static rt_size_t icn8826_read_point(struct rt_touch_device *touch, void *data, rt_size_t touch_num)
{
    rt_uint8_t buf[10];
    rt_uint8_t cmd[2];
    struct rt_i2c_bus_device * i2c_bus = RT_NULL;

    static rt_uint8_t s_tp_down = 0;
    static uint16_t x_save, y_save;
    static rt_uint8_t s_count = 0;
	rt_uint8_t touchdownstatus;
    struct rt_touch_data *temp_data;

    temp_data = (struct rt_touch_data *)data;

    temp_data->event = RT_TOUCH_EVENT_NONE;
    temp_data->timestamp = rt_touch_get_ts();

    i2c_bus = rt_i2c_bus_device_find(touch->config.dev_name);
    if (i2c_bus == RT_NULL)
    {
        LOG_D("can not find i2c bus");
    //    return -RT_EIO;
		return 0;
    }
    cmd[0] = 0x10;
    cmd[1] = 0x00;

    icn8826_read_regs(&icn8826_client.bus, cmd, &buf, 9);

    temp_data->x_coordinate = ((buf[4]&0xF)<<8)|buf[3];
    temp_data->y_coordinate = ((buf[6]&0xF)<<8)|buf[5];

    temp_data->timestamp = rt_touch_get_ts();
//printf("touch state:%x\n",buf[8]);
    if ((buf[8]&0x4)==0x4)    /* up event */
    {
        if (s_tp_down == 1)
        {
        //    if (++s_count > 2)
            {
                s_count = 0;
                s_tp_down = 0;
                temp_data->event = RT_TOUCH_EVENT_UP;
                temp_data->timestamp = rt_touch_get_ts();
                temp_data->x_coordinate = x_save;
                temp_data->y_coordinate = y_save;
            }
        }
		
	//	printf("touch up: %d,%d %d\n",temp_data->x_coordinate,temp_data->y_coordinate,temp_data->event);
     //   return RT_EOK;
	return 1;
    }
    s_count = 0;
#if 0
    if (s_tp_down == 0)     /* down event */
    {
        s_tp_down = 1;
        temp_data->event = RT_TOUCH_EVENT_DOWN;
    }
    else    /* move event */
    {
        temp_data->event = RT_TOUCH_EVENT_MOVE;
    }
#endif	
	touchdownstatus=buf[8]&3;
	if(touchdownstatus==3){
		s_tp_down = 1;
		temp_data->event = RT_TOUCH_EVENT_DOWN;
		}
	else if(touchdownstatus==2){
		temp_data->event = RT_TOUCH_EVENT_MOVE;
		}
    x_save = temp_data->x_coordinate;
    y_save = temp_data->y_coordinate;
//printf("touch down: %d,%d  %d\n",temp_data->x_coordinate,temp_data->y_coordinate,temp_data->event);
//    return RT_EOK;
return 1;

}

static rt_err_t icn8826_control(struct rt_touch_device *touch, int cmd, void *arg)
{

    if (cmd == RT_TOUCH_CTRL_GET_INFO) {
        return icn8826_get_info(&icn8826_client, arg);
    }
    return RT_EOK;
}

static struct rt_touch_ops icn8826_touch_ops = {
    .touch_readpoint = icn8826_read_point,
    .touch_control = icn8826_control,
};

static int rt_hw_icn8826_init(const char *name, struct rt_touch_config *cfg)
{
    struct rt_touch_device *touch_device = RT_NULL;

    touch_device =
        (struct rt_touch_device *)rt_malloc(sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL) {
        LOG_E("touch device malloc fail");
        return -RT_ERROR;
    }
    rt_memset((void *)touch_device, 0, sizeof(struct rt_touch_device));

    icn8826_client.bus =
        (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);

    if (icn8826_client.bus == RT_NULL) {
        printf("Can't find %s device\n", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)icn8826_client.bus, RT_DEVICE_FLAG_RDWR) !=
        RT_EOK) {
        printf("open %s device failed\n", cfg->dev_name);
        return -RT_ERROR;
    }

    icn8826_client.client_addr = icn8826_ADDRESS;

    /* register touch device */
    touch_device->info.type = RT_TOUCH_TYPE_CAPACITANCE;
    touch_device->info.vendor = RT_TOUCH_VENDOR_GT;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &icn8826_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        printf("touch device icn8826 init failed !!!\n");
        return -RT_ERROR;
    }

    printf("touch device icn8826 init success\n");
    return RT_EOK;
}

static int rt_icn8826_gpio_cfg()
{
    unsigned int g, p;
    long pin;

    // RST
    pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    hal_gpio_direction_input(g, p);
    rt_pin_write(pin, 0);
    rt_thread_mdelay(50);
    rt_pin_write(pin, 1);
    rt_thread_mdelay(100);

    // INT
    pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    hal_gpio_direction_input(g, p);
    hal_gpio_set_irq_mode(g, p, 0);

    return 0;
}

static int rt_hw_icn8826_port(void)
{
    struct rt_touch_config cfg;
    rt_uint8_t rst_pin;

    rt_icn8826_gpio_cfg();

    rst_pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT;
    cfg.user_data = &rst_pin;
#ifdef AIC_PM_DEMO
    rt_pm_set_pin_wakeup_source(cfg.irq_pin.pin);
#endif

    rt_hw_icn8826_init("icn8826", &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_icn8826_port);
