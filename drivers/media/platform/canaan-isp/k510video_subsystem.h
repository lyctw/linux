/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _VIDEO_SUBSYSTEM_H_
#define _VIDEO_SUBSYSTEM_H_

#define   VIDEO_SUBSYSTEM_BASE_ADDR 0x92600000
#define   MIPI_DPHY_BASE            0x00000000
#define   MIPI_CORNER_BASE          0x00080000
#define   CSI_HOST_BASE             0x00020000
#define   CSI1_HOST_BASE            0x00020800

#define   VI_BASE_ADDR              0x00020000  //addr range 0x00000~0x0ffff
#define   ISP_4K_BASE_ADDR          0x00040000  //addr range 0x10000~0x1ffff
#define   ISP_2K_BASE_ADDR          0x00050000  //addr range 0x20000~0x2ffff
#define   ISP_R2K_BASE_ADDR         0x00060000  //addr range 0x30000~0x3ffff
#define   ISP_3D_BASE_ADDR          0x00070000  //addr range 0x40000~0x4ffff
#define   ISP_VO_BASE_ADDR          0x00100000  //addr range 0x50000~0x5ffff
#define   BT1120_BASE_ADDR          0x00130000 
#define   TD_BASE_ADDR              0x00120000    //0x92730000 on previous release before 0429 ffinal
#define   FBC_BASE_ADDR             0x00030000

//
#define   VI_WRAP_BASE     			    (VI_BASE_ADDR + 0x0700)
#define   VI_REG_SIZE_ALIGN         0x0800
#define   VI_PIPE_CSI_0_0_OFFSET    0x0400
#define   VI_PIPE_CSI_0_1_OFFSET    0x0500
#define   VI_PIPE_CSI_0_2_OFFSET    0x0500
#define   VI_PIPE_CSI_1_0_OFFSET    (VI_REG_SIZE_ALIGN + 0x0400)
#define   VI_PIPE_CSI_1_1_OFFSET    (VI_REG_SIZE_ALIGN + 0x0500)
#define   VI_PIPE_CSI_1_2_OFFSET    (VI_REG_SIZE_ALIGN + 0x0500)
#define   VI_PIPE_DVP_0_OFFSET      0x0600
#define   VI_PIPE_DVP_1_OFFSET      (VI_REG_SIZE_ALIGN + 0x0600)
#define   VI_PIPE_CSI_0_0_REG_BASE  (VI_BASE_ADDR + VI_PIPE_CSI_0_0_OFFSET)
#define   VI_PIPE_CSI_0_1_REG_BASE  (VI_BASE_ADDR + VI_PIPE_CSI_0_1_OFFSET)
#define   VI_PIPE_CSI_0_2_REG_BASE  (VI_BASE_ADDR + VI_PIPE_CSI_0_2_OFFSET)
#define   VI_PIPE_CSI_1_0_REG_BASE  (VI_BASE_ADDR + VI_REG_SIZE_ALIGN +VI_PIPE_CSI_0_0_OFFSET)
#define   VI_PIPE_CSI_1_1_REG_BASE  (VI_BASE_ADDR + VI_REG_SIZE_ALIGN + VI_PIPE_CSI_0_1_OFFSET)
#define   VI_PIPE_CSI_1_2_REG_BASE  (VI_BASE_ADDR + VI_REG_SIZE_ALIGN + VI_PIPE_CSI_0_2_OFFSET)
#define   VI_PIPE_DVP_0_REG_BASE    (VI_BASE_ADDR + VI_PIPE_DVP_0_OFFSET)
#define   VI_PIPE_DVP_1_REG_BASE    (VI_BASE_ADDR + VI_REG_SIZE_ALIGN + VI_PIPE_DVP_0_OFFSET)
//
#define   ISP_F2K_WRAP_BASE				(ISP_2K_BASE_ADDR + 0x0000)
#define   ISP_R2K_WRAP_BASE				(ISP_R2K_BASE_ADDR + 0x0000)
//  
#define   ISP_F2K_CORE_BASE        (ISP_2K_BASE_ADDR + 0x0400)
#define   ISP_F2K_CORE_TABLE_BASE  (ISP_2K_BASE_ADDR + 0x0800)
#define   ISP_R2K_CORE_BASE        (ISP_R2K_BASE_ADDR + 0x0400)
#define   ISP_R2K_CORE_TABLE_BASE  (ISP_R2K_BASE_ADDR + 0x0800)
//  
#define   ISP_F2K_DS_BASE          (ISP_2K_BASE_ADDR + 0x3000)
#define   ISP_R2K_DS_BASE          (ISP_R2K_BASE_ADDR + 0x3000)
//  
#define   ISP_F2K_FBC_BASE				  (ISP_2K_BASE_ADDR + 0x5000)
#define   ISP_F2K_FBD_BASE				  (ISP_2K_BASE_ADDR + 0x5400)
#define   ISP_F2K_REMAP_BASE			  (ISP_2K_BASE_ADDR + 0x6000) 
#define   ISP_F2K_OUT0_REMAP_BASE	(ISP_2K_BASE_ADDR + 0x6000) 
#define   ISP_F2K_OUT1_REMAP_BASE	(ISP_2K_BASE_ADDR + 0x6200) 
#define   ISP_F2K_MAIN_REMAP_BASE	(ISP_2K_BASE_ADDR + 0x6400) 
#define   ISP_R2K_REMAP_BASE			  (ISP_R2K_BASE_ADDR + 0x6000)
#define   ISP_R2K_OUT0_REMAP_BASE	(ISP_R2K_BASE_ADDR + 0x6000) 
#define   ISP_R2K_OUT1_REMAP_BASE	(ISP_R2K_BASE_ADDR + 0x6200)
#define   ISP_R2K_MAIN_REMAP_BASE	(ISP_R2K_BASE_ADDR + 0x6400) 
//  
#define   TOF_REG_BASE              ISP_3D_BASE_ADDR
#define   TOF_REG_WRAP_OFFSET       0x00000000
#define   TOF_REG_CORE_OFFSET       0x00000400
#define   TOF_REG_TABLE_OFFSET      0x00000800
#define   TOF_REG_DS_OFFSET         0x00003000
  
#define   TOF_WRAP_REG_BASE         (TOF_REG_BASE + TOF_REG_WRAP_OFFSET)
#define   TOF_WRAP_REG_SIZE_ALIGN   0x0400
  
#define   TOF_CORE_REG_BASE         (TOF_REG_BASE + TOF_REG_CORE_OFFSET)
#define   TOF_CORE_REG_SIZE_ALIGN   0x0400
  
#define   TOF_TABLE_REG_BASE        (TOF_REG_BASE + TOF_REG_TABLE_OFFSET)
#define   TOF_TABLE_REG_SIZE_ALIGN  0x1f00
  
#define   TOF_DS_REG_BASE           (TOF_REG_BASE + TOF_REG_DS_OFFSET)
#define   TOF_DS_REG_SIZE_ALIGN     0x1f00

#define   FBC_WRAP_BASE             (FBC_BASE_ADDR + 0x0000)
#define   FBC_CORE_BASE             (FBC_BASE_ADDR + 0x0100)

#define   VO_CORE_BASE              (ISP_VO_BASE_ADDR + 0x0000)
#define   VO_REMAP_BASE             (ISP_VO_BASE_ADDR + 0x600)
#define   VO_CONFIG_LINE_CTL_BASE   (ISP_VO_BASE_ADDR + 0x600)
#define   VO_CONFIG_LINE_DATA_BASE  (ISP_VO_BASE_ADDR + 0x700)
#define   VO_HSCALE_BASE            (ISP_VO_BASE_ADDR + (0x8000))
#define   VO_HSCALE_END             (ISP_VO_BASE_ADDR + (0x83FF))
#define   VO_VSCALE_BASE            (ISP_VO_BASE_ADDR + (0x8C00))
#define   VO_VSCALE_END             (ISP_VO_BASE_ADDR + (0x8D7f))
#define   VO_GAMMA_BASE             (ISP_VO_BASE_ADDR + (0xC000))
#define   VO_GAMMA_END              (ISP_VO_BASE_ADDR + (0xCBFF))

#endif /* _VIDEO_SUBSYSTEM_H_ */
