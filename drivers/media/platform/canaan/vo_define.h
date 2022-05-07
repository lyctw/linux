/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef VO_DEFINE_H_
#define VO_DEFINE_H_

#include "VIDEO_SUBSYSTEM.h"




#define   VO_CONFIG_WRAP_START         ISP_VO_BASE_ADDR + 0x0000
#define   VO_HSCALE_START              ISP_VO_BASE_ADDR + (0x2000 <<2)
#define   VO_HSCALE_END                ISP_VO_BASE_ADDR + (0x20ff <<2)
#define   VO_VSCALE_START              ISP_VO_BASE_ADDR + (0x2300 <<2)
#define   VO_VSCALE_END                ISP_VO_BASE_ADDR + (0x237f <<2)
#define   VO_GAMMA_START               ISP_VO_BASE_ADDR + (0x3000 <<2)
#define   VO_GAMMA_END                 ISP_VO_BASE_ADDR + (0x3CFF <<2)
#define   VO_SPI_START                 ISP_VO_BASE_ADDR + (0x100 <<2)
#define   VO_SPI_END                   ISP_VO_BASE_ADDR + (0x10b <<2)


#define   ADDR_VO_SOFT_RST_CTL               0x0000
#define   ADDR_VO_REG_LOAD_CTL              0x0001

#define   ADDR_VO_DMA_SW_CTRL                0x0002
#define   ADDR_VO_DMA_RD_CTRL_OUT            0x0003
#define   ADDR_VO_DMA_ARB_MODE               0x0004
#define   ADDR_VO_DMA_WEIGHT_WR_0            0x0005
#define   ADDR_VO_DMA_WEIGHT_WR_1            0x0006
#define   ADDR_VO_DMA_WEIGHT_WR_2            0x0007
#define   ADDR_VO_DMA_WEIGHT_WR_3            0x0008
#define   ADDR_VO_DMA_WEIGHT_RD_0            0x0009
#define   ADDR_VO_DMA_WEIGHT_RD_1            0x000a
#define   ADDR_VO_DMA_WEIGHT_RD_2            0x000b
#define   ADDR_VO_DMA_WEIGHT_RD_3            0x000c
#define   ADDR_VO_DMA_PRIORITY_WR_0          0x000d
#define   ADDR_VO_DMA_PRIORITY_WR_1          0x000e
#define   ADDR_VO_DMA_PRIORITY_RD_0          0x000f
#define   ADDR_VO_DMA_PRIORITY_RD_1          0x0010
#define   ADDR_VO_DMA_ID_WR_0                0x0011
#define   ADDR_VO_DMA_ID_WR_1                0x0012
#define   ADDR_VO_DMA_ID_RD_0                0x0013
#define   ADDR_VO_DMA_ID_RD_1                0x0014

#define   ADDR_VO_LAYER0_LINE0_BD_CTL        0x0020
#define   ADDR_VO_LAYER0_LINE1_BD_CTL        0x0021
#define   ADDR_VO_LAYER0_LINE2_BD_CTL        0x0022
#define   ADDR_VO_LAYER0_LINE3_BD_CTL        0x0023
#define   ADDR_VO_LAYER1_BD_CTL              0x0024
#define   ADDR_VO_LAYER2_BD_CTL              0x0025
#define   ADDR_VO_LAYER3_BD_CTL              0x0026
#define   ADDR_VO_LAYER4_BD_CTL              0x0027
#define   ADDR_VO_LAYER5_BD_CTL              0x0028
#define   ADDR_VO_LAYER6_BD_CTL              0x0029

#define   ADDR_VO_DISP_XZONE_CTL             0x0030
#define   ADDR_VO_DISP_YZONE_CTL             0x0031
#define   ADDR_VO_DISP_LAYER0_XCTL           0x0032
#define   ADDR_VO_DISP_LAYER0_YCTL           0x0033
#define   ADDR_VO_DISP_LAYER1_XCTL           0x0034
#define   ADDR_VO_DISP_LAYER1_YCTL           0x0035
#define   ADDR_VO_DISP_LAYER2_XCTL           0x0036
#define   ADDR_VO_DISP_LAYER2_YCTL           0x0037
#define   ADDR_VO_DISP_LAYER3_XCTL           0x0038
#define   ADDR_VO_DISP_LAYER3_YCTL           0x0039
#define   ADDR_VO_DISP_LAYER4_XCTL           0x003a
#define   ADDR_VO_DISP_LAYER4_YCTL           0x003b
#define   ADDR_VO_DISP_LAYER5_XCTL           0x003c
#define   ADDR_VO_DISP_LAYER5_YCTL           0x003d
#define   ADDR_VO_DISP_LAYER6_XCTL           0x003e
#define   ADDR_VO_DISP_LAYER6_YCTL           0x003f

#define   ADDR_VO_DISP_HSYNC_CTL             0x0040
#define   ADDR_VO_DISP_HSYNC1_CTL            0x0041
#define   ADDR_VO_DISP_VSYNC1_CTL            0x0042
#define   ADDR_VO_DISP_HSYNC2_CTL            0x0043
#define   ADDR_VO_DISP_VSYNC2_CTL            0x0044
#define   ADDR_VO_DISP_CTL                   0x0045
#define   ADDR_VO_DISP_ENABLE                0x0046
#define   ADDR_VO_DISP_SIZE                  0x0047

#define   ADDR_VO_LCD_CTL                    0x004a

#define   ADDR_VO_LAYER0_CTL                 0x0050
#define   ADDR_VO_LAYER0_SCALE_BLENTH        0x0051
#define   ADDR_VO_LAYER0_IMG_IN_DAT_MODE     0x0052
#define   ADDR_VO_LAYER0_BADDR0_Y            0x0053
#define   ADDR_VO_LAYER0_BADDR0_UV           0x0054
#define   ADDR_VO_LAYER0_BADDR1_Y            0x0055
#define   ADDR_VO_LAYER0_BADDR1_UV           0x0056
#define   ADDR_VO_LAYER0_IMG_IN_PIX_SIZE     0x0057
#define   ADDR_VO_LAYER0_IMG_OUT_PIX_SIZE    0x0058
#define   ADDR_VO_LAYER0_IMG_IN_SIZE         0x0059
#define   ADDR_VO_LAYER0_IMG_IN_MEM_HSIZE    0x005a
#define   ADDR_VO_LAYER0_IMG_IN_OFFSET       0x005b
#define   ADDR_VO_LAYER0_SCALE_CTL           0x005c
#define   ADDR_VO_LAYER0_VSCALE_STEP         0x005d
#define   ADDR_VO_LAYER0_VSCALE_ST_PSTEP     0x005e
#define   ADDR_VO_LAYER0_HSCALE_STEP         0x005f
#define   ADDR_VO_LAYER0_HSCALE_ST_PSTEP     0x0060

#define   ADDR_VO_LAYER1_CTL                 0x0070
#define   ADDR_VO_LAYER1_IMG_IN_BADDR0_Y     0x0071
#define   ADDR_VO_LAYER1_IMG_IN_BADDR0_UV    0x0072
#define   ADDR_VO_LAYER1_IMG_IN_BADDR1_Y     0x0073
#define   ADDR_VO_LAYER1_IMG_IN_BADDR1_UV    0x0074
#define   ADDR_VO_LAYER1_IMG_IN_OFFSET       0x0075
#define   ADDR_VO_LAYER1_IMG_IN_BLENTH       0x0076
#define   ADDR_VO_LAYER1_IMG_IN_SIZE         0x0077
#define   ADDR_VO_LAYER1_IMG_IN_PIX_SIZE     0x0078

#define   ADDR_VO_LAYER2_CTL                 0x0080
#define   ADDR_VO_LAYER2_IMG_IN_BADDR0_Y     0x0081
#define   ADDR_VO_LAYER2_IMG_IN_BADDR0_UV    0x0082
#define   ADDR_VO_LAYER2_IMG_IN_BADDR1_Y     0x0083
#define   ADDR_VO_LAYER2_IMG_IN_BADDR1_UV    0x0084
#define   ADDR_VO_LAYER2_IMG_IN_OFFSET       0x0085
#define   ADDR_VO_LAYER2_IMG_IN_BLENTH       0x0086
#define   ADDR_VO_LAYER2_IMG_IN_SIZE         0x0087
#define   ADDR_VO_LAYER2_IMG_IN_PIX_SIZE     0x0088

#define   ADDR_VO_LAYER3_CTL                 0x0090
#define   ADDR_VO_LAYER3_IMG_IN_BADDR0_Y     0x0091
#define   ADDR_VO_LAYER3_IMG_IN_BADDR0_UV    0x0092
#define   ADDR_VO_LAYER3_IMG_IN_BADDR1_Y     0x0093
#define   ADDR_VO_LAYER3_IMG_IN_BADDR1_UV    0x0094
#define   ADDR_VO_LAYER3_IMG_IN_OFFSET       0x0095
#define   ADDR_VO_LAYER3_IMG_IN_BLENTH       0x0096
#define   ADDR_VO_LAYER3_IMG_IN_SIZE         0x0097
#define   ADDR_VO_LAYER3_IMG_IN_PIX_SIZE     0x0098

#define   ADDR_VO_DISP_OSD0_INFO             0x00a0
#define   ADDR_VO_DISP_OSD0_SIZE             0x00a1
#define   ADDR_VO_DISP_OSD0_VLU_ADDR0        0x00a2
#define   ADDR_VO_DISP_OSD0_ALP_ADDR0        0x00a3
#define   ADDR_VO_DISP_OSD0_VLU_ADDR1        0x00a4
#define   ADDR_VO_DISP_OSD0_ALP_ADDR1        0x00a5
#define   ADDR_VO_DISP_OSD0_DMA_CTRL         0x00a6
#define   ADDR_VO_DISP_OSD0_STRIDE           0x00a7

#define   ADDR_VO_DISP_OSD1_INFO             0x00b0
#define   ADDR_VO_DISP_OSD1_SIZE             0x00b1
#define   ADDR_VO_DISP_OSD1_VLU_ADDR0        0x00b2
#define   ADDR_VO_DISP_OSD1_ALP_ADDR0        0x00b3
#define   ADDR_VO_DISP_OSD1_VLU_ADDR1        0x00b4
#define   ADDR_VO_DISP_OSD1_ALP_ADDR1        0x00b5
#define   ADDR_VO_DISP_OSD1_DMA_CTRL         0x00b6
#define   ADDR_VO_DISP_OSD1_STRIDE           0x00b7

#define   ADDR_VO_DISP_OSD2_INFO             0x00c0
#define   ADDR_VO_DISP_OSD2_SIZE             0x00c1
#define   ADDR_VO_DISP_OSD2_VLU_ADDR0        0x00c2
#define   ADDR_VO_DISP_OSD2_ALP_ADDR0        0x00c3
#define   ADDR_VO_DISP_OSD2_VLU_ADDR1        0x00c4
#define   ADDR_VO_DISP_OSD2_ALP_ADDR1        0x00c5
#define   ADDR_VO_DISP_OSD2_DMA_CTRL         0x00c6
#define   ADDR_VO_DISP_OSD2_STRIDE           0x00c7

#define   ADDR_VO_OSD_YUV2RGB_CTL            0x00d0
#define   ADDR_VO_OSD_YUV2RGB_COEFF00        0x00d1
#define   ADDR_VO_OSD_YUV2RGB_COEFF01        0x00d2
#define   ADDR_VO_OSD_YUV2RGB_COEFF02        0x00d3
#define   ADDR_VO_OSD_YUV2RGB_COEFF03        0x00d4
#define   ADDR_VO_OSD_YUV2RGB_COEFF10        0x00d5
#define   ADDR_VO_OSD_YUV2RGB_COEFF11        0x00d6
#define   ADDR_VO_OSD_YUV2RGB_COEFF12        0x00d7
#define   ADDR_VO_OSD_YUV2RGB_COEFF13        0x00d8
#define   ADDR_VO_OSD_YUV2RGB_COEFF20        0x00d9
#define   ADDR_VO_OSD_YUV2RGB_COEFF21        0x00da
#define   ADDR_VO_OSD_YUV2RGB_COEFF22        0x00db
#define   ADDR_VO_OSD_YUV2RGB_COEFF23        0x00dc

#define   ADDR_VO_DISP_YUV2RGB_CTL           0x00e0
#define   ADDR_VO_DISP_YUV2RGB_COEF00        0x00e1
#define   ADDR_VO_DISP_YUV2RGB_COEF01        0x00e2
#define   ADDR_VO_DISP_YUV2RGB_COEF02        0x00e3
#define   ADDR_VO_DISP_YUV2RGB_COEF03        0x00e4
#define   ADDR_VO_DISP_YUV2RGB_COEF10        0x00e5
#define   ADDR_VO_DISP_YUV2RGB_COEF11        0x00e6
#define   ADDR_VO_DISP_YUV2RGB_COEF12        0x00e7
#define   ADDR_VO_DISP_YUV2RGB_COEF13        0x00e8
#define   ADDR_VO_DISP_YUV2RGB_COEF20        0x00e9
#define   ADDR_VO_DISP_YUV2RGB_COEF21        0x00ea
#define   ADDR_VO_DISP_YUV2RGB_COEF22        0x00eb
#define   ADDR_VO_DISP_YUV2RGB_COEF23        0x00ec

#define   ADDR_VO_DISP_MIX_LAYER_GLB_EN      0x00f0
#define   ADDR_VO_DISP_MIX_LAYER_GLB_ALPHA0  0x00f1
#define   ADDR_VO_DISP_MIX_LAYER_GLB_ALPHA1  0x00f2
#define   ADDR_VO_DISP_MIX_SEL               0x00f3
#define   ADDR_VO_DISP_BACKGROUND            0x00f4

#define   ADDR_VO_DISP_DITH_CTL              0x00f5
#define   ADDR_VO_DISP_CLUT_CTL              0x00f6

#define   ADDR_VO_DISP_IRQ0_CTL              0x00f8
#define   ADDR_VO_DISP_IRQ1_CTL              0x00f9
#define   ADDR_VO_DISP_IRQ2_CTL              0x00fa
#define   ADDR_VO_DISP_IRQ                   0x00fb


#endif /* VO_DEFINE_H_ */
