// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2008-2017 Andes Technology Corporation

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/device.h>
#include <linux/printk.h>
#include <linux/suspend.h>

extern unsigned int *wake_mask;
extern void __iomem *plic_base;
#define PLIC_PEND_OFF	0x1000
#define MAX_DEVICES	1024
#define MAX_USE_REGS	MAX_DEVICES / 32

static void riscv_suspend_cpu(void)
{
	int i;
	unsigned int wake;
	u32 __iomem *reg = plic_base + PLIC_PEND_OFF;

	while (true) {
		__asm__ volatile ("wfi\n\t");
		for (i = 0; i < MAX_USE_REGS; i++) {
			if (wake_mask[i]) {
				wake = readl(reg + i);
				if (wake_mask[i] & wake) {
					goto wakeup;
				}
			}
		}
	}
wakeup:
	return;
}

extern void riscv_suspend2ram(void);

static int riscv_pm_enter(suspend_state_t state)
{
	pr_debug("%s:state:%d\n", __func__, state);
	switch (state) {
	case PM_SUSPEND_STANDBY:
		riscv_suspend_cpu();
		return 1;
	case PM_SUSPEND_MEM:
		riscv_suspend2ram();
		return 1;
	default:
		return -EINVAL;
	}
}

static int riscv_pm_valid(suspend_state_t state)
{
	switch (state) {
	case PM_SUSPEND_ON:
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		return 1;
	default:
		return -EINVAL;
	}
}

static const struct platform_suspend_ops riscv_pm_ops = {
	.valid = riscv_pm_valid,
	.enter = riscv_pm_enter,
};

static int __init riscv_pm_init(void)
{
	pr_debug("Enter %s\n", __func__);
	suspend_set_ops(&riscv_pm_ops);
	return 0;
}
late_initcall(riscv_pm_init);
