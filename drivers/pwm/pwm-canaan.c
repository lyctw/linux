/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/time.h>

#define PWM_SCALE_MASK (0x0000000F)


#define REG_PWMCFG		0x00
#define REG_PWMCOUNT	0x08
#define REG_PWMS		0x10
#define REG_PWMCMP0		0x20
#define REG_PWMCMP(_channel)	(REG_PWMCMP0 + ((_channel) * 0x4))

#define PWM_NUMCHANNELS		4

#define PWMSTICKY			8
#define PWMZEROCMP			9
#define PWMDEGLITCH			10
#define PWMENALWAYS			12
#define PWMENONESHOT			13
#define PWMCMPCENTER(_channel)	(16 + (_channel))
#define PWMCMPGANG(_channel)		(24 + (_channel))
#define PWMCMPIP(_channel)		(28 + (_channel))

struct canaan_pwm_chip {
	struct	device	*dev;
	struct pwm_chip chip;
	struct clk *clk;
	void __iomem *base;
};

static inline struct canaan_pwm_chip *to_canaan_pwm_chip(struct pwm_chip *c)
{
	return container_of(c, struct canaan_pwm_chip, chip);
}

static void canaan_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			       struct pwm_state *state)
{
	struct canaan_pwm_chip *pc = to_canaan_pwm_chip(chip);
	u64 clk_rate;
    u64 period_cycles, period_cycles_max, duty_cycles;
    u32 pwmscale = 0;
    u32 pwmcfg;
    u32 pwmcmpx_max;

    clk_rate = clk_get_rate(pc->clk);

//配置周期和占空比
//1.计算周期的cycles数
	period_cycles = clk_rate * state->period / NSEC_PER_SEC;
    duty_cycles = clk_rate * state->duty_cycle / NSEC_PER_SEC;
    
//2.判断cycles数是否大于支持的最大值
    period_cycles_max = (1 << (15 + 16)) - 1LL;
    if(period_cycles > period_cycles_max)
    {
        return ; 
    }
//计算PWMCMPX可以设置的最大值
    pwmcmpx_max = (1<<16) - 1;
//找到最小的pwmscale值，保证精度最精确
    while ((period_cycles >> pwmscale) > pwmcmpx_max)
    {
		pwmscale++;
	}
//判断pwmscale是否满足4bit要求
    if(pwmscale > PWM_SCALE_MASK)
    {
        return;
    }
//判断占空比是否符合要求
    if(duty_cycles > period_cycles)
    {
        return;
    }
//设置pwmscale的值
    pwmcfg = readl_relaxed(pc->base + REG_PWMCFG);
    pwmcfg &= (~PWM_SCALE_MASK);
    pwmcfg |= pwmscale;
    writel(pwmcfg, pc->base + REG_PWMCFG);
    
//通过设置PWMCMP0来设置周期
    writel(period_cycles >> pwmscale, pc->base + REG_PWMCMP0);

//通过PWMCMPX来设置占空比
    writel(duty_cycles >> pwmscale, pc->base + REG_PWMCMP(pwm->hwpwm+1));
    
    state->period = NSEC_PER_SEC/clk_rate * ((period_cycles >> pwmscale)<<pwmscale);

	state->duty_cycle = NSEC_PER_SEC/clk_rate * ((duty_cycles >> pwmscale)<<pwmscale);
//打印配置后的值
	dev_info(pc->dev, "rate:%lld channel:%d pwmscale:%d pwmcmp0:%d pwmcmp%d:%d \n", clk_rate, pwm->hwpwm, pwmscale, state->period, pwm->hwpwm, state->duty_cycle);
}

static int canaan_pwm_enable(struct pwm_chip *chip,
			       struct pwm_device *pwm,
			       bool enable)
{
	struct canaan_pwm_chip *pc = to_canaan_pwm_chip(chip);
    int ret;
	u32 pwmcfg;

    if (enable) {
		ret = clk_enable(pc->clk);
		if (ret)
			return ret;
	}
    
	pwmcfg = readl_relaxed(pc->base + REG_PWMCFG);

	if (enable)
		pwmcfg |= (1<<PWMENALWAYS);
	else
		pwmcfg &= (~(1<<PWMENALWAYS));

	writel_relaxed(pwmcfg, pc->base + REG_PWMCFG);

    if (!enable)
		clk_disable(pc->clk);

	// u32 *addr = ioremap(0x970F0000U, 0x40);
	// printk("cfg: 0x%x\n", ioread32(addr));
	// printk("count: 0x%x\n", ioread32(addr+2));
	// printk("pwms: 0x%x\n", ioread32(addr+4));
	// printk("cmp0: 0x%x\n", ioread32(addr+8));
	// printk("cmp1: 0x%x\n", ioread32(addr+9));
	// printk("cmp2: 0x%x\n", ioread32(addr+10));
	// printk("cmp3: 0x%x\n", ioread32(addr+11));
	// iounmap(addr);
	// addr = ioremap(0x97040000, 0x1000);
	// printk("io17: 0x%x\n", ioread32(addr+17));
	// printk("io18: 0x%x\n", ioread32(addr+18));
	// printk("io19: 0x%x\n", ioread32(addr+19));
	// printk("io20: 0x%x\n", ioread32(addr+20));
	// iounmap(addr);
	return 0;
}

static int canaan_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
			      struct pwm_state *state)
{
	struct pwm_state current_state;
    bool enabled;
	int ret = 0;

	pwm_get_state(pwm, &current_state);
	enabled = current_state.enabled;

    if (!state->enabled) {
		if (current_state.enabled) {
			canaan_pwm_enable(chip, pwm, false);
		}
	}
    
	canaan_pwm_config(chip, pwm, state);
    
	if (state->enabled != current_state.enabled) {
		ret = canaan_pwm_enable(chip, pwm, state->enabled);
	}

	return ret;
}

static const struct pwm_ops canaan_pwm_ops = {
	.apply = canaan_pwm_apply,
	.owner = THIS_MODULE,
};

static const struct of_device_id canaan_pwm_dt_ids[] = {
	{ .compatible = "canaan,k510-pwm"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, canaan_pwm_dt_ids);

static int canaan_pwm_probe(struct platform_device *pdev)
{
	struct canaan_pwm_chip *pc;
	struct resource *res;
	int ret, ch;
    u32 pwmcfg = 0;

	pc = devm_kzalloc(&pdev->dev, sizeof(*pc), GFP_KERNEL);
	if (!pc)
		return -ENOMEM;

	pc->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pc->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pc->base))
		return PTR_ERR(pc->base);
    
    pc->clk = devm_clk_get(&pdev->dev, "pwm");
    if (IS_ERR(pc->clk)) {
		pc->clk = devm_clk_get(&pdev->dev, NULL);
		if (IS_ERR(pc->clk)) {
			ret = PTR_ERR(pc->clk);
			if (ret != -EPROBE_DEFER)
				dev_err(&pdev->dev, "Can't get bus clk: %d\n",
					ret);
			return ret;
		}
	}

	platform_set_drvdata(pdev, pc);

	pc->chip.dev = &pdev->dev;
	pc->chip.ops = &canaan_pwm_ops;
	pc->chip.base = -1;
	pc->chip.npwm = 3;

    
//1.计数器的值达到比较值后清零
    pwmcfg |= (1<<PWMZEROCMP);
//2.配置输出周期性脉冲信号
    pwmcfg |= (1<<PWMENALWAYS);

//3.清除所有的channels
    for (ch = 0; ch < PWM_NUMCHANNELS; ch++)
    {
        writel(0, pc->base + REG_PWMCMP(ch));
	}
    writel(pwmcfg, pc->base + REG_PWMCFG);
    
	ret = pwmchip_add(&pc->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "pwmchip_add() failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static int canaan_pwm_remove(struct platform_device *pdev)
{
	struct canaan_pwm_chip *pc = platform_get_drvdata(pdev);

	canaan_pwm_enable(&pc->chip, NULL, false);

	return pwmchip_remove(&pc->chip);
}

static struct platform_driver canaan_pwm_driver = {
	.driver = {
		.name = "canaan-pwm",
		.of_match_table = canaan_pwm_dt_ids,
	},
	.probe = canaan_pwm_probe,
	.remove = canaan_pwm_remove,
};
module_platform_driver(canaan_pwm_driver);

MODULE_AUTHOR("Jiangxb");
MODULE_DESCRIPTION("Canaan SoC PWM driver");
MODULE_LICENSE("GPL v2");