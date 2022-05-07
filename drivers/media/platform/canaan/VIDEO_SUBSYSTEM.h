/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
/*
 * VIDEO_SUBSYSTEM.h
 *
 *  Created on: Oct 30, 2019
 *      Author: fanwei
 */

#ifndef VIDEO_SUBSYSTEM_H_
#define VIDEO_SUBSYSTEM_H_

#define CANAAN_ISP
//#define USE_GPIO_I2C


#ifdef CANAAN_ISP
#define VIDEO_SUBSYSTEM_BASE_ADDR 0x92600000
#define VI_BASE_ADDR     0x92620000  //addr range 0x00000~0x0ffff
#define ISP_4K_BASE_ADDR 0x92640000  //addr range 0x10000~0x1ffff
#define ISP_2K_BASE_ADDR 0x92650000  //addr range 0x20000~0x2ffff
#define ISP_R2K_BASE_ADDR  0x92660000  //addr range 0x30000~0x3ffff
#define ISP_3D_BASE_ADDR 0x92670000  //addr range 0x40000~0x4ffff
#define ISP_VO_BASE_ADDR 0x92700000  //addr range 0x50000~0x5ffff
#define ISP_DEV_BASE_ADDR 0x92600000  //addr range 0x60000~0x6ffff
#define ISP_EXTCTRL_BASE_ADDR 0x92600000  //addr range 0x70000~0x7ffff
#define CNDS_DSI_BASE_ADDR 0x92710000  //addr range 0x70000~0x7ffff
#define BT1120_BASE_ADDR   0x92730000 

#define TD_BASE_ADDR  0x92720000    //0x92730000 on previous release before 0429 ffinal
#define MFBC_BASE_ADDR  0x92630000

#else

#define VIDEO_SUBSYSTEM_BASE_ADDR 0x92400000
#define VI_BASE_ADDR     0x92400000  //addr range 0x00000~0x0ffff
#define ISP_4K_BASE_ADDR 0x92410000  //addr range 0x10000~0x1ffff
#define ISP_2K_BASE_ADDR 0x92420000  //addr range 0x20000~0x2ffff
#define ISP_R2K_BASE_ADDR  0x92430000  //addr range 0x30000~0x3ffff
#define ISP_3D_BASE_ADDR 0x92440000  //addr range 0x40000~0x4ffff
#define ISP_VO_BASE_ADDR 0x92450000  //addr range 0x50000~0x5ffff
#define ISP_DEV_BASE_ADDR 0x92460000  //addr range 0x60000~0x6ffff
#define ISP_EXTCTRL_BASE_ADDR 0x92470000  //addr range 0x70000~0x7ffff
#define CNDS_DSI_BASE_ADDR 0x92480000  //addr range 0x70000~0x7ffff
#define BT1120_BASE_ADDR   0x92490000 
#define TD_BASE_ADDR  0x924A0000
#define MFBC_BASE_ADDR  0x924B0000


#endif


#endif /* VIDEO_SUBSYSTEM_H_ */
