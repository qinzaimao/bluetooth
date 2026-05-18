/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include <aic_hal.h>


#define LCD_CS_SET     panel_gpio_set_value(&spi_cs, 1)
#define LCD_CS_RESET   panel_gpio_set_value(&spi_cs, 0)

#define LCD_SCL_SET    panel_gpio_set_value(&spi_scl, 1)
#define LCD_SCL_RESET  panel_gpio_set_value(&spi_scl, 0)

#define LCD_SDA_SET    panel_gpio_set_value(&spi_sdi, 1)
#define LCD_SDA_RESET  panel_gpio_set_value(&spi_sdi, 0)

#define USLT  20

#define spi_delay(us)  aic_udelay(us)
#define DelayMS         aic_delay_ms

//#define SLEEP_PIN  "PD.2"
#define RESET_PIN  "PD.2"

#define CS         "PD.3"
#define SCL        "PD.5"
#define SDI        "PD.4"

static struct gpio_desc reset_gpio;
static struct gpio_desc sleep_gpio;

static struct gpio_desc spi_cs;
static struct gpio_desc spi_sdi;
static struct gpio_desc spi_scl;

static void panel_gpio_init(void)
{
    panel_get_gpio(&reset_gpio, RESET_PIN);
    panel_get_gpio(&spi_cs, CS);
    panel_get_gpio(&spi_sdi, SDI);
    panel_get_gpio(&spi_scl, SCL);
    //panel_get_gpio(&sleep_gpio, SLEEP_PIN);

    panel_gpio_set_value(&sleep_gpio, 1);
    aic_delay_ms(50);
    panel_gpio_set_value(&reset_gpio, 0);
    aic_delay_ms(20);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(120);
}

static void SPI_Write_Word(unsigned short dat)
{
	int i = 0;
	for(i=0; i<16; i++){
		if(dat&0x8000){
            		LCD_SDA_SET;	//GPIO_DATA_PIN[i]
		}
		else{
            		LCD_SDA_RESET;	//GPIO_DATA_PIN[i]
		}
		LCD_SCL_RESET;	//GPIO_DATA_PIN[i]
		spi_delay(USLT);
		LCD_SCL_SET;	//GPIO_DATA_PIN[i]
		spi_delay(USLT);
		dat <<= 1;
	}
}

void Write_LCD_REG(unsigned short dat)
{
	LCD_CS_RESET;
	SPI_Write_Word(dat);
	LCD_CS_SET;
}

static int panel_enable(struct aic_panel *panel)
{
    panel_gpio_init();

    panel_spi_device_emulation(CS, SDI, SCL);

    LCD_SDA_SET; //GPIO_DATA_PIN[i]
	LCD_SCL_SET; //GPIO_DATA_PIN[i]
	LCD_CS_SET; //GPIO_DATA_PIN[i]

//	printf("lv se st7701 >>>>>>>>>>>>>>>>>>>>>>>>\n");
	//FF 13
Write_LCD_REG(0x20FF);Write_LCD_REG(0x0000);Write_LCD_REG(0x4077);
Write_LCD_REG(0x20FF);Write_LCD_REG(0x0001);Write_LCD_REG(0x4001);
Write_LCD_REG(0x20FF);Write_LCD_REG(0x0002);Write_LCD_REG(0x4000);
Write_LCD_REG(0x20FF);Write_LCD_REG(0x0003);Write_LCD_REG(0x4000);
Write_LCD_REG(0x20FF);Write_LCD_REG(0x0004);Write_LCD_REG(0x4013);

	//EF
Write_LCD_REG(0x20EF);Write_LCD_REG(0x0000);Write_LCD_REG(0x4008);

	//FF 10
Write_LCD_REG(0x20FF);Write_LCD_REG(0x0000);Write_LCD_REG(0x4077);//
Write_LCD_REG(0x20FF);Write_LCD_REG(0x0001);Write_LCD_REG(0x4001);//
Write_LCD_REG(0x20FF);Write_LCD_REG(0x0002);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20FF);Write_LCD_REG(0x0003);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20FF);Write_LCD_REG(0x0004);Write_LCD_REG(0x4010);//

	//C0
Write_LCD_REG(0x20C0);Write_LCD_REG(0x0000);Write_LCD_REG(0x403B);//

Write_LCD_REG(0x20C0);Write_LCD_REG(0x0001);Write_LCD_REG(0x4000);//

	//C1
Write_LCD_REG(0x20C1);Write_LCD_REG(0x0000);Write_LCD_REG(0x400B);//

Write_LCD_REG(0x20C1);Write_LCD_REG(0x0001);Write_LCD_REG(0x4002);//

	//C2
Write_LCD_REG(0x20C2);Write_LCD_REG(0x0000);Write_LCD_REG(0x4031);//

Write_LCD_REG(0x20C2);Write_LCD_REG(0x0001);Write_LCD_REG(0x4002);//

	//CC
Write_LCD_REG(0xCC00);Write_LCD_REG(0x4010);//

	//B0
Write_LCD_REG(0x20B0);Write_LCD_REG(0x0000);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20B0);Write_LCD_REG(0x0001);Write_LCD_REG(0x400A);//
Write_LCD_REG(0x20B0);Write_LCD_REG(0x0002);Write_LCD_REG(0x4011);//
Write_LCD_REG(0x20B0);Write_LCD_REG(0x0003);Write_LCD_REG(0x400C);//
Write_LCD_REG(0x20B0);Write_LCD_REG(0x0004);Write_LCD_REG(0x4010);//
Write_LCD_REG(0x20B0);Write_LCD_REG(0x0005);Write_LCD_REG(0x4006);//
Write_LCD_REG(0x20B0);Write_LCD_REG(0x0006);Write_LCD_REG(0x4004);//
Write_LCD_REG(0x20B0);Write_LCD_REG(0x0007);Write_LCD_REG(0x4009);//
Write_LCD_REG(0x20B0);Write_LCD_REG(0x0008);Write_LCD_REG(0x4008);//
Write_LCD_REG(0x20B0);Write_LCD_REG(0x0009);Write_LCD_REG(0x4021);//
Write_LCD_REG(0x20B0);Write_LCD_REG(0x000A);Write_LCD_REG(0x4005);//
Write_LCD_REG(0x20B0);Write_LCD_REG(0x000B);Write_LCD_REG(0x4011);//
Write_LCD_REG(0x20B0);Write_LCD_REG(0x000C);Write_LCD_REG(0x400F);//
Write_LCD_REG(0x20B0);Write_LCD_REG(0x000D);Write_LCD_REG(0x4028);//
Write_LCD_REG(0x20B0);Write_LCD_REG(0x000E);Write_LCD_REG(0x402D);//
Write_LCD_REG(0x20B0);Write_LCD_REG(0x000F);Write_LCD_REG(0x4018);//

	//B1
Write_LCD_REG(0x20B1);Write_LCD_REG(0x0000);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20B1);Write_LCD_REG(0x0001);Write_LCD_REG(0x400A);//
Write_LCD_REG(0x20B1);Write_LCD_REG(0x0002);Write_LCD_REG(0x4011);//
Write_LCD_REG(0x20B1);Write_LCD_REG(0x0003);Write_LCD_REG(0x400D);//
Write_LCD_REG(0x20B1);Write_LCD_REG(0x0004);Write_LCD_REG(0x4010);//
Write_LCD_REG(0x20B1);Write_LCD_REG(0x0005);Write_LCD_REG(0x4006);//
Write_LCD_REG(0x20B1);Write_LCD_REG(0x0006);Write_LCD_REG(0x4004);//
Write_LCD_REG(0x20B1);Write_LCD_REG(0x0007);Write_LCD_REG(0x4008);//
Write_LCD_REG(0x20B1);Write_LCD_REG(0x0008);Write_LCD_REG(0x4009);//
Write_LCD_REG(0x20B1);Write_LCD_REG(0x0009);Write_LCD_REG(0x4021);//
Write_LCD_REG(0x20B1);Write_LCD_REG(0x000A);Write_LCD_REG(0x4005);//
Write_LCD_REG(0x20B1);Write_LCD_REG(0x000B);Write_LCD_REG(0x4011);//
Write_LCD_REG(0x20B1);Write_LCD_REG(0x000C);Write_LCD_REG(0x400F);//
Write_LCD_REG(0x20B1);Write_LCD_REG(0x000D);Write_LCD_REG(0x4028);//
Write_LCD_REG(0x20B1);Write_LCD_REG(0x000E);Write_LCD_REG(0x402D);//
Write_LCD_REG(0x20B1);Write_LCD_REG(0x000F);Write_LCD_REG(0x4018);//

	//FF 11
Write_LCD_REG(0x20FF);Write_LCD_REG(0x0000);Write_LCD_REG(0x4077);//	0xFF00h 0x77
Write_LCD_REG(0x20FF);Write_LCD_REG(0x0001);Write_LCD_REG(0x4001);//	0xFF01h 0x01
Write_LCD_REG(0x20FF);Write_LCD_REG(0x0002);Write_LCD_REG(0x4000);//	0xFF02h 0x00
Write_LCD_REG(0x20FF);Write_LCD_REG(0x0003);Write_LCD_REG(0x4000);//	0xFF03h 0x00
Write_LCD_REG(0x20FF);Write_LCD_REG(0x0004);Write_LCD_REG(0x4011);//	0xFF04h 0x10

	//B0
Write_LCD_REG(0x20B0);Write_LCD_REG(0x0000);Write_LCD_REG(0x404D);//

	//B1
Write_LCD_REG(0x20B1);Write_LCD_REG(0x0000);Write_LCD_REG(0x4033);// // Vcom

	//B2
Write_LCD_REG(0x20B2);Write_LCD_REG(0x0000);Write_LCD_REG(0x4087);//

	//B5
Write_LCD_REG(0x20B5);Write_LCD_REG(0x0000);Write_LCD_REG(0x404B);//

	//B7
Write_LCD_REG(0x20B7);Write_LCD_REG(0x0000);Write_LCD_REG(0x408C);//

	//B8
Write_LCD_REG(0x20B8);Write_LCD_REG(0x0000);Write_LCD_REG(0x4020);//

	//C1
Write_LCD_REG(0x20C1);Write_LCD_REG(0x0000);Write_LCD_REG(0x4078);//

	//C2
Write_LCD_REG(0x20C2);Write_LCD_REG(0x0000);Write_LCD_REG(0x4078);//

	//D0
Write_LCD_REG(0x20D0);Write_LCD_REG(0x0000);Write_LCD_REG(0x4088);//

	//E0
Write_LCD_REG(0x20E0);Write_LCD_REG(0x0000);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20E0);Write_LCD_REG(0x0001);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20E0);Write_LCD_REG(0x0002);Write_LCD_REG(0x4002);//

	//E1
Write_LCD_REG(0x20E1);Write_LCD_REG(0x0000);Write_LCD_REG(0x4002);//
Write_LCD_REG(0x20E1);Write_LCD_REG(0x0001);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E1);Write_LCD_REG(0x0002);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20E1);Write_LCD_REG(0x0003);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20E1);Write_LCD_REG(0x0004);Write_LCD_REG(0x4003);//
Write_LCD_REG(0x20E1);Write_LCD_REG(0x0005);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E1);Write_LCD_REG(0x0006);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20E1);Write_LCD_REG(0x0007);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20E1);Write_LCD_REG(0x0008);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20E1);Write_LCD_REG(0x0009);Write_LCD_REG(0x4044);//
Write_LCD_REG(0x20E1);Write_LCD_REG(0x000A);Write_LCD_REG(0x4044);//

	//E2
Write_LCD_REG(0x20E2);Write_LCD_REG(0x0000);Write_LCD_REG(0x4010);//
Write_LCD_REG(0x20E2);Write_LCD_REG(0x0001);Write_LCD_REG(0x4010);//
Write_LCD_REG(0x20E2);Write_LCD_REG(0x0002);Write_LCD_REG(0x4040);//
Write_LCD_REG(0x20E2);Write_LCD_REG(0x0003);Write_LCD_REG(0x4040);//
Write_LCD_REG(0x20E2);Write_LCD_REG(0x0004);Write_LCD_REG(0x40F2);//
Write_LCD_REG(0x20E2);Write_LCD_REG(0x0005);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E2);Write_LCD_REG(0x0006);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20E2);Write_LCD_REG(0x0007);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20E2);Write_LCD_REG(0x0008);Write_LCD_REG(0x40F2);//
Write_LCD_REG(0x20E2);Write_LCD_REG(0x0009);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E2);Write_LCD_REG(0x000A);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20E2);Write_LCD_REG(0x000B);Write_LCD_REG(0x4000);//

	//E3
Write_LCD_REG(0x20E3);Write_LCD_REG(0x0000);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20E3);Write_LCD_REG(0x0001);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20E3);Write_LCD_REG(0x0002);Write_LCD_REG(0x4011);//
Write_LCD_REG(0x20E3);Write_LCD_REG(0x0003);Write_LCD_REG(0x4011);//
	//E4
Write_LCD_REG(0x20E4);Write_LCD_REG(0x0000);Write_LCD_REG(0x4044);//
Write_LCD_REG(0x20E4);Write_LCD_REG(0x0001);Write_LCD_REG(0x4044);//


	//E5
Write_LCD_REG(0x20E5);Write_LCD_REG(0x0000);Write_LCD_REG(0x4007);//
Write_LCD_REG(0x20E5);Write_LCD_REG(0x0001);Write_LCD_REG(0x40EF);//
Write_LCD_REG(0x20E5);Write_LCD_REG(0x0002);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E5);Write_LCD_REG(0x0003);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E5);Write_LCD_REG(0x0004);Write_LCD_REG(0x4009);//
Write_LCD_REG(0x20E5);Write_LCD_REG(0x0005);Write_LCD_REG(0x40F1);//
Write_LCD_REG(0x20E5);Write_LCD_REG(0x0006);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E5);Write_LCD_REG(0x0007);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E5);Write_LCD_REG(0x0008);Write_LCD_REG(0x4003);//
Write_LCD_REG(0x20E5);Write_LCD_REG(0x0009);Write_LCD_REG(0x40F3);//
Write_LCD_REG(0x20E5);Write_LCD_REG(0x000A);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E5);Write_LCD_REG(0x000B);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E5);Write_LCD_REG(0x000C);Write_LCD_REG(0x4005);//
Write_LCD_REG(0x20E5);Write_LCD_REG(0x000D);Write_LCD_REG(0x40ED);//
Write_LCD_REG(0x20E5);Write_LCD_REG(0x000E);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E5);Write_LCD_REG(0x000F);Write_LCD_REG(0x40F0);//

	//E6
Write_LCD_REG(0x20E6);Write_LCD_REG(0x0000);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20E6);Write_LCD_REG(0x0001);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20E6);Write_LCD_REG(0x0002);Write_LCD_REG(0x4011);//
Write_LCD_REG(0x20E6);Write_LCD_REG(0x0003);Write_LCD_REG(0x4011);//

	//E7
Write_LCD_REG(0x20E7);Write_LCD_REG(0x0000);Write_LCD_REG(0x4044);//
Write_LCD_REG(0x20E7);Write_LCD_REG(0x0001);Write_LCD_REG(0x4044);//

	//E8
Write_LCD_REG(0x20E8);Write_LCD_REG(0x0000);Write_LCD_REG(0x4008);//
Write_LCD_REG(0x20E8);Write_LCD_REG(0x0001);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E8);Write_LCD_REG(0x0002);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E8);Write_LCD_REG(0x0003);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E8);Write_LCD_REG(0x0004);Write_LCD_REG(0x400A);//
Write_LCD_REG(0x20E8);Write_LCD_REG(0x0005);Write_LCD_REG(0x40F2);//
Write_LCD_REG(0x20E8);Write_LCD_REG(0x0006);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E8);Write_LCD_REG(0x0007);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E8);Write_LCD_REG(0x0008);Write_LCD_REG(0x4004);//
Write_LCD_REG(0x20E8);Write_LCD_REG(0x0009);Write_LCD_REG(0x40F4);//
Write_LCD_REG(0x20E8);Write_LCD_REG(0x000A);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E8);Write_LCD_REG(0x000B);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E8);Write_LCD_REG(0x000C);Write_LCD_REG(0x4006);//
Write_LCD_REG(0x20E8);Write_LCD_REG(0x000D);Write_LCD_REG(0x40EE);//
Write_LCD_REG(0x20E8);Write_LCD_REG(0x000E);Write_LCD_REG(0x40F0);//
Write_LCD_REG(0x20E8);Write_LCD_REG(0x000F);Write_LCD_REG(0x40F0);//


	//EB
Write_LCD_REG(0x20EB);Write_LCD_REG(0x0000);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20EB);Write_LCD_REG(0x0001);Write_LCD_REG(0x4000);//
Write_LCD_REG(0x20EB);Write_LCD_REG(0x0002);Write_LCD_REG(0x40E4);//
Write_LCD_REG(0x20EB);Write_LCD_REG(0x0003);Write_LCD_REG(0x40E4);//
Write_LCD_REG(0x20EB);Write_LCD_REG(0x0004);Write_LCD_REG(0x4044);//
Write_LCD_REG(0x20EB);Write_LCD_REG(0x0005);Write_LCD_REG(0x4088);//
Write_LCD_REG(0x20EB);Write_LCD_REG(0x0006);Write_LCD_REG(0x4040);//

	//EC
Write_LCD_REG(0x20EC);Write_LCD_REG(0x0000);Write_LCD_REG(0x4078);//
Write_LCD_REG(0x20EC);Write_LCD_REG(0x0001);Write_LCD_REG(0x4000);//

	//ED
Write_LCD_REG(0x20ED);Write_LCD_REG(0x0000);Write_LCD_REG(0x4020);//
Write_LCD_REG(0x20ED);Write_LCD_REG(0x0001);Write_LCD_REG(0x40F9);//
Write_LCD_REG(0x20ED);Write_LCD_REG(0x0002);Write_LCD_REG(0x4087);//
Write_LCD_REG(0x20ED);Write_LCD_REG(0x0003);Write_LCD_REG(0x4076);//
Write_LCD_REG(0x20ED);Write_LCD_REG(0x0004);Write_LCD_REG(0x4065);//
Write_LCD_REG(0x20ED);Write_LCD_REG(0x0005);Write_LCD_REG(0x4054);//
Write_LCD_REG(0x20ED);Write_LCD_REG(0x0006);Write_LCD_REG(0x404F);//
Write_LCD_REG(0x20ED);Write_LCD_REG(0x0007);Write_LCD_REG(0x40FF);//
Write_LCD_REG(0x20ED);Write_LCD_REG(0x0008);Write_LCD_REG(0x40FF);//
Write_LCD_REG(0x20ED);Write_LCD_REG(0x0009);Write_LCD_REG(0x40F4);//
Write_LCD_REG(0x20ED);Write_LCD_REG(0x000A);Write_LCD_REG(0x4045);//
Write_LCD_REG(0x20ED);Write_LCD_REG(0x000B);Write_LCD_REG(0x4056);//
Write_LCD_REG(0x20ED);Write_LCD_REG(0x000C);Write_LCD_REG(0x4067);//
Write_LCD_REG(0x20ED);Write_LCD_REG(0x000D);Write_LCD_REG(0x4078);//
Write_LCD_REG(0x20ED);Write_LCD_REG(0x000E);Write_LCD_REG(0x409F);//
Write_LCD_REG(0x20ED);Write_LCD_REG(0x000F);Write_LCD_REG(0x4002);//

	//EF
Write_LCD_REG(0x20EF);Write_LCD_REG(0x0000);Write_LCD_REG(0x4010);//
Write_LCD_REG(0x20EF);Write_LCD_REG(0x0001);Write_LCD_REG(0x400D);//
Write_LCD_REG(0x20EF);Write_LCD_REG(0x0002);Write_LCD_REG(0x4004);//
Write_LCD_REG(0x20EF);Write_LCD_REG(0x0003);Write_LCD_REG(0x4008);//
Write_LCD_REG(0x20EF);Write_LCD_REG(0x0004);Write_LCD_REG(0x403F);//
Write_LCD_REG(0x20EF);Write_LCD_REG(0x0005);Write_LCD_REG(0x401F);//



		Write_LCD_REG(0x2011);Write_LCD_REG(0x0000); Write_LCD_REG(0x4000);

		DelayMS(120);

		Write_LCD_REG(0x2029);Write_LCD_REG(0x0000); Write_LCD_REG(0x4000);

		DelayMS(100);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    //panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs st7701s_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing st7701s_timing = {
    .pixelclock = 18000000,
    .hactive = 480,
    .hfront_porch = 70,
    .hback_porch = 20,
    .hsync_len = 20,
    .vactive = 480,
    .vfront_porch = 14,
    .vback_porch = 12,
    .vsync_len = 2,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_18BIT_LD,
    .clock_phase = DEGREE_0,
    .data_order = RGB,
    .data_mirror = 0,
};

struct aic_panel rgb_st7701s = {
    .name = "panel-st7701s",
    .timings = &st7701s_timing,
    .funcs = &st7701s_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

