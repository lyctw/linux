/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/clk.h>
#include <linux/component.h>
#include <linux/crc-ccitt.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/log2.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/init.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>

#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_of.h>
#include <drm/drmP.h>
#include <video/mipi_display.h>

#include <linux/iopoll.h>
#include <linux/math64.h>
#include <linux/of_device.h>
#include <linux/mfd/syscon.h>
#include <linux/phy/phy.h>
#include <asm/unaligned.h>
#include <uapi/linux/videodev2.h>



#include "kendryte_dsi.h"
#include "kendryte_layer.h"

#define   MCTL_MAIN_DATA_CTL_OFFSET         0x4   //0x00000000 Main Control - main control setting for datapath
#define   MCTL_MAIN_PHY_CTL_OFFSET          0x8   //0x00000000 Main control setting for the physical lanes and drive the static signals for DPHY clock lane
#define   MCTL_MAIN_EN_OFFSET               0xc   //0x00000000 Control start/stop of the DSI link
#define   MCTL_DPHY_TIMEOUT1_OFFSET         0x14  //0x00000000 Main DPHY time-out settings. To better understand the way this register is used
#define   MCTL_DPHY_TIMEOUT2_OFFSET         0x18  //0x00000000 To better understand the way this register is used, please refer to Section : DSI
#define   MCTL_ULPOUT_TIME_OFFSET           0x1c  //0x00000000 Specify time to leave ULP mode. The time-out is reached when the ulpout
#define   MCTL_MAIN_STS_OFFSET              0x24  //0x00000000 Status of the DSI link
#define   DSC_MODE_CTL_OFFSET               0x30  //0x00000000 DSC Mode Control register
#define   DIRECT_CMD_MAIN_SETTINGS_OFFSET    0x84  //0x00000000 Main control of the Direct Command function.
#define   DIRECT_CMD_WRDAT_OFFSET            0x90  //0x00000000 Write data to outgoing Direct Command FIFO, byte 0 to 3; applicable to either
#define   DIRECT_CMD_SEND_OFFSET             0x80  //0x00000000 Direct_cmd_send is not a real register. When this address is written
#define   DIRECT_CMD_FIFO_RST_OFFSET        0x94  //0x00000000 Reset the write FIFO. This register is not a real register - when written it reset
#define   DIRECT_CMD_STS_OFFSET              0x88  //0x00000000 Direct Command Status: To ensure that the observed status bits are coherent
#define   DIRECT_CMD_STS_CLR_OFFSET         0x158 //0x00000000 Direct command status register clear function. Write '1' to clear
#define   DIRECT_CMD_RD_STS_CLR_OFFSET      0x15c //0x00000000 Direct command read status register clear function. Write '1' to clear
#define   DIRECT_CMD_RD_STS_OFFSET          0xa8  //0x00000000 Status of the read command received. It is recommended to clear
#define   DIRECT_CMD_RD_PROPERTY_OFFSET     0xa4  //0x00000000 read command characteristics
#define   DIRECT_CMD_RDDAT_OFFSET           0xa0  //0x00000000 Data from incoming Direct Command receive path, byte 0 to 3. NOTE: SW
#define   VID_MAIN_CTL_OFFSET              0xb0   //0x80000000 Video mode main control
#define   VID_VSIZE1_OFFSET                0xb4   //0x00000000 Image vertical Sync and Blanking settings
#define   VID_VSIZE2_OFFSET                0xb8   //0x00000000 Image vertical active line setting
#define   VID_HSIZE1_OFFSET                0xc0   //0x00000000 Image horizontal sync and Blanking setting
#define   VID_HSIZE2_OFFSET                0xc4   //0x00000000 Image horizontal byte size setting
#define   VID_BLKSIZE1_OFFSET              0xcc   //0x00000000 blanking packet size
#define   VID_BLKSIZE2_OFFSET              0xd0   //0x00000000 Pulse Mode blanking packet size
#define   VID_DPHY_TIME_OFFSET             0xdc   //0x00000000 Time of D-PHY behavior for wakeup time and Line duration for LP during
#define   TVG_CTL_OFFSET                   0xfc   //0x00000000 Main control of the TVGMIPI DSI v1.3.1 Host Controller User Guide
#define   VID_VCA_SETTING1_OFFSET          0xf4   //0x00000000 VCA control register 1
#define   VID_VCA_SETTING2_OFFSET          0xf8   //0x00000000 VCA control register 2
#define   TVG_IMG_SIZE_OFFSET              0x100  //0x00000000 TVG Generated image size
#define   TVG_COLOR1_OFFSET                0x104  //0x00000000 Color 1 of the dummy frame G, R
#define   TVG_COLOR1_OFFSET                0x104  //0x00000000 Color 1 of the dummy frame G, R
#define   TVG_COLOR1_BIS_OFFSET            0x108  //0x00000000 Color 1 of the dummy frame , B
#define   TVG_COLOR2_OFFSET                0x10c  //0x00000000 Color 2 of the dummy frame, G, R
#define   TVG_COLOR2_BIS_OFFSET            0x110  //0x00000000 Color 2 of the dummy frame, B
#define   TVG_STS_OFFSET                   0x114  //0x00000000 TVG status register
#define   CMD_MODE_STS_CTL_OFFSET           0x134 //0x00000000 Controls the enabling and edge detection of command status bits EDGE = 0
#define   VID_MODE_STS_CTL_OFFSET           0x140 //0x00000000 Control the enabling and edge detection of VSG status bits
#define   DPI_IRQ_EN_OFFSET                 0x1a0 //0x00000000 DPI interrupt enable

// config dsi timing 
struct kendryte_dsi_timing dsi_timing =  {

    .HACT=1080,
    .VACT=1920,
    .HSA=20,
    .HBP=20,
    .HFP=220,
    .VSA=5,
    .VBP=8,
    .VFP=5,

};


static void kendryte_dsi_dcs_write_reg(struct kendryte_dsi *dsi, uint32_t reg, uint32_t val)
{
    writel(val, dsi->base + reg);
}

static uint32_t kendryte_dsi_dcs_read_reg(struct kendryte_dsi *dsi, uint32_t reg)
{
    return readl(dsi->base + reg);
}

void kendryte_dsi_wait_phy_pll_locked(struct kendryte_dsi *dsi)
{
    while (1)
    {
        if ((kendryte_dsi_dcs_read_reg(dsi, MCTL_MAIN_STS_OFFSET) & 0x01) == 0x01)
        {     
            break;
        }
    }
}

void kendryte_dsi_wait_cklane_rdy(struct kendryte_dsi *dsi)
{
    while (1)
	 {
        if ((kendryte_dsi_dcs_read_reg(dsi, MCTL_MAIN_STS_OFFSET) & 0x02) == 0x02)
        {     
            break;
        }
    }
}

void kendryte_dsi_wait_dat1_rdy(struct kendryte_dsi *dsi)
{
    while (1)
	{
        if ((kendryte_dsi_dcs_read_reg(dsi, MCTL_MAIN_STS_OFFSET) & 0x04) == 0x04)
        {     
            break;
        }
    }
}

void kendryte_dsi_wait_dat2_rdy(struct kendryte_dsi *dsi)
{
    while (1)
	{
        if ((kendryte_dsi_dcs_read_reg(dsi, MCTL_MAIN_STS_OFFSET) & 0x08) == 0x08)
        {     
            break;
        }
    }
}

void kendryte_dsi_wait_dat3_rdy(struct kendryte_dsi *dsi)
{
    while (1)
    {
        if ((kendryte_dsi_dcs_read_reg(dsi, MCTL_MAIN_STS_OFFSET) & 0x10) == 0x10)
        {     
            break;
        }
    }
}

void kendryte_dsi_wait_dat4_rdy(struct kendryte_dsi *dsi)
{
    while (1)
	{
        if ((kendryte_dsi_dcs_read_reg(dsi, MCTL_MAIN_STS_OFFSET) & 0x20) == 0x20)
        {     
            break;
        }
    }
}

static uint32_t  kendryte_dsi_dcs_build_pkt_hdr(struct kendryte_dsi *dsi, const struct mipi_dsi_msg *msg)                                    
{
    uint32_t pkt = msg->type;

    if (msg->type == MIPI_DSI_DCS_LONG_WRITE) {
            pkt |= ((msg->tx_len + 1) & 0xffff) << 8;
            pkt |= (((msg->tx_len + 1) >> 8) & 0xffff) << 16;
    } else {
            pkt |= (((u8 *)msg->tx_buf)[0] << 8);
            if (msg->tx_len > 1)
                    pkt |= (((u8 *)msg->tx_buf)[1] << 16);
    }

//        pkt |= sun6i_dsi_ecc_compute(pkt) << 24;

    return pkt;
}


static int kendryte_dsi_dcs_write_short(struct kendryte_dsi *dsi, const struct mipi_dsi_msg *msg)                           
{
	int cmd_sts = 0;
	uint32_t reg_val = 0;
	uint8_t *tx_buf = NULL;

	reg_val = 0x01000000 |
		msg->tx_len << 16 |
		msg->type << 8 |
		0x00;

	printk("reg_val %02x", reg_val);
    	kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_MAIN_SETTINGS_OFFSET, reg_val);

	tx_buf = msg->tx_buf;
	if (msg->tx_len == 1)
		reg_val = tx_buf[0];
	else if (msg->tx_len == 2)
		reg_val = tx_buf[0] | tx_buf[1] << 8;
	else
		return -EINVAL;
	kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_WRDAT_OFFSET, reg_val);
	kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_SEND_OFFSET, 0x00);

    	while((cmd_sts & 0x02) != 0x02)
		cmd_sts = kendryte_dsi_dcs_read_reg(dsi, DIRECT_CMD_STS_OFFSET);
	kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_STS_CLR_OFFSET,0x02);

    return msg->tx_len;
}



static int kendryte_dsi_dcs_write_long(struct kendryte_dsi *dsi, const struct mipi_dsi_msg *msg)                         
{
    uint32_t reg, val, val2;
    int cmd_sts;
    uint8_t i = 0;
    int len;
    uint8_t *buff = kzalloc((sizeof(uint8_t) * 70), GFP_KERNEL);

    memcpy(buff, msg->tx_buf, msg->tx_len);

    mdelay(100);

    reg = 0x01003908 | (msg->tx_len << 16);                 //0x01003908

    kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_MAIN_SETTINGS_OFFSET,reg); //[24] cmd_lp_en  =1    0x01043908
                                                    //[23:14] cmd_size  cmd_size = len
                                                    //[13:8] cmd_head  =0x39
                                                    //[3 ] cmd_longnotshort  =0x1
                                                    //[2:0] cmd_nat : 3'b000:write command
                                                    //                3'b001:read command
                                                    //                3'b100:TE request
                                                    //                3'b101:trigger request
                                                    //                3'b110:BTA request
                                      
    
    val = msg->tx_len / 4;       
    for(i = 0; i < val; i++)
    {
        kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_WRDAT_OFFSET,((buff[(4 * i) + 3]<<24) + (buff[(4 * i) + 2]<<16) + (buff[(4 * i) + 1]<<8) + buff[(4 * i)]));// DATA  
    }
    val2 = msg->tx_len % 4;
    if(val2 == 1)
        kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_WRDAT_OFFSET,buff[(4 * val)]);// DATA
    else if(val2 == 2)
        kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_WRDAT_OFFSET,((buff[(4 * val) + 1]<<8) + buff[(4 * val)]));// DATA
    else if(val2 == 3)
        kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_WRDAT_OFFSET,((buff[(4 * val) + 2]<<16) + (buff[(4 * val) + 1]<<8) + buff[(4 * val)]));// DATA

    kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_SEND_OFFSET,0x00000); // cmd send

	while((cmd_sts & 0x02 )!= 0x02)
	{  
		cmd_sts = kendryte_dsi_dcs_read_reg(dsi, DIRECT_CMD_STS_OFFSET);
	}

	kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_STS_CLR_OFFSET,0x02); // clear write_completed

    kfree(buff);

    /*
        * TODO: There's some bits (reg 0x200, bits 8/9) that
        * apparently can be used to check whether the data have been
        * sent, but I couldn't get it to work reliably.
        */
    return msg->tx_len;
}

static int kendryte_dsi_dcs_read(struct kendryte_dsi *dsi,
                              const struct mipi_dsi_msg *msg)
{
    int cmd_sts =0;
    int data = 0;
    int size;
    int err;
    int reg = 0;
    int i = 0;
    int cut = 0;
    uint8_t *tx_buf = msg->tx_buf;
    uint8_t *rx_buf = msg->rx_buf;

    kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_STS_CLR_OFFSET, 0xffffffff);
    kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_RD_STS_CLR_OFFSET, 0xffffffff);
    kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_FIFO_RST_OFFSET, 0xffffffff);

    reg = 0x01000601 | (1 << 16);
    kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_MAIN_SETTINGS_OFFSET,reg);                   //[24] cmd_lp_en  =1            0x01010601          0x01020501
                                                                    //[23:16] cmd_size  =cmd_size. 1 or 2                   
                                                                    //[14:15] cmd_type  0 write  1 read                     1           
                                                                    //[13:8] cmd_head  =0x5                                 6            
                                                                    //[3 ] cmd_longnotshort  =0x1                           1
                                                                    //[2:0] cmd_nat : 3'b000:write command                  01
                                                                    //                3'b001:read command
                                                                    //                3'b100:TE request
                                                                    //                3'b101:trigger request
                                                                    //                3'b110:BTA request   
    
    kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_WRDAT_OFFSET, tx_buf[0]);
    kendryte_dsi_dcs_write_reg(dsi, DIRECT_CMD_SEND_OFFSET,0xffffffff); // cmd send   

    while((cmd_sts & 0x08 )!= 0x08)                                 //wait read success
    {  
        cmd_sts = kendryte_dsi_dcs_read_reg(dsi, DIRECT_CMD_STS_OFFSET);              
    }

    err = kendryte_dsi_dcs_read_reg(dsi, DIRECT_CMD_RD_STS_OFFSET);                   //read err
    size = kendryte_dsi_dcs_read_reg(dsi, DIRECT_CMD_RD_PROPERTY_OFFSET);             // read size [16 - 23]   
    data = kendryte_dsi_dcs_read_reg(dsi, DIRECT_CMD_RDDAT_OFFSET);                   //read data 

    for (i = 0; i < msg->rx_len; i++) {
    	rx_buf[i] = data & 0xFF;
	data = data >> 8;
    }

    return 0;
}


static int kendryte_dsi_attach(struct mipi_dsi_host *host,
                            struct mipi_dsi_device *device)
{
    struct kendryte_dsi *dsi = host_to_kendryte_dsi(host);

    dsi->device = device;
    dsi->panel = of_drm_find_panel(device->dev.of_node);
    if (IS_ERR(dsi->panel))
            return PTR_ERR(dsi->panel);

    dev_info(host->dev, "Attached device %s\n", device->name);

    return 0;
}

static int kendryte_dsi_detach(struct mipi_dsi_host *host,
                            struct mipi_dsi_device *device)
{
    struct kendryte_dsi *dsi = host_to_kendryte_dsi(host);

    dsi->panel = NULL;
    dsi->device = NULL;

}

static ssize_t  kendryte_dsi_transfer(struct mipi_dsi_host *host,
                                  const struct mipi_dsi_msg *msg)
{
    struct kendryte_dsi *dsi = host_to_kendryte_dsi(host);
    int ret;

    switch (msg->type) {
    case MIPI_DSI_DCS_SHORT_WRITE:
//    case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
    case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
    case MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE:
            ret = kendryte_dsi_dcs_write_short(dsi, msg);
            break;

    case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
    case MIPI_DSI_DCS_LONG_WRITE:
            ret = kendryte_dsi_dcs_write_long(dsi, msg);
            break;

    case MIPI_DSI_DCS_READ:
            if (msg->rx_len >= 1 && msg->rx_len <= 4) {
                ret = kendryte_dsi_dcs_read(dsi, msg);
                break;
            }
            /* Else, fall through */

    default:
        ret = -EINVAL;
    }

    return ret;
}

static const struct mipi_dsi_host_ops kendryte_dsi_host_ops = {
    .attach         = kendryte_dsi_attach,
    .detach         = kendryte_dsi_detach,
    .transfer       = kendryte_dsi_transfer,
};


static void kendryte_dsi_set_timing(struct kendryte_dsi *dsi, struct drm_display_mode *mode)
{
    
    struct kendryte_dsi_timing *timing = dsi->timing;
    struct mipi_dsi_device *device = dsi->device;
    unsigned int Bpp = mipi_dsi_pixel_format_to_bpp(device->format) / 8;
    uint16_t hbp = 0, hfp = 0, hsa = 0, hblk = 0, vblk = 0;
    uint32_t basic_ctl = 0;
    size_t bytes;
    uint8_t *buffer;

#if 0
    timing->HSA = (mode->hsync_end - mode->hsync_start) * Bpp ;
    timing->HBP = (mode->htotal - mode->hsync_end) * Bpp ;
    timing->HFP = (mode->hsync_start - mode->hdisplay) * Bpp;

    timing->VSA = (mode->vsync_end - mode->vsync_start) * Bpp;
    timing->VBP = (mode->vtotal - mode->vsync_end) * Bpp;
    timing->VFP = (mode->vsync_start - mode->vdisplay) * Bpp;

    if(device->lanes == 4)          // need write
    {
        // config 4 lan 
    }else
    {
        // config 2 lan
    }
#endif                         
                          

    kendryte_dsi_dcs_write_reg(dsi, TVG_IMG_SIZE_OFFSET, (timing->VACT << 16 ) + timing->HACT * 3);
    kendryte_dsi_dcs_write_reg(dsi, TVG_COLOR1_OFFSET, (0 << 12) +255)  ;  //[23:12] col1_green
                                                            //[11:0]  col1_red
    kendryte_dsi_dcs_write_reg(dsi, TVG_COLOR1_BIS_OFFSET, 0)  ; //[11:0]  col1_blue
    kendryte_dsi_dcs_write_reg(dsi, TVG_COLOR2_OFFSET, (0<<12) +0) ;   //[23:12] col2_green
                                                            //[11:0]  col2_red
    kendryte_dsi_dcs_write_reg(dsi, TVG_COLOR2_BIS_OFFSET,255) ;  //[11:0]  col2_blue


    kendryte_dsi_dcs_write_reg(dsi, VID_VSIZE1_OFFSET,(timing->VFP<<12)+(timing->VBP << 6) + timing->VSA);
                                        //[19:12] vfp_length
                                        //[11:6]  vbp_length
                                        //[5:0]   vsa_length
    kendryte_dsi_dcs_write_reg(dsi, VID_MODE_STS_CTL_OFFSET,0);//0xffffffff);
    kendryte_dsi_dcs_write_reg(dsi, VID_VSIZE2_OFFSET, timing->VACT); //[12:0] vact_length
    kendryte_dsi_dcs_write_reg(dsi, VID_HSIZE1_OFFSET, ((timing->HBP * 3 - 12) << 16)+(timing->HSA * 3 - 14));
                                            //[31:16] hbp_length  =(dpi_hbp*24/8-12)
                                            //[9:0]   hsa_length  =(dpi_hsa*24/8-14)

    kendryte_dsi_dcs_write_reg(dsi, VID_HSIZE2_OFFSET, ((timing->HFP * 3 - 6) << 16)+(timing->HACT * 3));
                                            //[26:16] hfp_length  =(dpi_hfp*24/8-6) >hss+hsa+hse+hbp
                                            //[14:0] rgb_size
    kendryte_dsi_dcs_write_reg(dsi, VID_BLKSIZE2_OFFSET, (timing->HSA + timing->HBP + timing->HFP + timing->HACT) * 3 - 20-(timing->HSA * 3 - 14));
                                            //[14:0] blkline_pulse_pck = (dpi_hsa+dpi_hbp+dpi_hact+dpi_hfp)*24/8-20-((dpi_hsa*24/8)-14)=3342
                                                //  (1080+20+15+72)*3-20= 3541
    kendryte_dsi_dcs_write_reg(dsi, VID_DPHY_TIME_OFFSET, (0x38 << 17)+((timing->HSA + timing->HBP + timing->HFP + timing->HACT)* 3 / 4)-((timing->HSA * 3 - 14) / 4));
                                            //[16:0] reg_line_duration =(line_total_)
                                            //[27:17] reg_wakeup_time
    kendryte_dsi_dcs_write_reg(dsi, VID_VCA_SETTING1_OFFSET, (0x000000));
    kendryte_dsi_dcs_write_reg(dsi, VID_VCA_SETTING2_OFFSET, ((timing->HSA + timing->HBP + timing->HFP + timing->HACT) * 3 - 20 - (timing->HSA * 3 - 14) - 6) <<16 );

}


static void kendryte_dsi_setup_format(struct kendryte_dsi *dsi, struct drm_display_mode *mode)                                 
{
    struct mipi_dsi_device *device = dsi->device;
//        u32 val = SUN6I_DSI_PIXEL_PH_VC(device->channel);
    u8 dt, fmt;
    u16 wc;

    /*
        * TODO: The format defines are only valid in video mode and
        * change in command mode.
    */
    switch (device->format) {
        case MIPI_DSI_FMT_RGB888:
                dt = MIPI_DSI_PACKED_PIXEL_STREAM_24;
                fmt = 8;
                break;
        case MIPI_DSI_FMT_RGB666:
                dt = MIPI_DSI_PIXEL_STREAM_3BYTE_18;
                fmt = 9;
                break;
        case MIPI_DSI_FMT_RGB666_PACKED:
                dt = MIPI_DSI_PACKED_PIXEL_STREAM_18;
                fmt = 10;
                break;
        case MIPI_DSI_FMT_RGB565:
                dt = MIPI_DSI_PACKED_PIXEL_STREAM_16;
                fmt = 11;
                break;
        default:
                return;
    }

    // need change mode 
    kendryte_dsi_dcs_write_reg(dsi, TVG_COLOR1_OFFSET,(0<<12) +255)  ;  //[23:12] col1_green                                                                //[11:0]  col1_red
    kendryte_dsi_dcs_write_reg(dsi, TVG_COLOR1_BIS_OFFSET,0)  ; //[11:0]  col1_blue
    kendryte_dsi_dcs_write_reg(dsi, TVG_COLOR2_OFFSET,(0<<12) +0) ;   //[23:12] col2_green                                                           //[11:0]  col2_red
    kendryte_dsi_dcs_write_reg(dsi, TVG_COLOR2_BIS_OFFSET,255) ;  //[11:0]  col2_blue
    kendryte_dsi_dcs_write_reg(dsi, VID_MAIN_CTL_OFFSET,(0x1<<31)+(0x1<<25)+(0x0<<21)+(1<<20)+(1<<19)+(3<<14)+(0x3e<<8)+0); //0x1 << 21

    // set test mode
#if 0
    kendryte_dsi_dcs_write_reg(dsi, MCTL_MAIN_DATA_CTL_OFFSET, 0x67);
    kendryte_dsi_dcs_write_reg(dsi, TVG_CTL_OFFSET, (6<<5)+(2<<3) + (2<<1) + 1);
#endif
}

static void kendryte_dsi_init(struct kendryte_dsi *dsi, struct drm_display_mode *mode)   
{

    struct mipi_dsi_device *device = dsi->device;
    uint32_t data = 0;
#if 0
    if(device->lanes == 4)
    {
        // config 4 lan 
    }else
    {
        // config 2 lan
    }
#endif
    kendryte_dsi_dcs_write_reg(dsi, DPI_IRQ_EN_OFFSET, 0); //enable dpi overflow int
    kendryte_dsi_dcs_write_reg(dsi, MCTL_MAIN_PHY_CTL_OFFSET, 0x3C17);    //[6]  lane1_ulp_en = 1   正常的   0x3C17 clk_continuous  
                                                               //[4]  clk_continuous  =1
                                                               //[2]  lane4_en =1
                                                               //[1]  lane3_en =1
                                                               //[0]  lane2_en =1
    
    kendryte_dsi_dcs_write_reg(dsi, 0x10, 0x100600);

    kendryte_dsi_dcs_write_reg(dsi, MCTL_DPHY_TIMEOUT1_OFFSET, 0xed8afffb);
    kendryte_dsi_dcs_write_reg(dsi, MCTL_DPHY_TIMEOUT2_OFFSET, 0xf30fffff);
    kendryte_dsi_dcs_write_reg(dsi, MCTL_MAIN_DATA_CTL_OFFSET, 0x2e027); //[6] tvg_sel = 1  test video generator enabled   //default 0x27                         0x2e067
                                                        //[5] vid_en = 1   enable the video stream generator
                                                        //[3:2] vid_if_select =2'b00  00:sdi;01:dpi��10:DSC
                                                        //[1] sdi_if_vid_mode = 1  select video mode
                                                        //[0] link_en = 1; // enable link

    kendryte_dsi_dcs_write_reg(dsi, MCTL_MAIN_EN_OFFSET, 0x40f9);               //4 lan
                                                    //[15]   if3_en: enable dsc interface
                                                    //[14]   if2_en: enable dpi interface   =1
                                                    //[13]   if1_en: enable sdi interface
                                                    //[9]   lane1_ulp_req =1
                                                    //[7]   dat4_en
                                                    //[6]   dat3_en
                                                    //[5]   dat2_en
                                                    //[4]   dat1_en
                                                    //[3]   cklane_en
    data = kendryte_dsi_dcs_read_reg(dsi, CMD_MODE_STS_CTL_OFFSET);
    printk("CMD_MODE_STS_CTL_OFFSET data 0x%x\n",data);
    data &= ~ (1 << 0); 
    kendryte_dsi_dcs_write_reg(dsi, CMD_MODE_STS_CTL_OFFSET, data);
                                                        
    kendryte_dsi_wait_phy_pll_locked(dsi);
    printk("phy pll locked ");

    kendryte_dsi_wait_cklane_rdy(dsi);
    printk("cklane  is ready!!! ");

    kendryte_dsi_wait_dat1_rdy(dsi);
    printk("dat1  is ready!!! ");

    kendryte_dsi_wait_dat2_rdy(dsi);
    printk("dat2  is ready!!! ");
    
    kendryte_dsi_wait_dat3_rdy(dsi);
    printk("dat3  is ready!!! ");

    kendryte_dsi_wait_dat4_rdy(dsi);
    printk("dat4  is ready!!! ");

    // will init lcd  ?????
    kendryte_dsi_dcs_write_reg(dsi, MCTL_ULPOUT_TIME_OFFSET,0x0003ab05);
}


void kendryte_dsi_get_reg_val(struct kendryte_dsi *dsi)
{
    int i = 0; 

    for(i =0; i < 0x200; i= i+4)  //0x49c
    {
    	printk("DSI REG:Reg[0x%x]= 0x%x\n", dsi->base + i, readl(dsi->base + i));
    }
}


static void kendryte_dsi_encoder_enable(struct drm_encoder *encoder)
{
    struct drm_display_mode *mode = &encoder->crtc->state->adjusted_mode;
    struct kendryte_dsi *dsi = encoder_to_kendryte_dsi(encoder);
    struct mipi_dsi_device *device = dsi->device;
    struct kendryte_dsi_timing *timing = dsi->timing;  
//    union phy_configure_opts opts = { };
//    struct phy_configure_opts_mipi_dphy *cfg = &opts.mipi_dphy;
    
    uint32_t delay;


//        pm_runtime_get_sync(dsi->dev);

/*
    // config tx d-phy
    kendryte_tx_phy_init(dsi->dphy, dsi->base);      // config 

    phy_init(dsi->dphy);                //system

    phy_mipi_dphy_get_default_config(mode->clock * 1000,
                                        mipi_dsi_pixel_format_to_bpp(device->format),
                                        device->lanes, cfg);

    phy_set_mode(dsi->dphy, PHY_MODE_MIPI_DPHY);
    phy_configure(dsi->dphy, &opts);
    phy_power_on(dsi->dphy);

*/
    // config dsi timing format init
    kendryte_dsi_init(dsi, mode);


    if (!IS_ERR(dsi->panel))
            drm_panel_prepare(dsi->panel);

    /*
        * FIXME: This should be moved after the switch to HS mode.
        *
        * Unfortunately, once in HS mode, it seems like we're not
        * able to send DCS commands anymore, which would prevent any
        * panel to send any DCS command as part as their enable
        * method, which is quite common.
        *
        * I haven't seen any artifact due to that sub-optimal
        * ordering on the panels I've tested it with, so I guess this
        * will do for now, until that IP is better understood.
        */
    if (!IS_ERR(dsi->panel))
            drm_panel_enable(dsi->panel);

    kendryte_dsi_set_timing(dsi, mode);
    kendryte_dsi_setup_format(dsi, mode);

//    kendryte_dsi_get_reg_val(dsi);

}


static void kendryte_dsi_encoder_disable(struct drm_encoder *encoder)
{
    struct kendryte_dsi *dsi = encoder_to_kendryte_dsi(encoder);

    printk("Disabling DSI output\n");

    if (!IS_ERR(dsi->panel)) {
            drm_panel_disable(dsi->panel);
            drm_panel_unprepare(dsi->panel);
    }

#if 0
    phy_power_off(dsi->dphy);
    phy_exit(dsi->dphy);

    pm_runtime_put(dsi->dev);
#endif
}

static int
kendryte_dsi_encoder_atomic_check(struct drm_encoder *encoder,
				 struct drm_crtc_state *crtc_state,
				 struct drm_connector_state *conn_state)
{
	struct kendryte_crtc_state *s = to_kendryte_crtc_state(crtc_state);
	struct kendryte_dsi *dsi = encoder_to_kendryte_dsi(encoder);
	struct drm_connector *connector = conn_state->connector;
	struct drm_display_info *info = &connector->display_info;

//	printk("kendryte_dsi_encoder_atomic_check  *************************** \n");

	switch (dsi->format) {
	case MIPI_DSI_FMT_RGB888:
		s->output_mode = 0;//ROCKCHIP_OUT_MODE_P888;
		break;
	case MIPI_DSI_FMT_RGB666:
		s->output_mode = 1;//ROCKCHIP_OUT_MODE_P666;
		break;
	case MIPI_DSI_FMT_RGB565:
		s->output_mode = 2;//ROCKCHIP_OUT_MODE_P565;
		break;
	default:
		WARN_ON(1);
		return -EINVAL;
	}

	if (info->num_bus_formats)
		s->bus_format = info->bus_formats[0];
	else
		s->bus_format = MEDIA_BUS_FMT_RGB888_1X24;

	s->output_type = DRM_MODE_CONNECTOR_DSI;
	s->bus_flags = info->bus_flags;
	s->tv_state = &conn_state->tv;
//	s->eotf = TRADITIONAL_GAMMA_SDR;
//	s->color_space = V4L2_COLORSPACE_DEFAULT;


	return 0;
}

static const struct drm_encoder_helper_funcs kendryte_dsi_enc_helper_funcs = {
    .disable = kendryte_dsi_encoder_disable,
    .enable  = kendryte_dsi_encoder_enable,
    .atomic_check = kendryte_dsi_encoder_atomic_check,
};



static const struct drm_encoder_funcs kendryte_dsi_enc_funcs = {
    .destroy        = drm_encoder_cleanup,
};


static int kendryte_dsi_get_modes(struct drm_connector *connector)
{
    struct kendryte_dsi *dsi = connector_to_kendryte_dsi(connector);

    return drm_panel_get_modes(dsi->panel);
}

static struct drm_connector_helper_funcs kendryte_dsi_connector_helper_funcs = {
    .get_modes      = kendryte_dsi_get_modes,
};


static enum drm_connector_status kendryte_dsi_connector_detect(struct drm_connector *connector, bool force)
{
    return connector_status_connected;
}

static const struct drm_connector_funcs kendryte_dsi_connector_funcs = {
    .detect                 = kendryte_dsi_connector_detect,
    .fill_modes             = drm_helper_probe_single_connector_modes,
    .destroy                = drm_connector_cleanup,
    .reset                  = drm_atomic_helper_connector_reset,
    .atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
    .atomic_destroy_state   = drm_atomic_helper_connector_destroy_state,
};



static int kendryte_dsi_bind(struct device *dev, struct device *master,
                         void *data)
{
    struct drm_device *drm = data;
    struct kendryte_drv *drv = drm->dev_private;
    struct kendryte_dsi *dsi = dev_get_drvdata(dev);
    struct drm_encoder *encoder = &dsi->encoder;
    struct drm_connector *connector = &dsi->connector;
    int ret;

    printk("kendryte_dsi_bind is start  \n");


    if (!dsi->panel)
            return -EPROBE_DEFER;

    dsi->drv = drv;

    ret = drm_of_find_panel_or_bridge(dev->of_node, 1, -1, &dsi->panel, &dsi->bridge);// 
                                
#if 0
    dsi->encoder.possible_crtcs = drm_of_find_possible_crtcs(drm, dev->of_node);
#else
    dsi->encoder.possible_crtcs = BIT(0);//)drm_of_find_possible_crtcs(drm, dev->of_node);
#endif
    if(dsi->encoder.possible_crtcs == 0)
        printk("dsi->encoder.possible_crtcs is err \n");

    ret = drm_encoder_init(drm,
                        &dsi->encoder,
                        &kendryte_dsi_enc_funcs,
                        DRM_MODE_ENCODER_DSI,
                        NULL);
    if (ret) {
            dev_err(dsi->dev, "Couldn't initialise the DSI encoder\n");
            return ret;
    }

    drm_encoder_helper_add(&dsi->encoder,
                            &kendryte_dsi_enc_helper_funcs);

//    printk("drm_encoder_init encode id is %d \n", encoder->base.id);
                                                           
    drm_connector_helper_add(&dsi->connector,
                                &kendryte_dsi_connector_helper_funcs);


    ret = drm_connector_init(drm, &dsi->connector,
                                &kendryte_dsi_connector_funcs,
                                DRM_MODE_CONNECTOR_DSI);
    if (ret) {
            dev_err(dsi->dev,
                    "Couldn't initialise the DSI connector\n");
            goto err_cleanup_connector;
    }

    drm_mode_connector_attach_encoder(&dsi->connector, &dsi->encoder);
    drm_panel_attach(dsi->panel, &dsi->connector);

    // enable dsi encode
//    kendryte_dsi_enc_helper_funcs.enable(&dsi->encoder);

    return 0;

err_cleanup_connector:
    drm_encoder_cleanup(&dsi->encoder);
    return ret;
}


static void kendryte_dsi_unbind(struct device *dev, struct device *master,
                            void *data)
{
    struct kendryte_dsi *dsi = dev_get_drvdata(dev);

    drm_panel_detach(dsi->panel);
}

static const struct component_ops  kendryte_dsi_ops = {
    .bind   = kendryte_dsi_bind,
    .unbind = kendryte_dsi_unbind,
};


static void clk_get_reg(void)
{
    
    uint32_t *addr = ioremap(0x97001000 + 0x134, 0x08);        // evb  0x00000107  linux   0x00000108   pix freq 8    应该是7 才对
    uint32_t *addr2 = ioremap(0x97001000 + 0x14c, 0x08);       //  evb 0x00000100   linux  0x00000100
    uint32_t *addr3 = ioremap(0x97001000 + 0x118, 0x08);       // evb 0x00000016     linux 0x2f       DSI_APB  freq 是2 ，应该是1 才对

    printk("SYSCTL_CLK_DISPLAY_PIXEL is %x \n", readl(addr));
    printk("SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_DSI_SYSTEM is %x \n", readl(addr2));
    printk("SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_DSI_APB is %x \n", readl(addr3));

    iounmap(addr);
    iounmap(addr2);
    iounmap(addr3);
}

static int kendryte_dsi_probe(struct platform_device *pdev)
{
    struct kendryte_dsi *dsi;
    struct resource *res;
    struct device *dev = &pdev->dev;
    struct device_node *np =dev->of_node;
    int id;
//    void __iomem *base;
    int ret;

    printk("start kendryte_dsi_probe \n");
    dsi = devm_kzalloc(dev, sizeof(*dsi), GFP_KERNEL);
    if (!dsi)
    {
        printk("devm_kzalloc dsi failed \n");
        return -ENOMEM;
    }
        
#if 0
    dsi->dphy = devm_kzalloc(dev, sizeof(struct kendryte_tx_phy), GFP_KERNEL);
    if (!dsi->dphy)
        return -ENOMEM;
#endif 

    dsi->timing = devm_kzalloc(dev, sizeof(struct kendryte_dsi_timing), GFP_KERNEL);
    if (!dsi->timing)
    {
        printk("devm_kzalloc dsi timing failed \n ");
        return -ENOMEM;
    }

    id = of_alias_get_id(dev->of_node, "dsi");
	if (id < 0)
		id = 0;
        
    platform_set_drvdata(pdev, dsi);
    dsi->dev = dev;
    dsi->id = id;
    dsi->host.ops = &kendryte_dsi_host_ops;
    dsi->host.dev = dev;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    dsi->base = devm_ioremap_resource(dev, res);
    if (IS_ERR(dsi->base)) {
        dev_err(dev, "Couldn't map the DSI encoder registers\n");
        return PTR_ERR(dsi->base);
    }

    of_property_read_u32(np, "framerate", &dsi->frame_rate);

    dsi->dsi_apb = devm_clk_get(&pdev->dev, "dsi_apb");
    if (IS_ERR(dsi->dsi_apb)) {
        printk("get dsi_apb failed \n");
        return PTR_ERR(dsi->dsi_apb);
    } 

    dsi->dsi_system = devm_clk_get(&pdev->dev, "dsi_system");
    if (IS_ERR(dsi->dsi_system)) {
        printk("get dsi_system failed \n");
        return PTR_ERR(dsi->dsi_system);
    } 

    dsi->dsi_pixel = devm_clk_get(&pdev->dev, "display_pixel");
    if (IS_ERR(dsi->dsi_pixel)) {
         printk("get dsi_pixel failed \n");
        return PTR_ERR(dsi->dsi_pixel);
    } 

    // //enable display apb clk
    ret = clk_prepare_enable(dsi->dsi_apb);                     
    if(ret < 0)
    {
        printk("mipi d-phy enable dsi_apb failed \n");
        return ret;
    }
    // enable dsi_pixel clk
    ret = clk_prepare_enable(dsi->dsi_pixel);                     
    if(ret < 0)
    {
        printk("mipi d-phy enable dsi_pixel failed \n");
        return ret;
    }

    //enable display dsi_system
    ret = clk_prepare_enable(dsi->dsi_system);                     
    if(ret < 0)
    {
        printk("mipi d-phy enable dsi_system failed \n");
        return ret;
    }

    // apb 2 freq  二分频设置的是 1 
 ///   clk_set_rate(dsi->dsi_apb, 166000000);     
    clk_set_rate(dsi->dsi_system, 166000000);          

    if(dsi->frame_rate == 30)
    {
        //b. set pix clk to 74.25M by default
        clk_set_rate(dsi->dsi_pixel, 74250000);          /// p30 594 / 8
    }else if(dsi->frame_rate == 60)    
    {
        //b. set pix clk to 74.25M by default
        clk_set_rate(dsi->dsi_pixel, 148500000);          /// 594 / 4 = 148.5 p60
    }
    else                                              // default p30
    {
        //b. set pix clk to 74.25M by default
        clk_set_rate(dsi->dsi_pixel, 74250000);          /// p30 594 / 8 
    }

    clk_get_reg();

    printk("get clk success \n");
    /*Parsing dsi timing**/
    dsi->timing = &dsi_timing;                  //need malloc     

    /*Parsing tx d-phy timing**/
//    dsi->kendryte_tx_phy = &tx_dphy;

/*
    dsi->phy = devm_phy_optional_get(dev, "mipi_dphy");
    if (IS_ERR(dsi->phy)) {
        ret = PTR_ERR(dsi->phy);
        DRM_DEV_ERROR(dev, "failed to get mipi dphy: %d\n", ret);
        return ret;
    }
*/
    ret = mipi_dsi_host_register(&dsi->host);
    if (ret) {
        dev_err(dev, "Couldn't register MIPI-DSI host\n");
        return -1;
    }

    ret = component_add(&pdev->dev, &kendryte_dsi_ops);
    if (ret) {
        dev_err(dev, "Couldn't register our component\n");
        goto err_remove_dsi_host;
    }

    return 0;

err_remove_dsi_host:
        mipi_dsi_host_unregister(&dsi->host);

    return ret;

}


static int kendryte_dsi_remove(struct platform_device *pdev)
{
        
    return 0;
}



static int __maybe_unused kendryte_dsi_runtime_resume(struct device *dev)
{


    return 0;
}


static int __maybe_unused kendryte_dsi_runtime_suspend(struct device *dev)
{

        return 0;
}


static const struct dev_pm_ops kendryte_dsi_pm_ops = {
    SET_RUNTIME_PM_OPS(kendryte_dsi_runtime_suspend,
                        kendryte_dsi_runtime_resume,
                        NULL)
};


static const struct of_device_id kendryte_dsi_of_table[] = {
    { .compatible = "kendryte,kendryte-k510-dsi" },
    { }
};



struct platform_driver kendryte_dsi_driver = {
    .probe          = kendryte_dsi_probe,
    .remove         = kendryte_dsi_remove,
    .driver         = {
        .name           = "kendryte_dsi",
        .of_match_table = kendryte_dsi_of_table,
        .pm             = &kendryte_dsi_pm_ops,
    },
};
EXPORT_SYMBOL(kendryte_dsi_driver);

//module_platform_driver(kendryte_dsi_driver);


MODULE_DESCRIPTION("kendryte 510 dsi Driver");
MODULE_LICENSE("GPL");

