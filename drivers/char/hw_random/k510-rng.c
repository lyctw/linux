// SPDX-License-Identifier: GPL-2.0
/*
 * RNG driver for Canaan K510 TRNG
 *
 * Author: Guang Yang <yangguang@canaan-creative.com>
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/hw_random.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>

#define PUF_REG_ENABLE			0x0
#define PUF_REG_MODE			0x4
#define PUF_REG_MODE_EN			0x8
#define PUF_REG_USER_MODE_ONLY		0xc
#define PUF_REG_TEST_ROW		0x10
#define PUF_REG_MODE_STA		0x14
#define PUF_REG_IP_LP_STATE		0x18
#define PUF_REG_TRNG_OUTPUT		0x1c
#define PUF_REG_TRNG_OUTPUT_VALID	0x20

#define PUF_REG_ENABLE_ENABLE		0x00000001U
#define PUF_REG_ENABLE_DISABLE		0x00000000U

#define PUF_REG_MODE_EN_ENABLE		0x00000001U
#define PUF_REG_MODE_EN_DISABLE		0x00000000U

#define PUF_REG_PGM_BUSY		0x00000001U
#define PUF_REG_PGM_READY		0x00000000U

#define PUF_REG_TRNG_LP_ENABLE		0x00000001U
#define PUF_REG_TRNG_LP_DISABLE		0x00000000U

#define PUF_REG_MODE_DEEP_STANDBY	0x02
#define PUF_REG_MODE_TRNG_OUTPUT	0x10

#define TRNG_OUTPUT_VALID		0x1
#define TRNG_OUTPUT_INVALID		0x0

struct k510_rng {
	void __iomem *base;
	struct hwrng rng;
	struct device *dev;
};

static int wait_us_1000(volatile uint32_t* register_addr, int mode);

#define check_pgm_busy_error(p) wait_us_1000(p->base+PUF_REG_MODE_STA, PUF_REG_PGM_READY)

#define to_k510_rng(p)	container_of(p, struct k510_rng, rng)


static int wait_us_1000(volatile uint32_t* register_addr, int mode)
{
    int wait_time = 1000;
    while(*register_addr != mode)
    {
        udelay(1);
        wait_time--;
        if(wait_time < 0)
            return -1;
    }
    return 0;
}


static int k510_rng_init(struct hwrng *rng)
{
	struct k510_rng *k510rng = to_k510_rng(rng);
	int ret = 0;

	writel(PUF_REG_ENABLE_ENABLE, k510rng->base + PUF_REG_ENABLE);
	if(check_pgm_busy_error(k510rng)) {
		dev_err(k510rng->dev, "enable puf failure \n");
		return -EBUSY;
	}

	writel(PUF_REG_MODE_TRNG_OUTPUT, k510rng->base + PUF_REG_MODE);
	if(check_pgm_busy_error(k510rng)) {
		dev_err(k510rng->dev, "set puf mode to TRNG failure \n");
		return -EBUSY;
	}
	
	writel(PUF_REG_MODE_EN_ENABLE, k510rng->base + PUF_REG_MODE_EN);
	if(check_pgm_busy_error(k510rng)) {
		dev_err(k510rng->dev, "set puf mode enable failure \n");
		return -EBUSY;
	}
	
	ret = wait_us_1000(k510rng->base+PUF_REG_MODE_STA, PUF_REG_MODE_TRNG_OUTPUT);
	if(ret) {
		dev_err(k510rng->dev, "puf is not set to TRNG MODE \n");
		return -EIO;
	}

	ret = wait_us_1000(k510rng->base+PUF_REG_TRNG_OUTPUT_VALID, TRNG_OUTPUT_VALID);
	if(ret) {
		dev_err(k510rng->dev, "TRNG output invalid \n");
		return -EIO;
	}


	return 0;
}

static void k510_rng_cleanup(struct hwrng *rng)
{
	struct k510_rng *k510rng = to_k510_rng(rng);
	int ret = 0;

	writel(PUF_REG_MODE_DEEP_STANDBY, k510rng->base + PUF_REG_MODE);
	if(check_pgm_busy_error(k510rng)) {
		dev_err(k510rng->dev, "set puf mode to deep standby failure \n");
		return;
	}

	writel(PUF_REG_MODE_EN_ENABLE, k510rng->base + PUF_REG_MODE_EN);
	if(check_pgm_busy_error(k510rng)) {
		dev_err(k510rng->dev, "set puf mode enable failure \n");
		return;
	}

	ret = wait_us_1000(k510rng->base+PUF_REG_MODE_STA, PUF_REG_MODE_DEEP_STANDBY);
	if(ret) {
		dev_err(k510rng->dev, "puf is not set to deep standby \n");
		return;
	}

	writel(PUF_REG_ENABLE_DISABLE, k510rng->base + PUF_REG_ENABLE);
	if(check_pgm_busy_error(k510rng)) {
		dev_err(k510rng->dev, "disable puf failure \n");
		return;
	}

}

static int k510_rng_read(struct hwrng *rng, void *buf, size_t max, bool wait)
{
	struct k510_rng *k510rng = to_k510_rng(rng);
	u32 *data = buf;

	*data = readl_relaxed(k510rng->base + PUF_REG_TRNG_OUTPUT);
	return 4;
}

static int k510_rng_probe(struct platform_device *pdev)
{
	struct k510_rng *rng;
	struct resource *res;
	int ret;

	rng = devm_kzalloc(&pdev->dev, sizeof(*rng), GFP_KERNEL);
	if (!rng)
		return -ENOMEM;

	rng->dev = &pdev->dev;
	platform_set_drvdata(pdev, rng);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	rng->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(rng->base))
		return PTR_ERR(rng->base);

	rng->rng.name = pdev->name;
	rng->rng.init = k510_rng_init;
	rng->rng.cleanup = k510_rng_cleanup;
	rng->rng.read = k510_rng_read;

	ret = devm_hwrng_register(&pdev->dev, &rng->rng);
	if (ret) {
		dev_err(&pdev->dev, "failed to register hwrng\n");
		return ret;
	}

	pm_runtime_enable(&pdev->dev);
	pm_runtime_get(&pdev->dev);

	return 0;
}

static const struct of_device_id k510_rng_dt_ids[] = {
	{ .compatible = "canaan,k510-rng" },
	{ }
};
MODULE_DEVICE_TABLE(of, k510_rng_dt_ids);

static struct platform_driver k510_rng_driver = {
	.probe		= k510_rng_probe,
	.driver		= {
		.name	= "k510-rng",
		.of_match_table = of_match_ptr(k510_rng_dt_ids),
	},
};

module_platform_driver(k510_rng_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Guang Yang <yangguang@canaan-creative.com>");
MODULE_DESCRIPTION("Canaan K510 random number generator driver");
