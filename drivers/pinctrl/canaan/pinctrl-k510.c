/*
 * Generic device tree based pinctrl driver for one register per pin
 * type pinmux controllers
 *
 * Copyright (C) 2012 Texas Instruments, Inc.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/interrupt.h>

#include <linux/irqchip/chained_irq.h>

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf-generic.h>

//#include <linux/platform_data/pinctrl-single.h>

#include "../core.h"
#include "../devicetree.h"
#include "../pinconf.h"
#include "../pinmux.h"

#include "muxpin.h"

#define DRIVER_NAME			"pinctrl-k510"
#define PCS_OFF_DISABLED		~0U

/**
 * struct k510_pctrl_func_vals - mux function register offset and value pair
 * @reg:	register virtual address
 * @val:	register value
 */
struct k510_pinctrl_func_vals {
	void __iomem *reg;
	unsigned val;
	unsigned mask;
};


/**
 * struct k510_pinctrl_function - pinctrl function
 * @name:	pinctrl function name
 * @vals:	register and vals array
 * @nvals:	number of entries in vals array
 * @pgnames:	array of pingroup names the function uses
 * @npgnames:	number of pingroup names the function uses
 * @node:	list node
 */
struct k510_pinctrl_function {
	const char *name;
	struct k510_pinctrl_func_vals *vals;
	unsigned nvals;
	const char **pgnames;
	int npgnames;
	struct list_head node;
};

/**
 * struct k510_pinctrl_gpiofunc_range - pin ranges with same mux value of gpio function
 * @offset:	offset base of pins
 * @npins:	number pins with the same mux value of gpio function
 * @gpiofunc:	mux value of gpio function
 * @node:	list node
 */
struct k510_pinctrl_gpiofunc_range {
	unsigned offset;
	unsigned npins;
	unsigned gpiofunc;
	struct list_head node;
};

/**
 * struct k510_pinctrl_data - wrapper for data needed by pinctrl framework
 * @pa:		pindesc array
 * @cur:	index to current element
 *
 * REVISIT: We should be able to drop this eventually by adding
 * support for registering pins individually in the pinctrl
 * framework for those drivers that don't need a static array.
 */
struct k510_pinctrl_data {
	struct pinctrl_pin_desc *pa;
	int cur;
};

/**
 * struct k510_pinctrl_soc_data - SoC specific settings
 * @flags:	initial SoC specific PCS_FEAT_xxx values
 */
struct k510_pinctrl_soc_data {
	unsigned flags;
};

/**
 * struct pcs_device - pinctrl device instance
 * @res:	resources
 * @base:	virtual address of the controller
 * @size:	size of the ioremapped area
 * @dev:	device entry
 * @np:		device tree node
 * @pctl:	pin controller device
 * @flags:	mask of PCS_FEAT_xxx values
 * @missing_nr_pinctrl_cells: for legacy binding, may go away
 * @socdata:	soc specific data
 * @lock:	spinlock for register access
 * @mutex:	mutex protecting the lists
 * @width:	bits per mux register
 * @fmask:	function register mask
 * @fshift:	function register shift
 * @foff:	value to turn mux off
 * @fmax:	max number of functions in fmask
 * @bits_per_mux: number of bits per mux
 * @bits_per_pin: number of bits per pin
 * @pins:	physical pins on the SoC
 * @gpiofuncs:	list of gpio functions
 * @irqs:	list of interrupt registers
 * @chip:	chip container for this instance
 * @domain:	IRQ domain for this instance
 * @desc:	pin controller descriptor
 * @read:	register read function to use
 * @write:	register write function to use
 */
struct k510_pinctrl_device {
	struct resource *res;
	void __iomem *base;
	unsigned size;
	struct device *dev;
	struct device_node *np;
	struct pinctrl_dev *pctl;
	unsigned flags;
	struct property *missing_nr_pinctrl_cells;
	struct k510_pinctrl_soc_data socdata;
	raw_spinlock_t lock;
	struct mutex mutex;
	unsigned width;
	unsigned fmask;
	unsigned fshift;
	unsigned foff;
	unsigned fmax;
	bool bits_per_mux;
	unsigned bits_per_pin;
	struct k510_pinctrl_data pins;
	struct list_head gpiofuncs;
	struct list_head irqs;
	struct irq_chip chip;
	struct irq_domain *domain;
	struct pinctrl_desc desc;
	unsigned (*read)(void __iomem *reg);
	void (*write)(unsigned val, void __iomem *reg);
};



/*
 * REVISIT: Reads and writes could eventually use regmap or something
 * generic. But at least on omaps, some mux registers are performance
 * critical as they may need to be remuxed every time before and after
 * idle. Adding tests for register access width for every read and
 * write like regmap is doing is not desired, and caching the registers
 * does not help in this case.
 */

static unsigned __maybe_unused k510_pinctrl_readl(void __iomem *reg)
{
	return readl(reg);
}

static void __maybe_unused k510_pinctrl_writel(unsigned val, void __iomem *reg)
{
	writel(val, reg);
}

static void k510_pinctrl_pin_dbg_show(struct pinctrl_dev *pctldev,
					struct seq_file *s,
					unsigned pin)
{
	struct k510_pinctrl_device *pctrl_dev;
	unsigned val, mux_bytes;
	unsigned long offset;
	size_t pa;

	pctrl_dev = pinctrl_dev_get_drvdata(pctldev);

	mux_bytes = pctrl_dev->width / BITS_PER_BYTE;
	offset = pin * mux_bytes;
	val = pctrl_dev->read(pctrl_dev->base + offset);
	pa = pctrl_dev->res->start + offset;

	seq_printf(s, "%zx %08x %s ", pa, val, DRIVER_NAME);
}

static void k510_pinctrl_dt_free_map(struct pinctrl_dev *pctldev,
				struct pinctrl_map *map, unsigned num_maps)
{
	struct k510_pinctrl_device *pctrl_dev;

	pctrl_dev = pinctrl_dev_get_drvdata(pctldev);
	devm_kfree(pctrl_dev->dev, map);
}

static int k510_pinctrl_dt_node_to_map(struct pinctrl_dev *pctldev,
				struct device_node *np_config,
				struct pinctrl_map **map, unsigned *num_maps);

static const struct pinctrl_ops k510_pinctrl_ops = {
	.get_groups_count = pinctrl_generic_get_group_count,
	.get_group_name = pinctrl_generic_get_group_name,
	.get_group_pins = pinctrl_generic_get_group_pins,
	.pin_dbg_show = k510_pinctrl_pin_dbg_show,
	.dt_node_to_map = k510_pinctrl_dt_node_to_map,
	.dt_free_map = k510_pinctrl_dt_free_map,
};


static int k510_pinctrl_set_mux(struct pinctrl_dev *pctldev, unsigned fselector,
	unsigned group)
{
	struct k510_pinctrl_device *pctrl_dev;
	struct function_desc *function;
	struct k510_pinctrl_function *func;
	int i;

	pctrl_dev = pinctrl_dev_get_drvdata(pctldev);
	/* If function mask is null, needn't enable it. */
	if (!pctrl_dev->fmask)
		return 0;

	function = pinmux_generic_get_function(pctldev, fselector);
	func = function->data;
	if (!func)
		return -EINVAL;

	dev_dbg(pctrl_dev->dev, "enabling %s function%i\n",
		func->name, fselector);

	for (i = 0; i < func->nvals; i++) {
		struct k510_pinctrl_func_vals *vals;
		unsigned long flags;
		unsigned val, mask;

		vals = &func->vals[i];
		raw_spin_lock_irqsave(&pctrl_dev->lock, flags);
		val = pctrl_dev->read(vals->reg);

		mask = pctrl_dev->fmask;

		val &= ~mask;
		val |= (vals->val & mask);
		pctrl_dev->write(val, vals->reg);
		raw_spin_unlock_irqrestore(&pctrl_dev->lock, flags);
	}

	return 0;
}


static const struct pinmux_ops k510_pinmux_ops = {
	.get_functions_count = pinmux_generic_get_function_count,
	.get_function_name = pinmux_generic_get_function_name,
	.get_function_groups = pinmux_generic_get_function_groups,
	.set_mux = k510_pinctrl_set_mux,
};


/**
 * k510_pinctrl_add_pin() - add a pin to the static per controller pin array
 * @pctrl_dev: k510 pinctrl driver instance
 * @offset: register offset from base
 */
static int k510_pinctrl_add_pin(struct k510_pinctrl_device *pctrl_dev, unsigned offset,
		unsigned pin_pos)
{
	struct pinctrl_pin_desc *pin;
	int i;

	i = pctrl_dev->pins.cur;
	if (i >= pctrl_dev->desc.npins) {
		dev_err(pctrl_dev->dev, "too many pins, max %i\n",
			pctrl_dev->desc.npins);
		return -ENOMEM;
	}


	pin = &pctrl_dev->pins.pa[i];
	pin->number = i;
	pctrl_dev->pins.cur++;

	return i;
}

/**
 * k510_pinctrl_allocate_pin_table() - adds all the pins for the pinctrl driver
 * @pctrl_dev: k510 pinctrl driver instance
 *
 * In case of errors, resources are freed in pcs_free_resources.
 *
 * If your hardware needs holes in the address space, then just set
 * up multiple driver instances.
 */
static int k510_pinctrl_allocate_pin_table(struct k510_pinctrl_device *pctrl_dev)
{
	int mux_bytes, nr_pins, i;

	mux_bytes = pctrl_dev->width / BITS_PER_BYTE;

	nr_pins = pctrl_dev->size / mux_bytes;

	dev_dbg(pctrl_dev->dev, "allocating %i pins\n", nr_pins);
	pctrl_dev->pins.pa = devm_kzalloc(pctrl_dev->dev,
				sizeof(*pctrl_dev->pins.pa) * nr_pins,
				GFP_KERNEL);
	if (!pctrl_dev->pins.pa)
		return -ENOMEM;

	pctrl_dev->desc.pins = pctrl_dev->pins.pa;
	pctrl_dev->desc.npins = nr_pins;

	for (i = 0; i < pctrl_dev->desc.npins; i++) {
		unsigned offset;
		int res;
		int pin_pos = 0;

		offset = i * mux_bytes;

		res = k510_pinctrl_add_pin(pctrl_dev, offset, pin_pos);
		if (res < 0) {
			dev_err(pctrl_dev->dev, "error adding pins: %i\n", res);
			return res;
		}
	}

	return 0;
}

/**
 * k510_pinctrl_add_function() - adds a new function to the function list
 * @pctrl_dev,: k510 pinctrl driver instance
 * @np: device node of the mux entry
 * @name: name of the function
 * @vals: array of mux register value pairs used by the function
 * @nvals: number of mux register value pairs
 * @pgnames: array of pingroup names for the function
 * @npgnames: number of pingroup names
 */
static struct k510_pinctrl_function *k510_pinctrl_add_function(struct k510_pinctrl_device *pctrl_dev,
					struct device_node *np,
					const char *name,
					struct k510_pinctrl_func_vals *vals,
					unsigned nvals,
					const char **pgnames,
					unsigned npgnames)
{
	struct k510_pinctrl_function *function;
	int res;

	function = devm_kzalloc(pctrl_dev->dev, sizeof(*function), GFP_KERNEL);
	if (!function)
		return NULL;

	function->vals = vals;
	function->nvals = nvals;

	res = pinmux_generic_add_function(pctrl_dev->pctl, name,
					  pgnames, npgnames,
					  function);
	if (res)
		return NULL;

	return function;
}

/**
 * k510_pinctrl_get_pin_by_offset() - get a pin index based on the register offset
 * @pctrl_dev: k510 pinctrl driver instance
 * @offset: register offset from the base
 *
 * Note that this is OK as long as the pins are in a static array.
 */
static int k510_pinctrl_get_pin_by_offset(struct k510_pinctrl_device *pctrl_dev, unsigned offset)
{
	unsigned index;

	if (offset >= pctrl_dev->size) {
		dev_err(pctrl_dev->dev, "mux offset out of range: 0x%x (0x%x)\n",
			offset, pctrl_dev->size);
		return -EINVAL;
	}

	index = offset / (pctrl_dev->width / BITS_PER_BYTE);

	return index;
}







/**
 * k510_parse_one_pinctrl_entry() - parses a device tree mux entry
 * @pctrl_dev: pinctrl driver instance
 * @np: device node of the mux entry
 * @map: map entry
 * @num_maps: number of map
 * @pgnames: pingroup names
 *
 * Note that this binding currently supports only sets of one register + value.
 *
 * Also note that this driver tries to avoid understanding pin and function
 * names because of the extra bloat they would cause especially in the case of
 * a large number of pins. This driver just sets what is specified for the board
 * in the .dts file. Further user space debugging tools can be developed to
 * decipher the pin and function names using debugfs.
 *
 * If you are concerned about the boot time, set up the static pins in
 * the bootloader, and only set up selected pins as device tree entries.
 */
static int k510_parse_one_pinctrl_entry(struct k510_pinctrl_device *pctrl_dev,
						struct device_node *np,
						struct pinctrl_map **map,
						unsigned *num_maps,
						const char **pgnames)
{
	const char *name = "pinctrl-k510,pins";
	struct k510_pinctrl_func_vals *vals;
	int rows, *pins, found = 0, res = -ENOMEM, i;
	struct k510_pinctrl_function *function;

	rows = pinctrl_count_index_with_args(np, name);
	if (rows <= 0) {
		dev_err(pctrl_dev->dev, "Invalid number of rows: %d\n", rows);
		return -EINVAL;
	}

	vals = devm_kzalloc(pctrl_dev->dev, sizeof(*vals) * rows, GFP_KERNEL);
	if (!vals)
		return -ENOMEM;

	pins = devm_kzalloc(pctrl_dev->dev, sizeof(*pins) * rows, GFP_KERNEL);
	if (!pins)
		goto free_vals;

	for (i = 0; i < rows; i++) {
		struct of_phandle_args pinctrl_spec;
		unsigned int io_num, func_num;
		unsigned int offset;
		int pin;

		res = pinctrl_parse_index_with_args(np, name, i, &pinctrl_spec);
		if (res)
			return res;

		if (pinctrl_spec.args_count < 2) {
			dev_err(pctrl_dev->dev, "invalid args_count for spec: %i\n",
				pinctrl_spec.args_count);
			break;
		}

		/* Index plus one value cell */
		io_num = pinctrl_spec.args[0];
		func_num = pinctrl_spec.args[1];

		if ((func_num & MUXPIN_FUCTION_MASK) > FUNC_LSRESV_MAX) {
			if(io_num != (func_num & MUXPIN_IONUM_MASK) >> 16)
			{
				dev_warn(pctrl_dev->dev, "Warning:> Pin num %d, but want %d\n", io_num, (func_num & MUXPIN_IONUM_MASK) >> 16);
			}
			io_num = (func_num & MUXPIN_IONUM_MASK) >> 16;
			func_num = func_num & MUXPIN_FUCTION_MASK;
		}

		offset = io_num*4;
		vals[found].reg = pctrl_dev->base + offset;

		k510_pinctrl_func2val(func_num, (muxpin_config_t *)&vals[found].val);

		dev_dbg(pctrl_dev->dev, "%s index: 0x%x value: 0x%x\n",
			pinctrl_spec.np->name, offset, pinctrl_spec.args[1]);

		pin = k510_pinctrl_get_pin_by_offset(pctrl_dev, offset);
		if (pin < 0) {
			dev_err(pctrl_dev->dev,
				"could not add functions for %s %ux\n",
				np->name, offset);
			break;
		}
		pins[found++] = pin;
	}

	pgnames[0] = np->name;
	function = k510_pinctrl_add_function(pctrl_dev, np, np->name, vals, found, pgnames, 1);
	if (!function) {
		res = -ENOMEM;
		goto free_pins;
	}

	res = pinctrl_generic_add_group(pctrl_dev->pctl, np->name, pins, found, pctrl_dev);
	if (res < 0)
		goto free_function;

	(*map)->type = PIN_MAP_TYPE_MUX_GROUP;
	(*map)->data.mux.group = np->name;
	(*map)->data.mux.function = np->name;

	*num_maps = 1;
	
	return 0;

free_function:
	pinmux_generic_remove_last_function(pctrl_dev->pctl);

free_pins:
	devm_kfree(pctrl_dev->dev, pins);

free_vals:
	devm_kfree(pctrl_dev->dev, vals);

	return res;
}

/**
 * k510_pinctrl_dt_node_to_map() - allocates and parses pinctrl maps
 * @pctldev: pinctrl instance
 * @np_config: device tree pinmux entry
 * @map: array of map entries
 * @num_maps: number of maps
 */
static int k510_pinctrl_dt_node_to_map(struct pinctrl_dev *pctldev,
				struct device_node *np_config,
				struct pinctrl_map **map, unsigned *num_maps)
{
	struct k510_pinctrl_device *pctrl_dev;
	const char **pgnames;
	int ret;

	pctrl_dev = pinctrl_dev_get_drvdata(pctldev);

	/* create map for pinmux */
	*map = devm_kzalloc(pctrl_dev->dev, sizeof(**map), GFP_KERNEL);
	if (!*map)
		return -ENOMEM;

	*num_maps = 0;

	pgnames = devm_kzalloc(pctrl_dev->dev, sizeof(*pgnames), GFP_KERNEL);
	if (!pgnames) {
		ret = -ENOMEM;
		goto free_map;
	}

	ret = k510_parse_one_pinctrl_entry(pctrl_dev, np_config, map,
			num_maps, pgnames);
	if (ret < 0) {
		dev_err(pctrl_dev->dev, "no pins entries for %s\n",
			np_config->name);
		goto free_pgnames;
	}

	return 0;

free_pgnames:
	devm_kfree(pctrl_dev->dev, pgnames);
free_map:
	devm_kfree(pctrl_dev->dev, *map);

	return ret;
}


/**
 * k510_pinctrl_free_resources() - free memory used by this driver
 * @pctrl_dev: k510 pinctrl driver instance
 */
static void k510_pinctrl_free_resources(struct k510_pinctrl_device *pctrl_dev)
{
	pinctrl_unregister(pctrl_dev->pctl);

}

#if 0
static int k510_pinctrl_add_gpio_func(struct device_node *node, struct k510_pinctrl_device *pctrl_dev)
{
	const char *propname = "pinctrl-k510,gpio-range";
	const char *cellname = "#pinctrl-k510,gpio-range-cells";
	struct of_phandle_args gpiospec;
	struct k510_pinctrl_gpiofunc_range *range;
	int ret, i;

	for (i = 0; ; i++) {
		ret = of_parse_phandle_with_args(node, propname, cellname,
						 i, &gpiospec);
		/* Do not treat it as error. Only treat it as end condition. */
		if (ret) {
			ret = 0;
			break;
		}
		range = devm_kzalloc(pctrl_dev->dev, sizeof(*range), GFP_KERNEL);
		if (!range) {
			ret = -ENOMEM;
			break;
		}
		range->offset = gpiospec.args[0];
		range->npins = gpiospec.args[1];
		range->gpiofunc = gpiospec.args[2];
		mutex_lock(&pctrl_dev->mutex);
		list_add_tail(&range->node, &pctrl_dev->gpiofuncs);
		mutex_unlock(&pctrl_dev->mutex);
	}
	return ret;
}
#endif









#ifdef CONFIG_PM
static int k510_pinctrl_suspend(struct platform_device *pdev,
					pm_message_t state)
{
	struct k510_pinctrl_device *pctrl_dev;

	pctrl_dev = platform_get_drvdata(pdev);
	if (!pctrl_dev)
		return -EINVAL;

	return pinctrl_force_sleep(pctrl_dev->pctl);
}

static int k510_pinctrl_resume(struct platform_device *pdev)
{
	struct k510_pinctrl_device *pctrl_dev;

	pctrl_dev = platform_get_drvdata(pdev);
	if (!pctrl_dev)
		return -EINVAL;

	return pinctrl_force_default(pctrl_dev->pctl);
}
#endif


static int k510_pinctrl_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;
	struct k510_pinctrl_device *pctrl_dev;
	const struct k510_pinctrl_soc_data *soc;
	int ret;

	soc = of_device_get_match_data(&pdev->dev);
	if (WARN_ON(!soc))
		return -EINVAL;

	pctrl_dev = devm_kzalloc(&pdev->dev, sizeof(*pctrl_dev), GFP_KERNEL);
	if (!pctrl_dev)
		return -ENOMEM;

	pctrl_dev->dev = &pdev->dev;
	pctrl_dev->np = np;
	raw_spin_lock_init(&pctrl_dev->lock);
	mutex_init(&pctrl_dev->mutex);
	INIT_LIST_HEAD(&pctrl_dev->gpiofuncs);
	pctrl_dev->flags = soc->flags;
	memcpy(&pctrl_dev->socdata, soc, sizeof(*soc));

	ret = of_property_read_u32(np, "pinctrl-k510,register-width",
				   &pctrl_dev->width);
	if (ret) {
		dev_err(pctrl_dev->dev, "register width not specified\n");

		return ret;
	}

	ret = of_property_read_u32(np, "pinctrl-k510,function-mask",
				   &pctrl_dev->fmask);
	if (!ret) {
		pctrl_dev->fshift = __ffs(pctrl_dev->fmask);
		pctrl_dev->fmax = pctrl_dev->fmask >> pctrl_dev->fshift;
	} else {
		/* If mask property doesn't exist, function mux is invalid. */
		pctrl_dev->fmask = 0;
		pctrl_dev->fshift = 0;
		pctrl_dev->fmax = 0;
	}


	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(pctrl_dev->dev, "could not get resource\n");
		return -ENODEV;
	}

	pctrl_dev->res = devm_request_mem_region(pctrl_dev->dev, res->start,
			resource_size(res), DRIVER_NAME);
	if (!pctrl_dev->res) {
		dev_err(pctrl_dev->dev, "could not get mem_region\n");
		return -EBUSY;
	}

	pctrl_dev->size = resource_size(pctrl_dev->res);
	pctrl_dev->base = devm_ioremap(pctrl_dev->dev, pctrl_dev->res->start, pctrl_dev->size);
	if (!pctrl_dev->base) {
		dev_err(pctrl_dev->dev, "could not ioremap\n");
		return -ENODEV;
	}

	platform_set_drvdata(pdev, pctrl_dev);

	pctrl_dev->read = k510_pinctrl_readl;
	pctrl_dev->write = k510_pinctrl_writel;

	pctrl_dev->desc.name = DRIVER_NAME;
	pctrl_dev->desc.pctlops = &k510_pinctrl_ops;
	pctrl_dev->desc.pmxops = &k510_pinmux_ops;
	pctrl_dev->desc.owner = THIS_MODULE;

	ret = k510_pinctrl_allocate_pin_table(pctrl_dev);
	if (ret < 0)
		goto free;

	ret = pinctrl_register_and_init(&pctrl_dev->desc, pctrl_dev->dev, pctrl_dev, &pctrl_dev->pctl);
	if (ret) {
		dev_err(pctrl_dev->dev, "could not register single pinctrl driver\n");
		goto free;
	}

/*
	ret = k510_pinctrl_add_gpio_func(np, pctrl_dev);
	if (ret < 0)
		goto free;
*/

	dev_info(pctrl_dev->dev, "%i pins, size %u\n", pctrl_dev->desc.npins, pctrl_dev->size);

	return pinctrl_enable(pctrl_dev->pctl);

free:
	k510_pinctrl_free_resources(pctrl_dev);

	return ret;
}

static int k510_pinctrl_remove(struct platform_device *pdev)
{
	struct k510_pinctrl_device *pctrl_dev = platform_get_drvdata(pdev);

	if (!pctrl_dev)
		return 0;

	k510_pinctrl_free_resources(pctrl_dev);

	return 0;
}


static const struct k510_pinctrl_soc_data pinctrl_k510 = {
};


static const struct of_device_id k510_pinctrl_of_match[] = {
	{ .compatible = "pinctrl-k510", .data = &pinctrl_k510 },
	{ },
};
MODULE_DEVICE_TABLE(of, k510_pinctrl_of_match);

static struct platform_driver k510_pinctrl_driver = {
	.probe		= k510_pinctrl_probe,
	.remove		= k510_pinctrl_remove,
	.driver = {
		.name		= DRIVER_NAME,
		.of_match_table	= k510_pinctrl_of_match,
	},
#ifdef CONFIG_PM
	.suspend = k510_pinctrl_suspend,
	.resume = k510_pinctrl_resume,
#endif
};

static int __init k510_pinctrl_init(void)
{
	return platform_driver_register(&k510_pinctrl_driver);
}
arch_initcall(k510_pinctrl_init);

MODULE_DESCRIPTION("k510 pinctrl driver");
MODULE_LICENSE("GPL v2");
