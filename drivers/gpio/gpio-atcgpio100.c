#include <linux/module.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/err.h>

#define GPIO_DATA_OUT			0x24
#define GPIO_DATA_IN			0x20
#define PIN_DIR				0x28
#define PIN_PULL_ENABLE			0x40
#define PIN_PULL_TYPE			0x44
#define INT_ENABLE			0x50
#define INT_STATE			0x64
#define INT_MODE(ch)			(0x54+((ch/8)<<2))
#define HIGH_LEVEL			2
#define LOW_LEVEL			3
#define NEGATIVE_EDGE			5
#define POSITIVE_EDGE			6
#define DUAL_EDGE			7
#define DEBOUNCE_ENABLE			0x70

#define ATCGPIO100_VIRTUAL_IRQ_BASE	32

struct atcgpio_priv {
	struct gpio_chip gc;
	struct irq_chip	chip;
	void __iomem *base;
	int virq_base;
	spinlock_t lock;
};

#define GPIO_READL(offset, base)	\
	readl((void __iomem *)base + (offset))

#define GPIO_WRITEL(val, offset, base)	\
	writel((val), (void __iomem *)base + (offset))

static inline struct atcgpio_priv *atcgpio_irq_data_get_data(struct irq_data *d)
{
	struct gpio_chip *chip = irq_data_get_irq_chip_data(d);

	return gpiochip_get_data(chip);
}

static int atcirq_to_gpio(unsigned int irq)
{
	return irq - gpio_to_irq(0);
}

static int atcgpio_to_irq(struct gpio_chip *gc, unsigned int offset)
{
	struct atcgpio_priv *priv;

	priv = gpiochip_get_data(gc);

	return priv->virq_base+ offset;
}

static int atcgpio_get(struct gpio_chip *gc, unsigned int gpio)
{
	struct atcgpio_priv *priv;

	priv = gpiochip_get_data(gc);

	return (GPIO_READL(GPIO_DATA_IN, priv->base) >> gpio & 1);
}

static void atcgpio_set(struct gpio_chip *gc, unsigned int gpio, int data)
{
	unsigned long val;
	struct atcgpio_priv *priv;

	priv = gpiochip_get_data(gc);

	if (data)
		val = GPIO_READL(GPIO_DATA_OUT, priv->base) | (0x1UL << gpio);
	else
		val = GPIO_READL(GPIO_DATA_OUT, priv->base) & ~(0x1UL << gpio);

	GPIO_WRITEL(val, GPIO_DATA_OUT, priv->base);
}

static int atcgpio_dir_in(struct gpio_chip *gc, unsigned int gpio)
{
	unsigned long val;
	unsigned long flags;
	struct atcgpio_priv *priv;

	priv = gpiochip_get_data(gc);
	spin_lock_irqsave(&priv->lock, flags);
	val = GPIO_READL(PIN_DIR, priv->base) & ~(0x1UL << gpio);
	GPIO_WRITEL(val, PIN_DIR, priv->base);
	spin_unlock_irqrestore(&priv->lock, flags);

	return 0;
}

static int atcgpio_dir_out(struct gpio_chip *gc, unsigned int gpio, int data)
{
	unsigned long val;
	unsigned long flags;
	struct atcgpio_priv *priv;

	priv = gpiochip_get_data(gc);
	spin_lock_irqsave(&priv->lock, flags);
	val = GPIO_READL(PIN_DIR, priv->base) | (0x1UL << gpio);
	GPIO_WRITEL(val, PIN_DIR, priv->base);
	gc->set(gc, gpio, data);
	spin_unlock_irqrestore(&priv->lock, flags);

	return 0;
}

static void atcgpio_irq_ack(struct irq_data *data)
{
	struct atcgpio_priv *priv;
	unsigned long flags;

	priv = atcgpio_irq_data_get_data(data);
	spin_lock_irqsave(&priv->lock, flags);
	GPIO_WRITEL(0x1UL << atcirq_to_gpio(data->irq), INT_STATE, priv->base);
	spin_unlock_irqrestore(&priv->lock, flags);
}

static void atcgpio_irq_unmask(struct irq_data *data)
{
	unsigned long val;
	unsigned long flags;
	struct atcgpio_priv *priv;
	
	priv = atcgpio_irq_data_get_data(data);
	spin_lock_irqsave(&priv->lock, flags);
	val = GPIO_READL(INT_ENABLE, priv->base) | (0x1UL << atcirq_to_gpio(data->irq));
	GPIO_WRITEL(val, INT_ENABLE, priv->base);
	spin_unlock_irqrestore(&priv->lock, flags);
}

static void atcgpio_irq_mask(struct irq_data *data)
{
	unsigned long val;
	unsigned long flags;
	struct atcgpio_priv *priv;

	priv = atcgpio_irq_data_get_data(data);
	spin_lock_irqsave(&priv->lock, flags);
	val = GPIO_READL(INT_ENABLE, priv->base) & ~(0x1UL << atcirq_to_gpio(data->irq));
	GPIO_WRITEL(val, INT_ENABLE, priv->base);
	spin_unlock_irqrestore(&priv->lock, flags);
}

static int atcgpio_irq_set_type(struct irq_data *data, unsigned int flow_type)
{
	unsigned long gpio = atcirq_to_gpio(data->irq);
	unsigned long ch = gpio%8;
	unsigned long mode_off = ch<<2;
	unsigned long val;
	unsigned long flags;
	struct atcgpio_priv *priv;

	priv = atcgpio_irq_data_get_data(data);

	spin_lock_irqsave(&priv->lock, flags);
	val = GPIO_READL(INT_MODE(gpio), priv->base);
	val &= ~(7<<mode_off);
	if (flow_type & IRQF_TRIGGER_RISING && flow_type & IRQF_TRIGGER_FALLING)
		GPIO_WRITEL(val | DUAL_EDGE<<mode_off, INT_MODE(gpio), priv->base);
	else if (flow_type & IRQF_TRIGGER_FALLING)
		GPIO_WRITEL(val | NEGATIVE_EDGE<<mode_off, INT_MODE(gpio), priv->base);
	else if (flow_type & IRQF_TRIGGER_RISING)
		GPIO_WRITEL(val | POSITIVE_EDGE<<mode_off, INT_MODE(gpio), priv->base);
	spin_unlock_irqrestore(&priv->lock, flags);

	return 0;
}

static void gpio_irq_router(struct irq_desc *desc)
{
	unsigned long status;
	struct atcgpio_priv *priv;
	int i = 0;

	priv = irq_desc_get_handler_data(desc);
	status = GPIO_READL(INT_STATE, priv->base);
	status &= ~((1 << 22) | (1 << 25) | (1 << 26));

	while (status) {
		if (status & 0x1UL)
			generic_handle_irq(gpio_to_irq(i));
		status >>= 1;
		i++;
	}
}

static int atcgpio100_gpio_probe(struct platform_device *pdev)
{
	struct resource *res, *irq_res;
	int ret, err;
	struct atcgpio_priv *priv;
	struct gpio_chip *gc;
	struct irq_chip *ic;

	priv = devm_kzalloc(&pdev->dev,	sizeof(*priv), GFP_KERNEL);

	if (!priv)
		return -ENOMEM;


	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "failed to request atcgpio100 resource\n");
		return -ENOENT;
	}

	irq_res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!irq_res) {
		dev_err(&pdev->dev, "failed to request atcgpio100 irq\n");
		return -ENOENT;
	}

	priv->virq_base = ATCGPIO100_VIRTUAL_IRQ_BASE;
	priv->base = devm_ioremap_resource(&pdev->dev, res);
	spin_lock_init(&priv->lock);

	if (IS_ERR((void *)priv->base))
		return PTR_ERR((void *)priv->base);

	/* disable interrupt */
	GPIO_WRITEL(0x00000000UL, INT_ENABLE, priv->base);
	/* clear interrupt */
	GPIO_WRITEL(0x0000FFFFUL, INT_STATE, priv->base);
	/* enable de-bouncing */
	GPIO_WRITEL(0x0000FFFFUL, DEBOUNCE_ENABLE, priv->base);
	/* enable interrupt */
	GPIO_WRITEL(0x0000FFFFUL, INT_ENABLE, priv->base);
	gc = &priv->gc;
	gc->owner = THIS_MODULE;
	gc->parent = &pdev->dev;
	gc->label = "atcgpio100";
	gc->base = 0;
	gc->ngpio = 16;
	gc->direction_output = atcgpio_dir_out;
	gc->direction_input = atcgpio_dir_in;
	gc->set = atcgpio_set;
	gc->get = atcgpio_get;
	gc->to_irq = atcgpio_to_irq;
	ic = &priv->chip;
	ic->name = "ATCGPIO100_irq";
	ic->irq_ack = atcgpio_irq_ack;
	ic->irq_mask = atcgpio_irq_mask;
	ic->irq_unmask = atcgpio_irq_unmask;
	ic->irq_set_type = atcgpio_irq_set_type;

	err = gpiochip_add_data(gc, priv);
	if (err < 0)
		return err;

	platform_set_drvdata(pdev, priv);
	
	ret = gpiochip_irqchip_add(&priv->gc, ic,priv->virq_base,
		handle_level_irq,IRQ_TYPE_NONE);

	if (ret) {
		dev_err(&pdev->dev, "could not add irqchip\n");
		gpiochip_remove(&priv->gc);
		return ret;
	}
	gpiochip_set_chained_irqchip(&priv->gc, ic,irq_res->start, gpio_irq_router);
	pr_info("ATCGPIO100 module inserted\n");

	return 0;
}

static int atcgpio100_gpio_remove(struct platform_device *pdev)
{
	struct atcgpio_priv *priv = platform_get_drvdata(pdev);

	/* disable interrupt */
	GPIO_WRITEL(0x00000000UL, INT_ENABLE, priv->base);
	gpiochip_remove(&priv->gc);
	pr_info("GPIO module removed\n");
	
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id atcgpio100_gpio_match[] = {
	{
		.compatible = "andestech,atcgpio100",
	},
	{ },
};
#endif

static struct platform_driver atcgpio100_gpio_driver = {
	.probe		= atcgpio100_gpio_probe,
	.driver		= {
		.name	= "atcgpio100_gpio",
		.of_match_table = of_match_ptr(atcgpio100_gpio_match),
	},
	.remove		= atcgpio100_gpio_remove,
};

module_platform_driver(atcgpio100_gpio_driver);
MODULE_DESCRIPTION("ATCGPIO100");
MODULE_LICENSE("GPL");
