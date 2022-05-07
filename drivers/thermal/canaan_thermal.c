/* 
 * Canaan thermal sensor driver
 * 
 */

#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/cpu_cooling.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/types.h>
#include <linux/nvmem-consumer.h>

//#define COOLING_DEVICE

#define TS_CONFIG       0x00
#define TS_CLK_RATIO    0x08
#define TS_WR_AN        0x0c
#define TS_STATUS       0x10
#define TS0_DATA        0x14      
#define TS1_DATA        0x18
#define TS2_DATA        0x1c
#define TS3_DATA        0x20
#define TS4_DATA        0x24

#define TS_RUN_SHIFT        31
#define TS_RUN_MASK         (1 << TS_RUN_SHIFT)
#define TS_SIG_RE_SHIFT     23
#define TS_SIG_RE_MASK      (1 << TS_SIG_RE_SHIFT)


#define TS_POWERDOWN        0x1F
#define TS_POWERON          0x00

#define TSENESOR_PARAM      (4094)
#define TSENSOR_Y           (237500)
#define TSENSOR_X           (81100)

enum canaan_thermal_trip {
    CANAAN_TRIP_PASSIVE,
    CANAAN_TRIP_CRITICAL,
    CANAAN_TRIP_MAX,
};

#define CANAAN_POLLING_DELAY    2000 /* millisecond */
#define CANAAN_PASSIVE_DELAY    1000

struct canaan_thermal_data {
    struct cpufreq_policy *policy;
    struct thermal_zone_device *tz;
    struct thermal_cooling_device *cdev;
    enum thermal_device_mode mode;
    void __iomem *base;
    int temp_passive;
    int temp_critical;
    int temp_max;
    int alarm_temp;
    int last_temp;
    bool irq_enabled;
    int irq;
    struct clk *thermal_clk;
    struct mutex lock;
};

static int read_temp_single(struct canaan_thermal_data *data,
                    int *temp)
{
    u32 val = 0;
    u64 ts_data = 0;
    u8 bit;
    u8 i = 0;

    iowrite32(TS_POWERON, data->base + TS_CONFIG);
    val = TS_RUN_MASK | TS_SIG_RE_MASK;
    iowrite32(val, data->base + TS_WR_AN);
    msleep(6);
    val = ioread32(data->base + TS_STATUS);
    if(val == 0)
        return -EAGAIN;
    for(bit = 0; bit < 5; bit++)
    {
        if((val>>bit) & 0x1)
        {
            ts_data += ioread32(data->base + TS0_DATA + bit*0x4);
            i++;
        }
    }
    ts_data = ts_data/i;
    *temp = (ts_data * TSENSOR_Y / TSENESOR_PARAM - TSENSOR_X);
    // printk("[%s,%d], *temp:%d", __func__, __LINE__, *temp);
    // usleep_range(10000, 11000);
    // val &= (~TS_SIG_RE_MASK);
    // iowrite32(val, data->base + TS_WR_AN);
    // iowrite32(TS_POWERDOWN, data->base + TS_CONFIG);
    
    if(*temp != data->last_temp)
        data->last_temp = *temp;
    
    return 0;
}

#if 0
static void canaan_set_panic_temp(struct canaan_thermal_data *data, 
                    int temp)
{
    // printk("[%s,%d]", __FUNCTION__, __LINE__);
}


static void canaan_set_alarm_temp(struct canaan_thermal_data *data, 
                    int temp)
{
    // printk("[%s,%d]", __FUNCTION__, __LINE__);
}
#endif

static int canaan_get_temp(struct thermal_zone_device *tz, int *temp)
{
    struct canaan_thermal_data *data = tz->devdata;
    // void __iomem *base = data->base;
    // unsigned int n_meas;
    // bool wait;
    // u32 val;

    if(data->mode == THERMAL_DEVICE_ENABLED)
    {
        mutex_lock(&data->lock);
        read_temp_single(data, temp);
        mutex_unlock(&data->lock);
    }
    else
    {
        return -EAGAIN;
    }

    return 0;
}

static int canaan_get_mode(struct thermal_zone_device *tz, 
            enum thermal_device_mode *mode)
{
    struct canaan_thermal_data *data = tz->devdata;

    *mode = data->mode;

    return 0;
}

static int canaan_set_mode(struct thermal_zone_device *tz, 
            enum thermal_device_mode mode)
{
    struct canaan_thermal_data *data = tz->devdata;
    void __iomem *base = data->base;

    if(mode == THERMAL_DEVICE_ENABLED)
    {
        tz->polling_delay = CANAAN_POLLING_DELAY;
        tz->passive_delay = CANAAN_PASSIVE_DELAY;

        iowrite32(TS_POWERON, base + TS_CONFIG);

        if(!data->irq_enabled)
        {
            data->irq_enabled = true;
            enable_irq(data->irq);
        }
    }
    else
    {
        iowrite32(TS_POWERDOWN, base + TS_CONFIG);

        tz->polling_delay = 0;
        tz->passive_delay = 0;

        if(data->irq_enabled)
        {
            disable_irq(data->irq);
            data->irq_enabled = false;
        }
    }

    data->mode = mode;
    thermal_zone_device_update(tz, THERMAL_EVENT_UNSPECIFIED);

    return 0;    
}

static int canaan_get_trip_type(struct thermal_zone_device *tz, int trip,
            enum thermal_trip_type *type)
{

    return 0;
}

static int canaan_get_crit_temp(struct thermal_zone_device *tz, int *temp)
{

    return 0;
}

static int canaan_get_trip_temp(struct thermal_zone_device *tz, int trip, 
            int *temp)
{

    return 0;
}

static int canaan_set_trip_temp(struct thermal_zone_device *tz, int trip,
            int temp)
{

    return 0;
}

static int canaan_bind(struct thermal_zone_device *tz,
            struct thermal_cooling_device *cdev)
{
    int ret;

    ret = thermal_zone_bind_cooling_device(tz, CANAAN_TRIP_PASSIVE, cdev,
                            THERMAL_NO_LIMIT,
                            THERMAL_NO_LIMIT,
                            THERMAL_WEIGHT_DEFAULT);

    if(ret)
    {

        return ret;
    }

    return 0;
}

static int canaan_unbind(struct thermal_zone_device *tz,
            struct thermal_cooling_device *cdev)
{
    int ret;

    ret = thermal_zone_unbind_cooling_device(tz, CANAAN_TRIP_PASSIVE, cdev);
    if(ret)
    {
        return ret;
    }

    return 0;
}




static struct thermal_zone_device_ops canaan_tz_ops = {
    .bind = canaan_bind,
    .unbind = canaan_unbind,
    .get_temp = canaan_get_temp,
    .get_mode = canaan_get_mode,
    .set_mode = canaan_set_mode,
    .get_trip_type = canaan_get_trip_type,
    .get_trip_temp = canaan_get_trip_temp,
    .get_crit_temp = canaan_get_crit_temp,
    .set_trip_temp = canaan_set_trip_temp,
};

#if 0
static irqreturn_t canaan_thermal_irq(int irq, void *dev)
{
    struct canaan_thermal_data *data = dev;

    /* In order to clear the interrupt, need to read the corresponding data  */
    // disable_irq_nosync(irq);
    data->irq_enabled = false;
    // enable_irq(irq);

    return IRQ_HANDLED;
}
#endif

static const struct of_device_id of_canaan_thermal_match[] = {
    { .compatible = "canaan,k510-tsensor"},
    { /* end */ }
};
MODULE_DEVICE_TABLE(of, of_canaan_thermal_match);

static int canaan_thermal_probe(struct platform_device *pdev)
{
    struct canaan_thermal_data *data;
    struct resource *res;
    int ret;

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if(!data)
        return -ENOMEM;
    
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    data->base = devm_ioremap_resource(&pdev->dev, res);
    if(IS_ERR(data->base))
        return PTR_ERR(data->base);
    
    data->irq = platform_get_irq(pdev, 0);
    if(data->irq < 0)
    {
        dev_err(&pdev->dev, "Can't claim IRQ\n");
        return data->irq;
    }

    platform_set_drvdata(pdev, data);
    mutex_init(&data->lock);

    data->policy = cpufreq_cpu_get(0);
    if(!data->policy)
    {
        pr_debug("%s, CPUFreq policy not found\n", __func__);
        return -EPROBE_DEFER;
    }

#ifdef COOLING_DEVICE
    /* we don't have cooling device yet */
    data->cdev = cpufreq_cooling_register(data->policy);
    if(IS_ERR(data->cdev))
    {
        ret = PTR_ERR(data->cdev);
        dev_err(&pdev->dev,
            "failed to register cpufreq cooling device: %d\n", ret);
        cpufreq_cpu_put(data->policy);
        return ret;
    }
#endif

    data->thermal_clk = devm_clk_get(&pdev->dev, NULL);
    if(IS_ERR(data->thermal_clk))
    {
        ret = PTR_ERR(data->thermal_clk);
        if(ret != -EPROBE_DEFER)
            dev_err(&pdev->dev,
                "failed to get thermal clk: %d\n", ret);
#ifdef COOLING_DEVICE
        cpufreq_cooling_unregister(data->cdev);
#endif
        cpufreq_cpu_put(data->policy);
        return ret;
    }

    ret = clk_prepare_enable(data->thermal_clk);
    if(ret)
    {
        dev_err(&pdev->dev, "failed to enable thermal clk: %d\n", ret);
#ifdef COOLING_DEVICE
        cpufreq_cooling_unregister(data->cdev);
#endif
        cpufreq_cpu_put(data->policy);
        return ret;
    }

    dev_info(&pdev->dev, "clk: %ld, irq: %d, ", clk_get_rate(data->thermal_clk), data->irq);

    data->tz = thermal_zone_device_register("canaan_thermal_zone", 
                                CANAAN_TRIP_MAX,
                                BIT(CANAAN_TRIP_PASSIVE), data,
                                &canaan_tz_ops, NULL,
                                CANAAN_PASSIVE_DELAY,
                                CANAAN_POLLING_DELAY);
    
    if(IS_ERR(data->tz))
    {
        ret = PTR_ERR(data->tz);
        dev_err(&pdev->dev, 
            "failed to register thermal zone device %d\n", ret);
        clk_disable_unprepare(data->thermal_clk);
#ifdef COOLING_DEVICE
        cpufreq_cooling_unregister(data->cdev);
#endif
        cpufreq_cpu_put(data->policy);
        return ret;
    }

    data->irq_enabled = true;
    iowrite32(TS_POWERON, data->base + TS_CONFIG);
    data->mode = THERMAL_DEVICE_DISABLED;

#if 0    
    ret = devm_request_irq(&pdev->dev, data->irq,
            canaan_thermal_irq, 0, dev_name(&pdev->dev), data);
    if(ret < 0)
    {
        dev_err(&pdev->dev, "failed to request irq: %d\n", ret);
        clk_disable_unprepare(data->thermal_clk);
        thermal_zone_device_unregister(data->tz);
#ifdef COOLING_DEVICE
        cpufreq_cooling_unregister(data->cdev);
#endif    
        cpufreq_cpu_put(data->policy);
        return ret;    
    }
#endif

    return 0;
}

static int canaan_thermal_remove(struct platform_device *pdev)
{
    struct canaan_thermal_data *data = platform_get_drvdata(pdev);

    disable_irq(data->irq);
    thermal_zone_device_unregister(data->tz);
#ifdef COOLING_DEVICE
    cpufreq_cooling_unregister(data->cdev);
#endif
	cpufreq_cpu_put(data->policy);
    
    return 0;
}

static struct platform_driver canaan_thermal = {
    .driver = {
        .name = "canaan_thermal",
        .of_match_table = of_canaan_thermal_match,
    },
    .probe = canaan_thermal_probe,
    .remove = canaan_thermal_remove,
};
module_platform_driver(canaan_thermal);

MODULE_DESCRIPTION("Thermal driver for canaan k510 Soc");
MODULE_LICENSE("GPL v2");

