/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include "linux/delay.h"
#include "../../k510isp_com.h"
#include "../../k510_isp.h"
#include "../../k510video_subsystem.h"
#include "../core/isp_core_drv.h"
#include "isp_table_drv.h"

extern int ISP_2K_WDR_L3_TABLE[1024];
extern int ISP_2K_WDR_M3_TABLE[1024];
extern int ISP_2K_WDR_S3_TABLE[1024];
extern int ISP_2K_WDR_L2_TABLE[1024];
extern int ISP_2K_WDR_S2_TABLE[1024];

extern int ISP_2K_YUV_GAMMA[256];
extern int ISP_2K_RGB_GAMMA[256];


static int IspCoreConfigTable(struct k510_isp_device *isp,int a[],int num,ISP_TABLE table)
{
    

	int isp_ram_ready = Isp_Drv_R2k_Core_GetRamRdStatus(isp);
	while((isp_ram_ready & 0x80) != 0x80)
	{
		msleep(500);

		isp_ram_ready = Isp_Drv_R2k_Core_GetRamRdStatus(isp);
		if((isp_ram_ready & 0x80) != 0x80)
		{
			printk(" isp share config ram is not ready for wr!!!!!!!!!!\n");
            return 1;
		}
		break;
	}

    int i =0;
    for (i =0;i<num; i++)
    {
  		//apstIspTableReg->isp_table[i] = (unsigned int )a[i]; 
        isp_reg_writel(isp,(unsigned int )a[i],ISP_IOMEM_R2K_CORE_TABLE,i<<2);
    }
	//ISP_CORE_DRV_SET_CoreRamWrStatus(apstIspCoreReg,(0x1<<table));
	//ISP_CORE_DRV_SET_CoreRamWrStatus(apstIspCoreReg,0x00);
    Isp_Drv_R2k_Core_SetRamWrStatus(isp,(0x1<<table));
    Isp_Drv_R2k_Core_SetRamWrStatus(isp,0x00);
	//writel((ISP_2K_BASE_ADDR + 0xc9<<2),(0x1<<table));
	//writel((ISP_2K_BASE_ADDR + 0xc9<<2),0x00);
    switch(table)
    {
    	case RGB_GAMMA: 
			printk("Isp2K RGB Gamma TABLE config done!\n"); 
			break;
    	case YUV_GAMMA: 
			printk("Isp2K YUV Gamma TABLE config done!\n");
			break;
    	default: 
			printk(" NO ANY Isp2K TABLE will be configed!!!!!!!!\n");
			break;
    }
    return	0;	
}
/*
 * 
 */
void isp_r2k_core_table_init(struct k510_isp_device *isp)
{
    printk("isp_r2k_core_table_init start！\n");
    IspCoreConfigTable(isp,ISP_2K_RGB_GAMMA,256,RGB_GAMMA);
    IspCoreConfigTable(isp,ISP_2K_YUV_GAMMA,256,YUV_GAMMA);    
    return;
}
