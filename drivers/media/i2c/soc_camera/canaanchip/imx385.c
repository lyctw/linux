/*
 * Driver for IMX385 CMOS Image Sensor from Sony
 *
 * Copyright (C) 2022, Canaan Bright Sight Co., Ltd
 * Copyright (C) 2014, Andrew Chew <achew@nvidia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * V0.0X01.0X01 add enum_frame_interval function.
 * V0.0X01.0X02 add function g_mbus_config.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <linux/version.h>
#include <linux/canaan-camera-module.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-image-sizes.h>
#include <media/v4l2-mediabus.h>
// gpio 
#include <linux/gpio/driver.h>
#include <linux/gpio/consumer.h>

#define DRIVER_VERSION			KERNEL_VERSION(0, 0x01, 0x0)

/* IMX385 supported geometry */
#define IMX385_TABLE_END				0xffff
#define IMX385_ANALOGUE_GAIN_MIN		0
#define IMX385_ANALOGUE_GAIN_MAX		4095
#define IMX385_ANALOGUE_GAIN_DEFAULT	0

/* In dB*256 */
#define IMX385_DIGITAL_GAIN_MIN			0
#define IMX385_DIGITAL_GAIN_MAX			4095
#define IMX385_DIGITAL_GAIN_DEFAULT		0

#define IMX385_DIGITAL_EXPOSURE_MIN		0
#define IMX385_DIGITAL_EXPOSURE_MAX		1125
#define IMX385_DIGITAL_EXPOSURE_DEFAULT	560

/*
 * Constants for sensor reset delay
 */
#define IMX385_RESET_DELAY1			(2000)
#define IMX385_RESET_DELAY2			(2200)

#define IMX385_EXP_LINES_MARGIN			4

#define IMX385_NAME						"IMX385"

#define IMX385_LANES					4 //2

static const s64 link_freq_menu_items[] = {
	456000000,
};

struct imx385_reg {
	u16 addr;
	u8 val;
};

struct imx385_mode {
	u32 width;
	u32 height;
	struct v4l2_fract max_fps;
	u32 hts_def;
	u32 vts_def;
	const struct imx385_reg *reg_list;
};

/* MCLK:24MHz  1920x1080  30fps   MIPI LANE2 */
static const struct imx385_reg imx385_init_tab_1920_1080_30fps[] = {
	{0x3000,0x01},
	{0x3001,0x00},	
	{0x3002,0x01},
    {0x3004,0x10},
    {0x3005,0x01},
    {0x3007,0x03},
    {0x3009,0x02},
    {0x300a,0x00},
    {0x300b,0x00},
    {0x3012,0x2c},
    {0x3013,0x01},
    {0x3014,0x80},
    {0x3015,0x00},
    {0x3016,0x09},
    {0x3018,0x65},
    {0x3019,0x04},
    {0x301a,0x00},
    {0x301b,0x30},
    {0x301c,0x11},
    {0x3020,0x02},
    {0x3021,0x00},
    {0x3022,0x00},
    {0x303a,0x0c},
    {0x303b,0x00},
    {0x303c,0x00},
    {0x303d,0x00},
    {0x303e,0x49},
    {0x303f,0x04},
    {0x3044,0x01},
    {0x3046,0x30},
    {0x3047,0x38},
    {0x3049,0x0a},
    {0x3054,0x66},
    {0x305c,0x18},
    {0x305d,0x10},
    {0x305e,0x20},
    {0x305f,0x10},
    {0x310b,0x07},
    {0x3110,0x12},
    {0x31ed,0x38},
    {0x3338,0xd4},
    {0x3339,0x40},
    {0x333a,0x10},
    {0x333b,0x00},
    {0x333c,0xd4},
    {0x333d,0x40},
    {0x333e,0x10},
    {0x333f,0x00},
    {0x3344,0x00},
    {0x3346,0x01},
    {0x3357,0x49},
    {0x3358,0x04},
    {0x336b,0x3f},
    {0x336c,0x1f},
    {0x337d,0x0c},
    {0x337e,0x0c},
    {0x337f,0x01},
    {0x3380,0x40},
    {0x3381,0x4a},
    {0x3382,0x67},
    {0x3383,0x1F},
    {0x3384,0x3f},
    {0x3385,0x27},
    {0x3386,0x1F},
    {0x3387,0x17},
    {0x3388,0x77},
    {0x3389,0x27},
    {0x338d,0x67},
    {0x338e,0x03},
	{0x3000,0x00},//
    {0x3002,0x00},
	{IMX385_TABLE_END, 0x00}
};

static const struct imx385_reg start[] = {
	{0x3000, 0x00},		/* mode select streaming on */
	{IMX385_TABLE_END, 0x00}
};

static const struct imx385_reg stop[] = {
	{0x3000, 0x01},		/* mode select streaming off */
	{IMX385_TABLE_END, 0x00}
};

enum {
	TEST_PATTERN_DISABLED,
	TEST_PATTERN_MULTI_PIXELS,
	TEST_PATTERN_SEQUENCE1,
	TEST_PATTERN_HORIZONTAL_COLOR_BAR,
	TEST_PATTERN_VERTICAL_COLOR_BAR,
	TEST_PATTERN_SEQUENCE2,
	TEST_PATTERN_GRADATION_PATTERN1,
	TEST_PATTERN_GRADATION_PATTERN2,
	TEST_PATTERN_000_555_TOGGLE_PATTERN,
	TEST_PATTERN_4PIXEL_PATTERN,
 	TEST_PATTERN_HORIZONTAL1_ROW,
	TEST_PATTERN_VERTICAL1_COLUMN,
	TEST_PATTERN_1ROW_AND_1_COLUMN,
	TEST_PATTERN_STRIPE_PATTERN,
	TEST_PATTERN_CHECKER_PATTERN,
	TEST_PATTERN_1_PIXEL_PATTERN,
	TEST_PATTERN_HORIZONTAL_2PIXEL_PATTERN,
	TEST_PATTERN_MAX,
};

static const char *const tp_qmenu[] = {
	"Disabled",
	"Multiple Pixels Pattern",
	"Sequence Pattern 1",
	"Horizontal Color-bar Chart",
	"Vertical Color-bar Chart",
	"Sequence Pattern 2",
	"Gradation Pattern 1",
	"Gradation Pattern 2",
	"000h/555h Toggle Pattern",
	"4-pixel Pattern",
	"Horizontal 1 Row Pattern",
	"Vertical 1 Column Pattern",
	"1 Row and 1 Column Pattern",
	"Stripe Pattern of the Arbitrary Value",
	"Checker pattern of the Arbitrary Value",
	"1-pixel Pattern",
	"Horizontal 2-pixel Pattern",		
};

#define SIZEOF_I2C_TRANSBUF 32

struct imx385 {
	struct v4l2_subdev subdev;
	struct media_pad pad;
	struct v4l2_ctrl_handler ctrl_handler;
	struct clk *clk;
	struct v4l2_rect crop_rect;
	int hflip;
	int vflip;
	u8 analogue_gain;
	u16 digital_gain;	/* bits 11:0 */
	u16 exposure_time;

	u16 test_pattern;
	u16 test_pattern_pgthru;
	u16 test_pattern_colorwidth;
	u16 test_pattern_pg_mode;
	u16 test_pattern_pghpos;
	u16 test_pattern_pgvpos;
	u16 test_pattern_pghpstep;
	u16 test_pattern_pgvpstep;
	u16 test_pattern_pghpnum;
	u16 test_pattern_pgvpnum;
	u16 test_pattern_pghprm;
	u16 test_pattern_pgdata1;
	u16 test_pattern_pgdata2;

	struct v4l2_ctrl *hblank;
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *pixel_rate;
	const struct imx385_mode *cur_mode;
	u32 cfg_num;
	u16 cur_vts;
	u32 module_index;
	const char *module_facing;
	const char *module_name;
	const char *len_name;
	struct gpio_desc *imx385_powerdown;
	struct gpio_desc *imx385_reset;
	struct mutex lock; /* mutex lock for operations */
};
/**
 * @brief 
 * 
 */
static const struct imx385_mode supported_modes[] = {
	{
		.width = 1920,
		.height = 1080,
		.max_fps = {
			.numerator = 10000,
			.denominator = 300000,
		},
		.hts_def = 0x0898 - IMX385_EXP_LINES_MARGIN,
		.vts_def = 0x0465,
		.reg_list = imx385_init_tab_1920_1080_30fps,
	},
};
/**
 * @brief 
 * 
 * @param client 
 * @return struct imx385* 
 */
static struct imx385 *to_imx385(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct imx385, subdev);
}
/**
 * @brief 
 * 
 * @param client 
 * @param addr 
 * @param data 
 * @return int 
 */
static int reg_write(struct i2c_client *client, const u16 addr, const u8 data)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	u8 tx[3];
	int ret;

	msg.addr = client->addr;
	msg.buf = tx;
	msg.len = 3;
	msg.flags = 0;
	tx[0] = addr >> 8;
	tx[1] = addr & 0xff;
	tx[2] = data;
	ret = i2c_transfer(adap, &msg, 1);
	mdelay(2);

	return ret == 1 ? 0 : -EIO;
}
/**
 * @brief 
 * 
 * @param client 
 * @param addr 
 * @return int 
 */
static int reg_read(struct i2c_client *client, const u16 addr)
{
	u8 buf[2] = {addr >> 8, addr & 0xff};
	int ret;
	struct i2c_msg msgs[] = {
		{
			.addr  = client->addr,
			.flags = 0,
			.len   = 2,
			.buf   = buf,
		}, {
			.addr  = client->addr,
			.flags = I2C_M_RD,
			.len   = 1,
			.buf   = buf,
		},
	};

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		dev_warn(&client->dev, "Reading register %x from %x failed\n",
			 addr, client->addr);
		return ret;
	}

	return buf[0];
}
/**
 * @brief 
 * 
 * @param client 
 * @param table 
 * @return int 
 */
static int reg_write_table(struct i2c_client *client,
			   const struct imx385_reg table[])
{
	const struct imx385_reg *reg;
	int ret;

	for (reg = table; reg->addr != IMX385_TABLE_END; reg++) {
		ret = reg_write(client, reg->addr, reg->val);
		if (ret < 0)
			return ret;
	}

	return 0;
}
/**
 * @brief 
 * 
 * @param client 
 * @param table 
 * @return int 
 */
static int reg_read_table(struct i2c_client *client,
			   const struct imx385_reg table[])
{
	const struct imx385_reg *reg;
	int ret;

	for (reg = table; reg->addr != IMX385_TABLE_END; reg++) {
		dev_info(&client->dev,"%s:addr(0x%x),val(0x%x)\n",__func__,reg->addr,reg_read(client,reg->addr));
	}

	return 0;
}
/**
 * @brief 
 * 
 */
/* V4L2 subdev video operations */
static int imx385_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx385 *priv = to_imx385(client);
	u8 reg = 0x00;
	int ret;

	dev_info(&client->dev,"%s:imx385-0 start\n",__func__);
	if (!enable)
	{
		dev_err(&client->dev,"%s:enable(%d)\n",__func__,enable);
		return reg_write_table(client, stop);
	}	

	ret = reg_write_table(client, priv->cur_mode->reg_list);
	if (ret)
	{
		dev_err(&client->dev,"%s:reg_write_table failed,ret(%d)\n",__func__,ret);
		return ret;
	}	
	//
	//reg_read_table(client, priv->cur_mode->reg_list);
	/* Handle flip/mirror */
	if (priv->vflip)
		reg |= 0x1;
	if (priv->hflip)
		reg |= 0x2;

	ret = reg_write(client, 0x3007, reg);
	if (ret)
	{
		dev_err(&client->dev,"%s:reg(0x3007) failed,ret(%d)\n",__func__,ret);
		return ret;
	}
	/* Handle test pattern */
	if (priv->test_pattern) {
		ret = reg_write(client, 0x300a, 0x00); //Black level offset value setting Set to “000h” when using Pattern Generator
		ret = reg_write(client, 0x300b, 0x00);
		ret = reg_write(client, 0x300e, 0x00);
		ret = reg_write(client, 0x3089, 0x00);
		reg = (priv->test_pattern &0x1)|((priv->test_pattern_pgthru &0x1)<<1)|((priv->test_pattern_colorwidth &0x3)<<2)|((priv->test_pattern_pg_mode &0xf)<<4);
		ret = reg_write(client, 0x308c, reg);//0x2d);
		ret = reg_write(client, 0x308d, priv->test_pattern_pghpos & 0xff);
		ret = reg_write(client, 0x308e, (priv->test_pattern_pghpos >>8) & 0xf);
		ret = reg_write(client, 0x308f, priv->test_pattern_pgvpos & 0xff);
		ret = reg_write(client, 0x3090, (priv->test_pattern_pgvpos >>8) & 0xf);
		ret = reg_write(client, 0x3091, priv->test_pattern_pghpstep & 0xff);
		ret = reg_write(client, 0x3092, priv->test_pattern_pgvpstep & 0xff);
		ret = reg_write(client, 0x3093, priv->test_pattern_pghpnum & 0xff);
		ret = reg_write(client, 0x3094, priv->test_pattern_pgvpnum & 0xff);
		ret = reg_write(client, 0x3095, priv->test_pattern_pghprm & 0xf);
		ret = reg_write(client, 0x3096, priv->test_pattern_pgdata1 & 0xff);
		ret = reg_write(client, 0x3097, (priv->test_pattern_pgdata1 >>8) & 0xf);
		ret = reg_write(client, 0x3098, priv->test_pattern_pgdata2 & 0xff);
		ret = reg_write(client, 0x3099, (priv->test_pattern_pgdata2 >>8) & 0xf);

		ret |= reg_write(client, 0x303c, priv->crop_rect.left >> 8);
		ret |= reg_write(client, 0x303d, priv->crop_rect.left & 0xff);
		ret |= reg_write(client, 0x3038, priv->crop_rect.top >> 8);
		ret |= reg_write(client, 0x3039, priv->crop_rect.top & 0xff);

		ret |= reg_write(client, 0x303e, priv->crop_rect.width >> 8);
		ret |= reg_write(client, 0x303f, priv->crop_rect.width & 0xff);
		ret |= reg_write(client, 0x303a, priv->crop_rect.height >> 8);
		ret |= reg_write(client, 0x303b, priv->crop_rect.height & 0xff);
	} else {
		ret = reg_write(client, 0x300a, 0xf0);
		ret = reg_write(client, 0x300b, 0x00);
		ret = reg_write(client, 0x300e, 0x01);
		ret = reg_write(client, 0x3089, 0xff);
		ret = reg_write(client, 0x308c, 0x00);
	}

	priv->cur_vts = priv->cur_mode->vts_def - IMX385_EXP_LINES_MARGIN;
	if (ret)
	{
		dev_err(&client->dev,"%s:ret(%d)\n",__func__,ret);
		return ret;
	}
//	dev_info(&client->dev,"%s:end\n",__func__);
	return reg_write_table(client, start);
}
/**
 * @brief 
 * 
 */
/* V4L2 subdev core operations */
static int imx385_s_power(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx385 *priv = to_imx385(client);

	if (on)	{
		dev_dbg(&client->dev, "imx385 power on\n");
		//gpiod_set_value_cansleep(priv->imx385_powerdown, 0);
		gpiod_set_value_cansleep(priv->imx385_reset, 1);
	} else if (!on) {
		dev_dbg(&client->dev, "imx385 power off\n");
		//gpiod_set_value_cansleep(priv->imx385_powerdown, 1);
		gpiod_set_value_cansleep(priv->imx385_reset, 0);
	}

	return 0;
}

/* V4L2 ctrl operations */
static int imx385_s_ctrl_test_pattern(struct v4l2_ctrl *ctrl)
{
	struct imx385 *priv =
	    container_of(ctrl->handler, struct imx385, ctrl_handler);

	switch (ctrl->val) {
	case TEST_PATTERN_DISABLED:
		priv->test_pattern = 0x0000;
		break;
	case TEST_PATTERN_MULTI_PIXELS:
		priv->test_pattern = 0x0001;
		priv->test_pattern_pg_mode = 0;
		priv->test_pattern_pgdata1 = 0xf;
		priv->test_pattern_pgdata2 = 0xf;
		priv->test_pattern_pghpos = 0;
		priv->test_pattern_pgvpos = 0;
		priv->test_pattern_pghpstep = 1;
		priv->test_pattern_pgvpstep = 1;
		priv->test_pattern_pghpnum = 1;
		priv->test_pattern_pgvpnum = 1;
		break;
	case TEST_PATTERN_SEQUENCE1:
		priv->test_pattern = 0x0001;
		priv->test_pattern_pg_mode = 1;
		break;
	case TEST_PATTERN_HORIZONTAL_COLOR_BAR:
		priv->test_pattern = 0x0001;
		priv->test_pattern_pg_mode = 2;
		priv->test_pattern_colorwidth = 0;
		break;
	case TEST_PATTERN_VERTICAL_COLOR_BAR:
		priv->test_pattern = 0x0001;
		priv->test_pattern_pg_mode = 3;
		priv->test_pattern_colorwidth = 0;
		break;
	case TEST_PATTERN_SEQUENCE2:
		priv->test_pattern = 0x0001;
		priv->test_pattern_pg_mode = 4;
		break;
	case TEST_PATTERN_GRADATION_PATTERN1:
		priv->test_pattern = 0x0001;
		priv->test_pattern_pg_mode = 5;
		break;
	case TEST_PATTERN_GRADATION_PATTERN2:
		priv->test_pattern = 0x0001;
		priv->test_pattern_pg_mode = 6;
		break;
	case TEST_PATTERN_000_555_TOGGLE_PATTERN:
		priv->test_pattern = 0x0001;
		priv->test_pattern_pg_mode = 7;
		break;
	case TEST_PATTERN_4PIXEL_PATTERN:
		priv->test_pattern = 0x0001;
		priv->test_pattern_pg_mode = 8;
		priv->test_pattern_pgdata1 = 0xf;
		priv->test_pattern_pgdata2 = 0xf;
		priv->test_pattern_pghpos = 0;
		priv->test_pattern_pgvpos = 0;
		priv->test_pattern_pgthru = 1;
		break;
	case TEST_PATTERN_HORIZONTAL1_ROW:
		priv->test_pattern = 0x0001;
		priv->test_pattern_pg_mode = 9;
		priv->test_pattern_pgdata1 = 0xf;
		priv->test_pattern_pgdata2 = 0xf;
		priv->test_pattern_pgvpos = 0;
		priv->test_pattern_pgthru = 1;
		break;
	case TEST_PATTERN_VERTICAL1_COLUMN:
		priv->test_pattern = 0x0001;
		priv->test_pattern_pg_mode = 0xa;
		priv->test_pattern_pgdata1 = 0xf;
		priv->test_pattern_pgdata2 = 0xf;
		priv->test_pattern_pghpos = 0;
		priv->test_pattern_pgthru = 1;
		break;
	case TEST_PATTERN_1ROW_AND_1_COLUMN:
		priv->test_pattern = 0x0001;
		priv->test_pattern_pg_mode = 0xb;
		priv->test_pattern_pgdata1 = 0xf;
		priv->test_pattern_pgdata2 = 0xf;
		priv->test_pattern_pghpos = 0;
		priv->test_pattern_pgthru = 1;
		break;
	case TEST_PATTERN_STRIPE_PATTERN:
		priv->test_pattern = 0x0001;
		priv->test_pattern_pg_mode = 0xc;
		priv->test_pattern_pgdata1 = 0xf;
		priv->test_pattern_pgdata2 = 0xf;
		break;
	case TEST_PATTERN_CHECKER_PATTERN:
		priv->test_pattern = 0x0001;
		priv->test_pattern_pg_mode = 0xd;
		priv->test_pattern_pgdata1 = 0xf;
		priv->test_pattern_pgdata2 = 0xf;
		break;
	case TEST_PATTERN_1_PIXEL_PATTERN:
		priv->test_pattern = 0x0001;
		priv->test_pattern_pg_mode = 0xe;
		priv->test_pattern_pgdata1 = 0xf;
		priv->test_pattern_pgdata2 = 0xf;
		priv->test_pattern_pghpos = 0;
		priv->test_pattern_pgvpos = 0;
		priv->test_pattern_pgthru = 1;
		break;
	case TEST_PATTERN_HORIZONTAL_2PIXEL_PATTERN:
		priv->test_pattern = 0x0001;
		priv->test_pattern_pg_mode = 0xf;
		priv->test_pattern_pgdata1 = 0xf;
		priv->test_pattern_pgdata2 = 0xf;
		priv->test_pattern_pghpos = 0;
		priv->test_pattern_pgvpos = 0;
		priv->test_pattern_pghprm = 1;
		priv->test_pattern_pgthru = 1;
		break;	
	default:
		return -EINVAL;
	}

	return 0;
}
/**
 * @brief 
 * 
 * @param sd 
 * @param fi 
 * @return int 
 */
static int imx385_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx385 *priv = to_imx385(client);
	const struct imx385_mode *mode = priv->cur_mode;

	fi->interval = mode->max_fps;

	return 0;
}
/*
 * imx385_reset - Function called to reset the sensor
 * @priv: Pointer to device structure
 * @rst: Input value for determining the sensor's end state after reset
 *
 * Set the senor in reset and then
 * if rst = 0, keep it in reset;
 * if rst = 1, bring it out of reset.
 *
 */
static void imx385_reset(struct imx385 *priv, int rst)
{
	gpiod_set_value_cansleep(priv->imx385_reset, 0);
	usleep_range(IMX385_RESET_DELAY1, IMX385_RESET_DELAY2);
	gpiod_set_value_cansleep(priv->imx385_reset, !!rst);
	usleep_range(IMX385_RESET_DELAY1, IMX385_RESET_DELAY2);
}
/**
 * @brief 
 * 
 * @param ctrl 
 * @return int 
 */
static int imx385_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct imx385 *priv =
	    container_of(ctrl->handler, struct imx385, ctrl_handler);
	struct i2c_client *client = v4l2_get_subdevdata(&priv->subdev);
	u8 reg;
	int ret;

	dev_dbg(&client->dev,"%s:ctrl->id(0x%x),ctrl->val(%d)\n",__func__,ctrl->id,ctrl->val);

	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		priv->hflip = ctrl->val;
		break;

	case V4L2_CID_VFLIP:
		priv->vflip = ctrl->val;
		break;

	case V4L2_CID_ANALOGUE_GAIN:
	case V4L2_CID_GAIN:
		priv->digital_gain = ctrl->val*5;
		ret |= reg_write(client, 0x3015, (priv->digital_gain&0x3000) >> 12);
		ret |= reg_write(client, 0x3014, (priv->digital_gain/16) & 0xff);

		return ret;

	case V4L2_CID_EXPOSURE:
		priv->exposure_time = 1125 - ctrl->val -1;

		ret = reg_write(client, 0x3022, (priv->exposure_time >> 16)&0x01);
		ret = reg_write(client, 0x3021, priv->exposure_time >> 8);
		ret |= reg_write(client, 0x3020, priv->exposure_time & 0xff);
		return ret;

	case V4L2_CID_TEST_PATTERN:
		return imx385_s_ctrl_test_pattern(ctrl);

	case V4L2_CID_VBLANK:
		if (ctrl->val < priv->cur_mode->vts_def)
			ctrl->val = priv->cur_mode->vts_def;
		if ((ctrl->val - IMX385_EXP_LINES_MARGIN) != priv->cur_vts)
			priv->cur_vts = ctrl->val - IMX385_EXP_LINES_MARGIN;
		ret = reg_write(client, 0x3019, ((priv->cur_vts >> 8) & 0xff));
		ret |= reg_write(client, 0x3018, (priv->cur_vts & 0xff));
		return ret;

	default:
		return -EINVAL;
	}
	/* If enabled, apply settings immediately */
	reg = reg_read(client, 0x3000);
	if ((reg & 0x01) != 0x01)
		imx385_s_stream(&priv->subdev, 1);

	return 0;
}
/**
 * @brief 
 * 
 * @param sd 
 * @param cfg 
 * @param code 
 * @return int 
 */
static int imx385_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index != 0)
		return -EINVAL;
	code->code = MEDIA_BUS_FMT_SRGGB12_1X12;

	return 0;
}
/**
 * @brief 
 * 
 * @param mode 
 * @param framefmt 
 * @return int 
 */
static int imx385_get_reso_dist(const struct imx385_mode *mode,
				struct v4l2_mbus_framefmt *framefmt)
{
	return abs(mode->width - framefmt->width) +
	       abs(mode->height - framefmt->height);
}
/**
 * @brief 
 * 
 * @param fmt 
 * @return const struct imx385_mode* 
 */
static const struct imx385_mode *imx385_find_best_fit(
					struct v4l2_subdev_format *fmt)
{
	struct v4l2_mbus_framefmt *framefmt = &fmt->format;
	int dist;
	int cur_best_fit = 0;
	int cur_best_fit_dist = -1;
	int i;

	for (i = 0; i < ARRAY_SIZE(supported_modes); i++) {
		dist = imx385_get_reso_dist(&supported_modes[i], framefmt);
		if (cur_best_fit_dist == -1 || dist < cur_best_fit_dist) {
			cur_best_fit_dist = dist;
			cur_best_fit = i;
		}
	}

	return &supported_modes[cur_best_fit];
}
/**
 * @brief 
 * 
 * @param sd 
 * @param cfg 
 * @param fmt 
 * @return int 
 */
static int imx385_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx385 *priv = to_imx385(client);
	const struct imx385_mode *mode;
	s64 h_blank, v_blank, pixel_rate;
	u32 fps = 0;
	
	dev_info(&client->dev,"%s:start\n",__func__);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	mode = imx385_find_best_fit(fmt);
	fmt->format.code = MEDIA_BUS_FMT_SRGGB12_1X12;
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.field = V4L2_FIELD_NONE;
	priv->cur_mode = mode;
	h_blank = mode->hts_def - mode->width;
	__v4l2_ctrl_modify_range(priv->hblank, h_blank,
					h_blank, 1, h_blank);
	v_blank = mode->vts_def - mode->height;
	__v4l2_ctrl_modify_range(priv->vblank, v_blank,
					v_blank,
					1, v_blank);
	fps = DIV_ROUND_CLOSEST(mode->max_fps.denominator,
		mode->max_fps.numerator);
	pixel_rate = mode->vts_def * mode->hts_def * fps;
	__v4l2_ctrl_modify_range(priv->pixel_rate, pixel_rate,
					pixel_rate, 1, pixel_rate);

	/* reset crop window */
	priv->crop_rect.left = 1920/2 - (mode->width / 2); 
	if (priv->crop_rect.left < 0)
		priv->crop_rect.left = 0;
	priv->crop_rect.top = 1080/2 - (mode->height / 2); 
	if (priv->crop_rect.top < 0)
		priv->crop_rect.top = 0;
	priv->crop_rect.width = mode->width;
	priv->crop_rect.height = mode->height;

	return 0;
}
/**
 * @brief 
 * 
 * @param sd 
 * @param cfg 
 * @param fmt 
 * @return int 
 */
static int imx385_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx385 *priv = to_imx385(client);
	const struct imx385_mode *mode = priv->cur_mode;

	dev_info(&client->dev,"%s:start\n",__func__);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.code = MEDIA_BUS_FMT_SRGGB12_1X12;
	fmt->format.field = V4L2_FIELD_NONE;

	dev_info(&client->dev,"%s:mode->width(%d),mode->height(%d)end\n",__func__,mode->width,mode->height);
	return 0;
}
/**
 * @brief 
 * 
 * @param sd 
 * @param cmd 
 * @param arg 
 * @return long 
 */
static long imx385_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx385 *imx385 = to_imx385(client);
	long ret = 0;

	switch (cmd) {
	//case CANAANMODULE_GET_MODULE_INFO:
	//	imx385_get_module_inf(imx385, (struct CANAANMODULE_inf *)arg);
	//	break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}
/**
 * @brief 
 * 
 */
#ifdef CONFIG_COMPAT
static long imx385_compat_ioctl32(struct v4l2_subdev *sd,
				  unsigned int cmd, unsigned long arg)
{
	void __user *up = compat_ptr(arg);
	struct module_inf *inf;
	struct module_awb_cfg *cfg;
	long ret;

	switch (cmd) {
	case CANAANMODULE_GET_MODULE_INFO:
		inf = kzalloc(sizeof(*inf), GFP_KERNEL);
		if (!inf) {
			ret = -ENOMEM;
			return ret;
		}

		ret = imx385_ioctl(sd, cmd, inf);
		if (!ret)
			ret = copy_to_user(up, inf, sizeof(*inf));
		kfree(inf);
		break;
	case CANAANMODULE_AWB_CFG:
		cfg = kzalloc(sizeof(*cfg), GFP_KERNEL);
		if (!cfg) {
			ret = -ENOMEM;
			return ret;
		}

		ret = copy_from_user(cfg, up, sizeof(*cfg));
		if (!ret)
			ret = imx385_ioctl(sd, cmd, cfg);
		kfree(cfg);
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}
#endif
/**
 * @brief 
 * 
 * @param sd 
 * @param cfg 
 * @param fie 
 * @return int 
 */
static int imx385_enum_frame_interval(struct v4l2_subdev *sd,
				       struct v4l2_subdev_pad_config *cfg,
				       struct v4l2_subdev_frame_interval_enum *fie)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx385 *priv = to_imx385(client);

	if (fie->index >= priv->cfg_num)
		return -EINVAL;

	if (fie->code != MEDIA_BUS_FMT_SRGGB12_1X12)
		return -EINVAL;

	fie->width = supported_modes[fie->index].width;
	fie->height = supported_modes[fie->index].height;
	fie->interval = supported_modes[fie->index].max_fps;
	return 0;
}
/**
 * @brief 
 * 
 * @param sd 
 * @param config 
 * @return int 
 */
static int IMX385_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *config)
{
	u32 val = 0;

	val = 1 << (IMX385_LANES - 1) |
	      V4L2_MBUS_CSI2_CHANNEL_0 |
	      V4L2_MBUS_CSI2_CONTINUOUS_CLOCK;
	config->type = V4L2_MBUS_CSI2;
	config->flags = val;

	return 0;
}

static int imx385_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *config)
{
	u32 val = 0;

	val = 1 << (IMX385_LANES - 1) |
	      V4L2_MBUS_CSI2_CHANNEL_0 |
	      V4L2_MBUS_CSI2_CONTINUOUS_CLOCK;
	config->type = V4L2_MBUS_CSI2;
	config->flags = val;

	return 0;
}

/* Various V4L2 operations tables */
static struct v4l2_subdev_video_ops imx385_subdev_video_ops = {
	.s_stream = imx385_s_stream,
	.g_frame_interval = imx385_g_frame_interval,
	.g_mbus_config = imx385_g_mbus_config,
};

static struct v4l2_subdev_core_ops imx385_subdev_core_ops = {
	.s_power = imx385_s_power,
	.ioctl = imx385_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = imx385_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_pad_ops imx385_subdev_pad_ops = {
	.enum_mbus_code = imx385_enum_mbus_code,
	.enum_frame_interval = imx385_enum_frame_interval,
	.set_fmt = imx385_set_fmt,
	.get_fmt = imx385_get_fmt,
};

static struct v4l2_subdev_ops imx385_subdev_ops = {
	.core = &imx385_subdev_core_ops,
	.video = &imx385_subdev_video_ops,
	.pad = &imx385_subdev_pad_ops,
};

static const struct v4l2_ctrl_ops imx385_ctrl_ops = {
	.s_ctrl = imx385_s_ctrl,
};
/**
 * @brief 
 * 
 * @param client 
 * @return int 
 */
static int imx385_video_probe(struct i2c_client *client)
{
	struct v4l2_subdev *subdev = i2c_get_clientdata(client);
	u16 model_id;
	u32 lot_id;
	u16 chip_id;
	u16 data_format;
	int ret;

	ret = imx385_s_power(subdev, 1);
	if (ret < 0)
		return ret;

	/* Check and show model, lot, and chip ID. */
	ret = reg_read(client, 0x339a);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Model ID (high byte)\n");
		goto done;
	}
	model_id = ret << 8;
	//?
	ret = reg_read(client, 0x3399);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Model ID (low byte)\n");
		goto done;
	}
	model_id |= ret;

	ret = reg_read(client, 0x339C);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Lot ID (mid byte)\n");
		goto done;
	}
	lot_id |= ret << 8;

	ret = reg_read(client, 0x339B);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Lot ID (low byte)\n");
		goto done;
	}
	lot_id |= ret;
	//?
	ret = reg_read(client, 0x339E);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Chip ID (high byte)\n");
		goto done;
	}
	chip_id = ret;

	ret = reg_read(client, 0x33A2);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Data format (high byte)\n");
		goto done;
	}
	data_format = ret<<8;

	ret = reg_read(client, 0x33A1);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Data format (low byte)\n");
		goto done;
	}
	data_format |= ret;

	if (model_id != 0x0385) {
		dev_err(&client->dev, "Model ID: %x not supported!\n",
			model_id);
		ret = -ENODEV;
		goto done;
	}
	dev_info(&client->dev,
		 "Model ID 0x%04x, Lot ID 0x%06x, Chip ID 0x%04x,data_format 0x%x\n",
		 model_id, lot_id, chip_id,data_format);
done:
	imx385_s_power(subdev, 0);
	return ret;
}
/**
 * @brief 
 * 
 * @param sd 
 * @return int 
 */
static int imx385_ctrls_init(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx385 *priv = to_imx385(client);
	const struct imx385_mode *mode = priv->cur_mode;
	s64 pixel_rate, h_blank, v_blank;
	int ret;
	u32 fps = 0;

	v4l2_ctrl_handler_init(&priv->ctrl_handler, 10);
	v4l2_ctrl_new_std(&priv->ctrl_handler, &imx385_ctrl_ops,
			  V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&priv->ctrl_handler, &imx385_ctrl_ops,
			  V4L2_CID_VFLIP, 0, 1, 1, 0);

	/* exposure */
	v4l2_ctrl_new_std(&priv->ctrl_handler, &imx385_ctrl_ops,
			  V4L2_CID_ANALOGUE_GAIN,
			  IMX385_ANALOGUE_GAIN_MIN,
			  IMX385_ANALOGUE_GAIN_MAX,
			  1, IMX385_ANALOGUE_GAIN_DEFAULT);
	v4l2_ctrl_new_std(&priv->ctrl_handler, &imx385_ctrl_ops,
			  V4L2_CID_GAIN,
			  IMX385_DIGITAL_GAIN_MIN,
			  IMX385_DIGITAL_GAIN_MAX, 1,
			  IMX385_DIGITAL_GAIN_DEFAULT);
	v4l2_ctrl_new_std(&priv->ctrl_handler, &imx385_ctrl_ops,
			  V4L2_CID_EXPOSURE,
			  IMX385_DIGITAL_EXPOSURE_MIN,
			  IMX385_DIGITAL_EXPOSURE_MAX, 1,
			  IMX385_DIGITAL_EXPOSURE_DEFAULT);

	/* blank */
	h_blank = mode->hts_def - mode->width;
	priv->hblank = v4l2_ctrl_new_std(&priv->ctrl_handler, NULL, V4L2_CID_HBLANK,
			  h_blank, h_blank, 1, h_blank);
	v_blank = mode->vts_def - mode->height;
	priv->vblank = v4l2_ctrl_new_std(&priv->ctrl_handler, NULL, V4L2_CID_VBLANK,
			  v_blank, v_blank, 1, v_blank);

	/* freq */
	v4l2_ctrl_new_int_menu(&priv->ctrl_handler, NULL, V4L2_CID_LINK_FREQ,
			       0, 0, link_freq_menu_items);
	fps = DIV_ROUND_CLOSEST(mode->max_fps.denominator,
		mode->max_fps.numerator);
	pixel_rate = mode->vts_def * mode->hts_def * fps;
	priv->pixel_rate = v4l2_ctrl_new_std(&priv->ctrl_handler, NULL, V4L2_CID_PIXEL_RATE,
			  0, pixel_rate, 1, pixel_rate);

	v4l2_ctrl_new_std_menu_items(&priv->ctrl_handler, &imx385_ctrl_ops,
				     V4L2_CID_TEST_PATTERN,
				     ARRAY_SIZE(tp_qmenu) - 1, 0, 0, tp_qmenu);

	priv->subdev.ctrl_handler = &priv->ctrl_handler;
	if (priv->ctrl_handler.error) {
		dev_err(&client->dev, "Error %d adding controls\n",
			priv->ctrl_handler.error);
		ret = priv->ctrl_handler.error;
		goto error;
	}

	ret = v4l2_ctrl_handler_setup(&priv->ctrl_handler);
	if (ret < 0) {
		dev_err(&client->dev, "Error %d setting default controls\n",
			ret);
		goto error;
	}

	return 0;
error:
	v4l2_ctrl_handler_free(&priv->ctrl_handler);
	return ret;
}
/**
 * @brief 
 * 
 * @param client 
 * @param did 
 * @return int 
 */
static int imx385_probe(struct i2c_client *client,
			const struct i2c_device_id *did)
{
	struct imx385 *priv;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct device *dev = &client->dev;
	struct device_node *node = dev->of_node;
	struct v4l2_subdev *sd;
	char facing[2];
	int ret;

	dev_info(dev, "driver version: %02x.%02x.%02x",
		DRIVER_VERSION >> 16,
		(DRIVER_VERSION & 0xff00) >> 8,
		DRIVER_VERSION & 0x00ff);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_warn(&adapter->dev,
			 "I2C-Adapter doesn't support I2C_FUNC_SMBUS_BYTE\n");
		return -EIO;
	}
	priv = devm_kzalloc(&client->dev, sizeof(struct imx385), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	ret = of_property_read_u32(node, CANAANMODULE_CAMERA_MODULE_INDEX,
				   &priv->module_index);
	ret |= of_property_read_string(node, CANAANMODULE_CAMERA_MODULE_FACING,
				       &priv->module_facing);
	ret |= of_property_read_string(node, CANAANMODULE_CAMERA_MODULE_NAME,
				       &priv->module_name);
	ret |= of_property_read_string(node, CANAANMODULE_CAMERA_LENS_NAME,
				       &priv->len_name);
	if (ret) {
		dev_err(dev, "could not get module information!\n");
		return -EINVAL;
	}

    //priv->imx385_powerdown = devm_gpiod_get(dev, "powerdown-gpios", GPIOD_OUT_HIGH);
    //if(IS_ERR(priv->imx385_powerdown))
    //{
	//	dev_err(dev, "get imx385_powerdown err !\n");
    //    return -EINVAL;
    //}

    priv->imx385_reset = devm_gpiod_get_optional(dev,"reset-gpios",GPIOD_OUT_HIGH);
    if(IS_ERR(priv->imx385_reset))
    {
		ret = PTR_ERR(priv->imx385_reset);
		dev_err(dev, "%s:Reset imx385 GPIO not setup in DT ret(%d)\n",__func__,ret);
        return ret;
    }

	/* pull sensor out of reset */
	imx385_reset(priv, 1);

	/* 1920 * 1080 by default */
	priv->cur_mode = &supported_modes[0];
	priv->cfg_num = ARRAY_SIZE(supported_modes);

	priv->crop_rect.left = 0x0; //0x2A8
	priv->crop_rect.top = 0x0; //0x2b4;
	priv->crop_rect.width = priv->cur_mode->width;
	priv->crop_rect.height = priv->cur_mode->height;

	v4l2_i2c_subdev_init(&priv->subdev, client, &imx385_subdev_ops);
	ret = imx385_ctrls_init(&priv->subdev);
	if (ret < 0)
		return ret;
	ret = imx385_video_probe(client);
	if (ret < 0)
		return ret;

	priv->subdev.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
		     V4L2_SUBDEV_FL_HAS_EVENTS;

	priv->pad.flags = MEDIA_PAD_FL_SOURCE;
	priv->subdev.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&priv->subdev.entity, 1, &priv->pad);
	if (ret < 0)
		return ret;

	sd = &priv->subdev;
	memset(facing, 0, sizeof(facing));
	if (strcmp(priv->module_facing, "back") == 0)
		facing[0] = 'b';
	else
		facing[0] = 'f';

	snprintf(sd->name, sizeof(sd->name), "m%02d_%s_%s %s",
		 priv->module_index, facing,
		 IMX385_NAME, dev_name(sd->dev));
	ret = v4l2_async_register_subdev_sensor_common(sd);
	if (ret < 0)
		return ret;

	return ret;
}
/**
 * @brief 
 * 
 * @param client 
 * @return int 
 */
static int imx385_remove(struct i2c_client *client)
{
	struct imx385 *priv = to_imx385(client);

	v4l2_async_unregister_subdev(&priv->subdev);
	media_entity_cleanup(&priv->subdev.entity);
	v4l2_ctrl_handler_free(&priv->ctrl_handler);
	return 0;
}

static const struct i2c_device_id imx385_id[] = {
	{"imx385", 0},
	{}
};

static const struct of_device_id imx385_of_match[] = {
	{ .compatible = "sony,imx385" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, imx385_of_match);

MODULE_DEVICE_TABLE(i2c, imx385_id);
static struct i2c_driver imx385_i2c_driver = {
	.driver = {
		.of_match_table = of_match_ptr(imx385_of_match),
		.name = IMX385_NAME,
	},
	.probe = imx385_probe,
	.remove = imx385_remove,
	.id_table = imx385_id,
};

module_i2c_driver(imx385_i2c_driver);
MODULE_DESCRIPTION("Sony imx385 Camera driver");
MODULE_LICENSE("GPL v2");
