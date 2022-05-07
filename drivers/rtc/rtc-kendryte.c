/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/clk.h>
#include <linux/log2.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <asm/irq.h>

#define KENDRYTE_DATA_REG				0x00
#define KENDRYTE_TIME_REG				0x04
#define KENDRYTE_ALARM_DATA_REG			0x08
#define KENDRYTE_ALARM_TIME_REG			0x0c
#define	KENDRYTE_INITIAL_COUNT_REG		0x10
#define KENDRYTE_CURRENT_COUNT_REG 		0x14
#define KENDRYTE_ITERRUPT_CTRL_REG		0x18
#define KENDRYTE_REGISTER_CTRL_REG		0x1c
#define KENDRYTE_EXTENDED_REG			0x28


typedef enum _rtc_timer_mode_e
{
    RTC_TIMER_PAUSE,   						/*!< 0: Timer pause */
    RTC_TIMER_RUNNING, 						/*!< 1: Timer time running */
    RTC_TIMER_SETTING, 						/*!< 2: Timer time setting */
    RTC_TIMER_MAX      						/*!< Max count of this enum*/
} rtc_timer_mode_t;

typedef enum _rtc_tick_interrupt_mode_e
{
    RTC_INT_SECOND, 						/*!< 0: Interrupt every second */
    RTC_INT_MINUTE,							/*!< 1: Interrupt every minute */
    RTC_INT_HOUR,   						/*!< 2: Interrupt every hour */
    RTC_INT_DAY,    						/*!< 3: Interrupt every day */
    RTC_INT_MAX     						/*!< Max count of this enum*/
} rtc_tick_interrupt_mode_t;


struct kendryte_rtc {
	struct device *dev;
    struct rtc_device *rtc;	
	void __iomem *base;
	int irq_alarm;
	int irq_tick;	
//	spinlock_t pie_lock;
//    spinlock_t alarm_clk_lock;
};


static int kendryte_get_reg_val(int begin, int end, uint32_t reg)
{
	int i;
	uint32_t val = 0;
	int len = end - begin;
	
	if((len < 0) || end > 31)
		return -1;
		
	for(i = 0; i < len + 1; i++)    // 2 - 3 len = 1,but 2 3 is 2bit ,so add 1
	{
		val = val  | (1 << i);
	}
	val = (val << begin) & reg;
	
	return val >> begin;
}


static void kendryte_rtc_change_status(struct kendryte_rtc *rtc, bool status)
{
	uint32_t reg;
	uint32_t val = 0x2;
	
	reg = readl(rtc->base + KENDRYTE_REGISTER_CTRL_REG);
	
	if(status == 1)
	{		
		reg =  (~val) |  kendryte_get_reg_val(0, 1, reg);
		writel(reg, rtc->base + KENDRYTE_REGISTER_CTRL_REG);
	}
	else{
		reg = val & kendryte_get_reg_val(0, 1, reg);
		writel(reg, rtc->base + KENDRYTE_REGISTER_CTRL_REG);
	}
	udelay(100);
}


static void kendryte_rtc_set_mode(struct kendryte_rtc *rtc, int mode)
{
	uint32_t reg ,val;

	reg = readl(rtc->base + KENDRYTE_REGISTER_CTRL_REG);
	
	switch(mode)
    {
        case RTC_TIMER_PAUSE:
            val = (reg & (~0x03))  | 0x00;
            break;
        case RTC_TIMER_RUNNING:
            val = (reg & (~0x03))  | 0x01;
            break;
        case RTC_TIMER_SETTING:
            val = (reg & (~0x03))  | 0x02;
            break;
        default:
			val = reg;
			break;
    }

	writel(val, rtc->base + KENDRYTE_REGISTER_CTRL_REG);
	udelay(100);
}

/*set alarm interrupt enable/disable*/
static void kendryte_rtc_alarm_set_interrupt(struct kendryte_rtc *rtc, int enable)
{
	uint32_t reg = 0;

	kendryte_rtc_set_mode(rtc, RTC_TIMER_SETTING);	
	reg = readl(rtc->base + KENDRYTE_ITERRUPT_CTRL_REG);		
	reg = (reg & ~(BIT_MASK(1))) | (enable << 1);
	writel(reg, rtc->base + KENDRYTE_ITERRUPT_CTRL_REG);		
	kendryte_rtc_set_mode(rtc, RTC_TIMER_RUNNING);
	udelay(100);	
}

//set clock value
static void kendryte_rtc_set_clock_count_val(struct kendryte_rtc *rtc, uint32_t count)
{
//	uint32_t reg = 0;
	kendryte_rtc_set_mode(rtc ,RTC_TIMER_SETTING);
	
	writel(count, rtc->base + KENDRYTE_CURRENT_COUNT_REG);
	
	kendryte_rtc_set_mode(rtc ,RTC_TIMER_RUNNING);
}

/********clear alarm  irq****/
static void kendryte_rtc_clear_alarm_irq(struct kendryte_rtc *rtc)
{
	uint32_t reg = 0;
	kendryte_rtc_set_mode(rtc ,RTC_TIMER_SETTING);
	
	reg = readl(rtc->base + KENDRYTE_ITERRUPT_CTRL_REG);
	reg = (reg & ~(BIT_MASK(4))) | (1 << 4);
	writel(reg, rtc->base + KENDRYTE_ITERRUPT_CTRL_REG);
	
	kendryte_rtc_set_mode(rtc ,RTC_TIMER_RUNNING);
	
}

// set rtc frequency
static void kendryte_set_freq(struct kendryte_rtc *rtc, int freq)
{
	
	kendryte_rtc_set_mode(rtc, RTC_TIMER_SETTING);
			
	writel(freq, rtc->base + KENDRYTE_INITIAL_COUNT_REG);
	udelay(100);
	
	kendryte_rtc_set_mode(rtc, RTC_TIMER_RUNNING);
		
}


//rtc get time
static int kendryte_rtc_gettime(struct device *dev, struct rtc_time *rtc_tm)
{
	struct kendryte_rtc *rtc = dev_get_drvdata(dev);
	uint32_t reg;

	// get date
	reg = readl(rtc->base + KENDRYTE_DATA_REG);	
	rtc_tm->tm_mday = kendryte_get_reg_val(8, 12, reg);						// 8:12
	rtc_tm->tm_mon = kendryte_get_reg_val(16, 19, reg) - 1;					// 16:19 
	rtc_tm->tm_year = kendryte_get_reg_val(20, 31, reg) + 100;				// 20:31
	
	// get time
	reg = readl(rtc->base + KENDRYTE_TIME_REG);	
	rtc_tm->tm_min = kendryte_get_reg_val(16, 21, reg);
	rtc_tm->tm_hour = kendryte_get_reg_val(24, 28, reg);
	rtc_tm->tm_sec  = kendryte_get_reg_val(10, 15, reg);

	// printk("-------------read time %04d.%02d.%02d %02d:%02d:%02d\n",
    //             rtc_tm->tm_year + 1900, rtc_tm->tm_mon, rtc_tm->tm_mday,
    //             rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);

	return 0;	
}

//rtc set time
static int kendryte_rtc_settime(struct device *dev, struct rtc_time *rtc_tm)
{
	struct kendryte_rtc *rtc = dev_get_drvdata(dev);
	int year = rtc_tm->tm_year - 100;
	uint32_t reg = 0;
//	int rtc_century;
	
	if (year < 0 || year >= 100) {
		dev_err(dev, "rtc only supports 100 years\n");
		return -EINVAL;
    }
	
	// printk("--------set time %04d.%02d.%02d %02d:%02d:%02d\n",
    //             1900 + rtc_tm->tm_year, rtc_tm->tm_mon, rtc_tm->tm_mday,
    //             rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);

	kendryte_rtc_set_mode(rtc, RTC_TIMER_SETTING);
	
	reg = reg | (((rtc_tm->tm_year + 1900) % 100)  << 20);   
	reg = reg | (((rtc_tm->tm_mon + 1) % 13) << 16);	
	reg = reg | ((rtc_tm->tm_mday % 32) << 8);
	writel(reg, rtc->base + KENDRYTE_DATA_REG);
	
	reg = 0;
	reg = reg | ((rtc_tm->tm_hour % 24) << 24);
	reg = reg | ((rtc_tm->tm_min % 60) << 16);
	reg = reg | ((rtc_tm->tm_sec % 61) << 10);
	writel(reg, rtc->base + KENDRYTE_TIME_REG);
	
	kendryte_rtc_set_mode(rtc, RTC_TIMER_RUNNING);

	return 0;	
}

// get alarm
static int kendryte_rtc_getalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct kendryte_rtc *rtc = dev_get_drvdata(dev);
	struct rtc_time *alm_tm = &alrm->time;
	unsigned int alm_en;
	uint32_t reg = 0;
	
	reg = readl(rtc->base + KENDRYTE_ALARM_DATA_REG);
	alm_tm->tm_year = kendryte_get_reg_val(20, 31, reg);
	alm_tm->tm_mon  = kendryte_get_reg_val(16, 19, reg) - 1;
	alm_tm->tm_mday = kendryte_get_reg_val(8, 12, reg);
	
	reg = readl(rtc->base + KENDRYTE_ALARM_TIME_REG);
	alm_tm->tm_hour = kendryte_get_reg_val(24, 28, reg);
	alm_tm->tm_min  = kendryte_get_reg_val(16, 21 , reg);
	alm_tm->tm_sec  = kendryte_get_reg_val(10, 15 , reg);
	
	reg = readl(rtc->base + KENDRYTE_ITERRUPT_CTRL_REG);
	alm_en = reg & BIT_MASK(1);
	
	// printk("--------k510_rtc_getalarm %04d.%02d.%02d %02d:%02d:%02d\n",
    //             1900 + alm_tm->tm_year, alm_tm->tm_mon, alm_tm->tm_mday,
    //             alm_tm->tm_hour, alm_tm->tm_min, alm_tm->tm_sec);
				
	if(alm_en)
		alrm->enabled = 1;

	return 0;
}

//set alarm
static int kendryte_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct kendryte_rtc *rtc = dev_get_drvdata(dev);
	struct rtc_time *alm_tm = &alrm->time;
	uint32_t reg = 0;
	uint32_t val = 0x77;
	
	// printk("--------k510_rtc_setalarm %04d.%02d.%02d %02d:%02d:%02d\n",
    //             1900 + alm_tm->tm_year, alm_tm->tm_mon, alm_tm->tm_mday,
    //             alm_tm->tm_hour, alm_tm->tm_min, alm_tm->tm_sec);
				
	kendryte_rtc_alarm_set_interrupt(rtc, 0);
	
	kendryte_rtc_set_mode(rtc, RTC_TIMER_SETTING);
	
	reg = reg | (((alm_tm->tm_year + 1900) % 100)  << 20);   
	reg = reg | (((alm_tm->tm_mon + 1) % 13) << 16);			// 1 - 12
	reg = reg | ((alm_tm->tm_mday % 32) << 8);
	writel(reg, rtc->base + KENDRYTE_ALARM_DATA_REG);
	
	reg = 0;
	reg = reg | ((alm_tm->tm_hour % 24) << 24);
	reg = reg | ((alm_tm->tm_min % 60) << 16);
	reg = reg | ((alm_tm->tm_sec % 61) << 10);
	writel(reg, rtc->base + KENDRYTE_ALARM_TIME_REG);

	// set alarm interrupt mask
	reg = readl(rtc->base + KENDRYTE_ITERRUPT_CTRL_REG);
	reg = (reg & ~(val << 25)) | (val << 25);
	writel(reg, rtc->base + KENDRYTE_ITERRUPT_CTRL_REG);
	
	kendryte_rtc_set_mode(rtc, RTC_TIMER_SETTING);
	
	//open alarm interryp
	kendryte_rtc_alarm_set_interrupt(rtc, 1);	
			
	return 0;
}

//set pro show tick status
static int kendryte_rtc_proc(struct device *dev, struct seq_file *seq)
{
	uint32_t reg = 0;
	uint8_t alarm_en, tick_en, tick_mode;
	struct kendryte_rtc *rtc = dev_get_drvdata(dev);
	
	reg = readl(rtc->base + KENDRYTE_ITERRUPT_CTRL_REG);	
	alarm_en = BIT_MASK(1) & reg;
	tick_en = BIT_MASK(1) & reg;
	
	if(alarm_en == 1)
	{
		seq_printf(seq, "alarm_IRQ\t: %s\n","yes");
	}
	else
	{
		seq_printf(seq, "alarm_IRQ\t: %s\n","no");
	}
	
	if(tick_en == 1)
	{
		tick_mode = kendryte_get_reg_val(2, 3, reg);
		if(tick_mode == RTC_INT_SECOND){
			seq_printf(seq, "tick_IRQ is open \t: %s\n", "SECOND");
			
		}else if(tick_mode == RTC_INT_MINUTE){
			seq_printf(seq, "tick_IRQ is open \t: %s\n", "MINUTE");
			
		}else if(tick_mode == RTC_INT_HOUR){
			seq_printf(seq, "tick_IRQ is open \t: %s\n", "HOUR");			
		}else{
			seq_printf(seq, "tick_IRQ is open \t: %s\n", "DAY");
		}
	}
	else
	{
		seq_printf(seq, "tick_IRQ\t: %s\n","no");
	}
	
	return 0;	
}

/******rtc alarm enabled*****/
static int kendryte_rtc_setaie(struct device *dev, unsigned int enabled)
{
	struct kendryte_rtc *rtc = dev_get_drvdata(dev);
	
	printk( "k510_rtc set aie enabled is %d \n", enabled);
	
	if(enabled){
		kendryte_rtc_alarm_set_interrupt(rtc, 1);
	}else{
		kendryte_rtc_alarm_set_interrupt(rtc, 0);
	}
	
	return 0;
}

static const struct rtc_class_ops kendryte_rtcops = {
	.read_time      = kendryte_rtc_gettime,
	.set_time       = kendryte_rtc_settime,
	.read_alarm     = kendryte_rtc_getalarm,
	.set_alarm      = kendryte_rtc_setalarm,
	.proc           = kendryte_rtc_proc,
	.alarm_irq_enable = kendryte_rtc_setaie,	
};



// alarm hander irq
static irqreturn_t kendryte_rtc_alarmirq(int irq, void *id)
{
	struct kendryte_rtc *rtc = (struct kendryte_rtc *)id;

	rtc_update_irq(rtc->rtc, 1, RTC_AF | RTC_IRQF);
	
	printk(" this is alarmirq \n");

	kendryte_rtc_clear_alarm_irq(rtc);
	
	return IRQ_HANDLED;
}

/* IRQ Handlers */
static irqreturn_t kendryte_rtc_tickirq(int irq, void *id)
{
	struct kendryte_rtc *rtc = (struct kendryte_rtc *)rtc;

	printk("this is tickirq \n");
	
	rtc_update_irq(rtc->rtc, 1, RTC_AF | RTC_IRQF);

	return IRQ_HANDLED;
}

//Unable to handle kernel paging request at virtual address ffffffd004050070
static int kendryte_rtc_probe(struct platform_device *pdev)
{
	struct kendryte_rtc *rtc = NULL;
//	struct rtc_time rtc_tm;
	struct resource *res;
	int ret;
	printk("into probe rtc \n");
	rtc = devm_kzalloc(&pdev->dev, sizeof(*rtc), GFP_KERNEL);
	
	/* get irq_tick and  irq_alarm*/
	rtc->irq_tick = platform_get_irq(pdev, 1);
	if (rtc->irq_tick < 0) {
		dev_err(&pdev->dev, "no irq for rtc tick\n");
		return rtc->irq_tick;
	}
	rtc->irq_alarm = platform_get_irq(pdev, 0);
	if (rtc->irq_alarm < 0) {
			dev_err(&pdev->dev, "no irq for alarm\n");
			return rtc->irq_alarm;
	}	
	dev_dbg(&pdev->dev, "info->irq_alarm is %d info->irq_tick is %d \n", rtc->irq_alarm, rtc->irq_tick);
	printk("info->irq_alarm is %d info->irq_tick is %d \n", rtc->irq_alarm, rtc->irq_tick);

	platform_set_drvdata(pdev, rtc);

	/* get the memory region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	rtc->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(rtc->base))
		return PTR_ERR(rtc->base);
	
	device_init_wakeup(&pdev->dev, 1);
		
	rtc->rtc = devm_rtc_device_register(&pdev->dev, "kendryte", &kendryte_rtcops, THIS_MODULE);	
	if (IS_ERR(rtc->rtc)) {
		dev_err(&pdev->dev, "cannot attach rtc\n");
		ret = PTR_ERR(rtc->rtc);
		goto err_nortc;
	}

	ret = devm_request_irq(&pdev->dev, rtc->irq_alarm, kendryte_rtc_alarmirq,
								0, "kendryte-rtc alarm", rtc);
	if (ret) {
		dev_err(&pdev->dev, "IRQ%d error %d\n", rtc->irq_alarm, ret);
		goto err_nortc;
	}
	ret = devm_request_irq(&pdev->dev, rtc->irq_tick, kendryte_rtc_tickirq,
						   0, "kendryte-rtc tick", rtc);
	if (ret) {
		dev_err(&pdev->dev, "IRQ%d error %d\n", rtc->irq_tick, ret);
		goto err_nortc;
	}
	
	kendryte_rtc_change_status(rtc, 1);
	kendryte_rtc_set_clock_count_val(rtc, 1);
	kendryte_set_freq(rtc, 32768);		//62.5m	

	return 0;

err_nortc:
    kendryte_rtc_change_status(rtc, 0);	   
     
	return ret;	

}


static int kendryte_rtc_remove(struct platform_device *pdev)
{
	struct kendryte_rtc *rtc = platform_get_drvdata(pdev);
	
	kendryte_rtc_setaie(rtc->dev, 0);				//close rtc interrupt 
	kendryte_rtc_change_status(rtc, 0);				//close rtc
	kfree(rtc);	
	
	return 0;
}


static const struct of_device_id kendryte_rtc_dt_match[] = {
	{
		.compatible = "canaan,k510-rtc",
    },
	{},
};


static struct platform_driver kendryte_rtc_driver = {
	.probe          	= kendryte_rtc_probe,
	.remove         	= kendryte_rtc_remove,
	.driver         	= {
		.name   		= "kendryte-rtc",
		.of_match_table = of_match_ptr(kendryte_rtc_dt_match),
	},
};
module_platform_driver(kendryte_rtc_driver);

MODULE_DESCRIPTION("k510  RTC Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:k510-rtc");
