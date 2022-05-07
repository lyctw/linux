/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __WS2812B_H__
#define __WS2812B_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_IOCTL_LED_MAGIC     (0xA0)
#define CMD_IOCTL_LED_INIT      _IOW(CMD_IOCTL_LED_MAGIC, 1, unsigned long)
#define CMD_IOCTL_LED_DEINIT    _IOW(CMD_IOCTL_LED_MAGIC, 2, unsigned long)

#define RST_KEEP_US     (300) /* >50us */
#define T0H_NS  (800)
#define T0L_NS  (400)
#define T1H_NS  (800)
#define T1L_NS  (400)

/*****************************************
* struct led_attr_t
*****************************************/
typedef struct {
	int rst_us;
	int t0h_ns;
	int t0l_ns;
	int t1h_ns;
	int t1l_ns;
	int speed_hz;
	int led_count;
} led_attr_t;


/*****************************************
* struct led_rgb_u
*****************************************/
typedef struct {
	unsigned int reserved:8;
	unsigned int blue:8;
	unsigned int red:8;
	unsigned int green:8;
}  __attribute__((packed, aligned(4))) led_color_t;

typedef union {
	led_color_t rgb;
	unsigned int value;
}  led_rgb_u;

#ifdef __cplusplus
}
#endif

#endif /*__WS2812B_H__*/
