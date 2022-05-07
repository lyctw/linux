/*
 * Designware SPI core controller driver (refer pxa2xx_spi.c)
 *
 * Copyright (c) 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/highmem.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/spi/spi-mem.h>
#include <linux/delay.h>

#include "spi-dw.h"

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif

/* Slave spi_dev related */
struct chip_data {
	u8 tmode;		/* TR/TO/RO/EEPROM */
	u8 type;		/* SPI/SSP/MicroWire */

	u8 poll_mode;		/* 1 means use poll mode */

	u16 clk_div;		/* baud rate divider */
	u32 speed_hz;		/* baud rate */
	void (*cs_control)(u32 command);
	u32 cr0;
	u32 rx_sample_dly;	/* RX sample delay */
};

#ifdef CONFIG_DEBUG_FS
#define SPI_REGS_BUFSIZE	1024
static ssize_t dw_spi_show_regs(struct file *file, char __user *user_buf,
		size_t count, loff_t *ppos)
{
	struct dw_spi *dws = file->private_data;
	char *buf;
	u32 len = 0;
	ssize_t ret;

	buf = kzalloc(SPI_REGS_BUFSIZE, GFP_KERNEL);
	if (!buf)
		return 0;

	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"%s registers:\n", dev_name(&dws->master->dev));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"=================================\n");
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"CTRL0: \t\t0x%08x\n", dw_readl(dws, DW_SPI_CTRL0));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"SPI_CTRL0: \t\t0x%08x\n", dw_readl(dws, DW_SPI_SPI_CTRLR0));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"CTRL1: \t\t0x%08x\n", dw_readl(dws, DW_SPI_CTRL1));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"SSIENR: \t0x%08x\n", dw_readl(dws, DW_SPI_SSIENR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"SER: \t\t0x%08x\n", dw_readl(dws, DW_SPI_SER));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"BAUDR: \t\t0x%08x\n", dw_readl(dws, DW_SPI_BAUDR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"TXFTLR: \t0x%08x\n", dw_readl(dws, DW_SPI_TXFLTR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"RXFTLR: \t0x%08x\n", dw_readl(dws, DW_SPI_RXFLTR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"TXFLR: \t\t0x%08x\n", dw_readl(dws, DW_SPI_TXFLR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"RXFLR: \t\t0x%08x\n", dw_readl(dws, DW_SPI_RXFLR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"SR: \t\t0x%08x\n", dw_readl(dws, DW_SPI_SR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"IMR: \t\t0x%08x\n", dw_readl(dws, DW_SPI_IMR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"ISR: \t\t0x%08x\n", dw_readl(dws, DW_SPI_ISR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"DMACR: \t\t0x%08x\n", dw_readl(dws, DW_SPI_DMACR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"DMATDLR: \t0x%08x\n", dw_readl(dws, DW_SPI_DMATDLR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"DMARDLR: \t0x%08x\n", dw_readl(dws, DW_SPI_DMARDLR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"=================================\n");

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);
	return ret;
}

static const struct file_operations dw_spi_regs_ops = {
	.owner		= THIS_MODULE,
	.open		= simple_open,
	.read		= dw_spi_show_regs,
	.llseek		= default_llseek,
};

static int dw_spi_debugfs_init(struct dw_spi *dws)
{
	char name[32];

	snprintf(name, 32, "dw_spi%d", dws->master->bus_num);
	dws->debugfs = debugfs_create_dir(name, NULL);
	if (!dws->debugfs)
		return -ENOMEM;

	debugfs_create_file("registers", S_IFREG | S_IRUGO,
		dws->debugfs, (void *)dws, &dw_spi_regs_ops);
	return 0;
}

static void dw_spi_debugfs_remove(struct dw_spi *dws)
{
	debugfs_remove_recursive(dws->debugfs);
}

#else
static inline int dw_spi_debugfs_init(struct dw_spi *dws)
{
	return 0;
}

static inline void dw_spi_debugfs_remove(struct dw_spi *dws)
{
}
#endif /* CONFIG_DEBUG_FS */

static void dw_spi_set_cs(struct spi_device *spi, bool enable)
{
	struct dw_spi *dws = spi_controller_get_devdata(spi->controller);
	struct chip_data *chip = spi_get_ctldata(spi);

	/* Chip select logic is inverted from spi_set_cs() */
	if (chip && chip->cs_control)
		chip->cs_control(!enable);

	if (!enable)
		dw_writel(dws, DW_SPI_SER, BIT(spi->chip_select));
}

/* Return the max entries we can fill into tx fifo */
static inline u32 tx_max(struct dw_spi *dws)
{
	u32 tx_room, rxtx_gap;

	tx_room = dws->fifo_len - dw_readl(dws, DW_SPI_TXFLR);

	/*
	 * Another concern is about the tx/rx mismatch, we
	 * though to use (dws->fifo_len - rxflr - txflr) as
	 * one maximum value for tx, but it doesn't cover the
	 * data which is out of tx/rx fifo and inside the
	 * shift registers. So a control from sw point of
	 * view is taken.
	 */
	rxtx_gap = dws->fifo_len - (dws->rx_len - dws->tx_len);

	return min3((u32)dws->tx_len, tx_room, rxtx_gap);
}

/* Return the max entries we should read out of rx fifo */
static inline u32 rx_max(struct dw_spi *dws)
{
	return min_t(u32, dws->rx_len, dw_readl(dws, DW_SPI_RXFLR));
}

static void dw_writer(struct dw_spi *dws)
{
	u32 max = tx_max(dws);
	u16 txw = 0;

	while (max--) {
		if (dws->tx) {
			if (dws->n_bytes == 1)
				txw = *(u8 *)(dws->tx);
			else if (dws->n_bytes == 2)
				txw = *(u16 *)(dws->tx);
			else
				txw = *(u32 *)(dws->tx);

			dws->tx += dws->n_bytes;
		}
		dw_write_io_reg(dws, DW_SPI_DR, txw);
		--dws->tx_len;
	}
}

static void dw_reader(struct dw_spi *dws)
{
	u32 max = rx_max(dws);
	u16 rxw;

	while (max--) {
		rxw = dw_read_io_reg(dws, DW_SPI_DR);
		if (dws->rx) {
			if (dws->n_bytes == 1)
				*(u8 *)(dws->rx) = rxw;
			else if (dws->n_bytes == 2)
				*(u16 *)(dws->rx) = rxw;
			else
				*(u32 *)(dws->rx) = rxw;

			dws->rx += dws->n_bytes;
		}
		--dws->rx_len;
	}
}

int dw_spi_check_status(struct dw_spi *dws, bool raw)
{
	u32 irq_status;
	int ret = 0;

	if (raw)
		irq_status = dw_readl(dws, DW_SPI_RISR);
	else
		irq_status = dw_readl(dws, DW_SPI_ISR);

	if (irq_status & SPI_INT_RXOI) {
		dev_err(&dws->master->dev, "RX FIFO overflow detected\n");
		ret = -EIO;
	}

	if (irq_status & SPI_INT_RXUI) {
		dev_err(&dws->master->dev, "RX FIFO underflow detected\n");
		ret = -EIO;
	}

	if (irq_status & SPI_INT_TXOI) {
		dev_err(&dws->master->dev, "TX FIFO overflow detected\n");
		ret = -EIO;
	}

	/* Generically handle the erroneous situation */
	if (ret) {
		spi_reset_chip(dws);
		if (dws->master->cur_msg)
			dws->master->cur_msg->status = ret;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(dw_spi_check_status);

static void int_error_stop(struct dw_spi *dws, const char *msg)
{
	spi_reset_chip(dws);

	dev_err(&dws->master->dev, "%s\n", msg);
	dws->master->cur_msg->status = -EIO;
	spi_finalize_current_transfer(dws->master);
}

static irqreturn_t dw_spi_transfer_handler(struct dw_spi *dws)
{
	u16 irq_status = dw_readl(dws, DW_SPI_ISR);

	if (dw_spi_check_status(dws, false)) {
		spi_finalize_current_transfer(dws->master);
		return IRQ_HANDLED;
	}

	/*
	 * Read data from the Rx FIFO every time we've got a chance executing
	 * this method. If there is nothing left to receive, terminate the
	 * procedure. Otherwise adjust the Rx FIFO Threshold level if it's a
	 * final stage of the transfer. By doing so we'll get the next IRQ
	 * right when the leftover incoming data is received.
	 */
	dw_reader(dws);
	if (!dws->rx_len) {
		spi_mask_intr(dws, 0xff);
		spi_finalize_current_transfer(dws->master);
	} else if (dws->rx_len <= dw_readl(dws, DW_SPI_RXFLTR)) {
		dw_writel(dws, DW_SPI_RXFLTR, dws->rx_len - 1);
	}

	/*
	 * Send data out if Tx FIFO Empty IRQ is received. The IRQ will be
	 * disabled after the data transmission is finished so not to
	 * have the TXE IRQ flood at the final stage of the transfer.
	 */
	if (irq_status & SPI_INT_TXEI) {
		dw_writer(dws);
		if (!dws->tx_len)
			spi_mask_intr(dws, SPI_INT_TXEI);
	}

	return IRQ_HANDLED;
}

static irqreturn_t dw_spi_irq(int irq, void *dev_id)
{
	struct spi_controller *master = dev_id;
	struct dw_spi *dws = spi_controller_get_devdata(master);
	u16 irq_status = dw_readl(dws, DW_SPI_ISR) & 0x3f;

	if (!irq_status)
		return IRQ_NONE;

	if (!master->cur_msg) {
		spi_mask_intr(dws, SPI_INT_TXEI);
		return IRQ_HANDLED;
	}

	return dws->transfer_handler(dws);
}

void dw_spi_update_config(struct dw_spi *dws, struct spi_device *spi,
			  struct dw_spi_cfg *cfg)
{
	struct chip_data *chip = spi_get_ctldata(spi);
	u32 cr0 = chip->cr0;
	u32 speed_hz;
	u16 clk_div;

	/* CTRLR0[ 4/3: 0] Data Frame Size */
	cr0 |= (cfg->dfs - 1);

	/* CTRLR0[11:10] Transfer Mode */
	cr0 |= cfg->tmode << DWC_SSI_CTRLR0_TMOD_OFFSET;

	dw_writel(dws, DW_SPI_CTRL0, cr0);

	if (cfg->tmode == SPI_TMOD_EPROMREAD || cfg->tmode == SPI_TMOD_RO)
		dw_writel(dws, DW_SPI_CTRL1, cfg->ndf ? cfg->ndf - 1 : 0);

	/* Note DW APB SSI clock divider doesn't support odd numbers */
	clk_div = (DIV_ROUND_UP(dws->max_freq, cfg->freq) + 1) & 0xfffe;
	speed_hz = dws->max_freq / clk_div;

	if (dws->current_freq != speed_hz) {
		spi_set_clk(dws, clk_div);
		dws->current_freq = speed_hz;
	}

	//printk("max_freq: %d, cfg->freq: %d, current_freq: %d, speed_hz: %d, div: %d\n", dws->max_freq, cfg->freq, dws->current_freq, speed_hz, clk_div);
	/* Update RX sample delay if required */
	if (dws->cur_rx_sample_dly != chip->rx_sample_dly) {
		dw_writel(dws, DW_SPI_RX_SAMPLE_DLY, chip->rx_sample_dly);
		dws->cur_rx_sample_dly = chip->rx_sample_dly;
	}
}
EXPORT_SYMBOL_GPL(dw_spi_update_config);

static void dw_spi_irq_setup(struct dw_spi *dws)
{
	u16 level;
	u8 imask;

	/*
	 * Originally Tx and Rx data lengths match. Rx FIFO Threshold level
	 * will be adjusted at the final stage of the IRQ-based SPI transfer
	 * execution so not to lose the leftover of the incoming data.
	 */
	level = min_t(u16, dws->fifo_len / 2, dws->tx_len);
	dw_writel(dws, DW_SPI_TXFLTR, level);
	dw_writel(dws, DW_SPI_RXFLTR, level - 1);

	dws->transfer_handler = dw_spi_transfer_handler;

	imask = SPI_INT_TXEI | SPI_INT_TXOI | SPI_INT_RXUI | SPI_INT_RXOI |
		SPI_INT_RXFI;
	spi_umask_intr(dws, imask);
}

/* Must be called inside pump_transfers() */
/*
 * The iterative procedure of the poll-based transfer is simple: write as much
 * as possible to the Tx FIFO, wait until the pending to receive data is ready
 * to be read, read it from the Rx FIFO and check whether the performed
 * procedure has been successful.
 *
 * Note this method the same way as the IRQ-based transfer won't work well for
 * the SPI devices connected to the controller with native CS due to the
 * automatic CS assertion/de-assertion.
 */
static int dw_spi_poll_transfer(struct dw_spi *dws,
				struct spi_transfer *transfer)
{
	struct spi_delay delay;
	u16 nbits;
	int ret;

	delay.unit = SPI_DELAY_UNIT_SCK;
	nbits = dws->n_bytes * 8;

	do {
		dw_writer(dws);

		delay.value = nbits * (dws->rx_len - dws->tx_len);
		spi_delay_exec(&delay, transfer);

		dw_reader(dws);

		ret = dw_spi_check_status(dws, true);
		if (ret)
			return ret;
	} while (dws->rx_len);

	return 0;
}

//yangguang: add dwc_ssi support
static u8 dw_spi_update_tmode(struct dw_spi *dws)
{
       if (!dws->tx)
               return SPI_TMOD_RO;
       if (!dws->rx)
               return SPI_TMOD_TO;

       return SPI_TMOD_TR;
}

/* Configure CTRLR0 for DW_apb_ssi */
u32 dw_spi_update_cr0(struct spi_controller *master, struct spi_device *spi,
                     struct spi_transfer *transfer)
{
       struct dw_spi *dws = spi_controller_get_devdata(master);
       struct chip_data *chip = spi_get_ctldata(spi);
       u32 cr0;

       /* Default SPI mode is SCPOL = 0, SCPH = 0 */
       cr0 = (transfer->bits_per_word - 1)
               | (chip->type << SPI_FRF_OFFSET)
               | ((((spi->mode & SPI_CPOL) ? 1 : 0) << SPI_SCOL_OFFSET) |
                  (((spi->mode & SPI_CPHA) ? 1 : 0) << SPI_SCPH_OFFSET) |
                  (((spi->mode & SPI_LOOP) ? 1 : 0) << SPI_SRL_OFFSET))
               | (chip->tmode << SPI_TMOD_OFFSET);

       /*
        * Adjust transfer mode if necessary. Requires platform dependent
        * chipselect mechanism.
        */
       if (chip->cs_control) {
               chip->tmode = dw_spi_update_tmode(dws);

               cr0 &= ~SPI_TMOD_MASK;
               cr0 |= (chip->tmode << SPI_TMOD_OFFSET);
       }

       return cr0;
}
EXPORT_SYMBOL_GPL(dw_spi_update_cr0);

/* Configure CTRLR0 for DWC_ssi */
u32 dw_spi_update_cr0_v1_01a(struct spi_controller *master,
                            struct spi_device *spi,
                            struct spi_transfer *transfer)
{
       struct dw_spi *dws = spi_controller_get_devdata(master);
       struct chip_data *chip = spi_get_ctldata(spi);
       u32 cr0;

       /* CTRLR0[ 4: 0] Data Frame Size */
       cr0 = (transfer->bits_per_word - 1);

       /* CTRLR0[ 7: 6] Frame Format */
       cr0 |= chip->type << DWC_SSI_CTRLR0_FRF_OFFSET;

       /*
        * SPI mode (SCPOL|SCPH)
        * CTRLR0[ 8] Serial Clock Phase
        * CTRLR0[ 9] Serial Clock Polarity
        */
       cr0 |= ((spi->mode & SPI_CPOL) ? 1 : 0) << DWC_SSI_CTRLR0_SCPOL_OFFSET;
       cr0 |= ((spi->mode & SPI_CPHA) ? 1 : 0) << DWC_SSI_CTRLR0_SCPH_OFFSET;

       /* CTRLR0[11:10] Transfer Mode */
       cr0 |= chip->tmode << DWC_SSI_CTRLR0_TMOD_OFFSET;

       /* CTRLR0[13] Shift Register Loop */
       cr0 |= ((spi->mode & SPI_LOOP) ? 1 : 0) << DWC_SSI_CTRLR0_SRL_OFFSET;

       /* Adjust Transfer Mode if necessary */
       if (chip->cs_control) {
               chip->tmode = dw_spi_update_tmode(dws);

               cr0 &= ~DWC_SSI_CTRLR0_TMOD_MASK;
               cr0 |= chip->tmode << DWC_SSI_CTRLR0_TMOD_OFFSET;
       }

       return cr0;
}
EXPORT_SYMBOL_GPL(dw_spi_update_cr0_v1_01a);

static int dw_spi_transfer_one(struct spi_controller *master,
		struct spi_device *spi, struct spi_transfer *transfer)
{
	struct dw_spi *dws = spi_controller_get_devdata(master);
	struct chip_data *chip = spi_get_ctldata(spi);
	u8 imask = 0;
	u16 txlevel = 0;
	u32 cr0;
	int ret;

	struct dw_spi_cfg cfg = {
		.tmode = SPI_TMOD_TR,
		.dfs = transfer->bits_per_word,
		.freq = transfer->speed_hz,
	};
	
	dws->dma_mapped = 0;
	dws->n_bytes = DIV_ROUND_UP(transfer->bits_per_word, 8);
	dws->tx = (void *)transfer->tx_buf;
	dws->tx_len = transfer->len / dws->n_bytes;
	dws->rx = transfer->rx_buf;
	dws->rx_len = dws->tx_len;

	/* Ensure the data above is visible for all CPUs */
	smp_mb();

	spi_enable_chip(dws, 0);

	dw_spi_update_config(dws, spi, &cfg);

	transfer->effective_speed_hz = dws->current_freq;

	/* Check if current transfer is a DMA transaction */
	if (master->can_dma && master->can_dma(master, spi, transfer))
		dws->dma_mapped = master->cur_msg_mapped;

	/* For poll mode just disable all interrupts */
	spi_mask_intr(dws, 0xff);

	/*
	 * Interrupt mode
	 * we only need set the TXEI IRQ, as TX/RX always happen syncronizely
	 */
	if (dws->dma_mapped) {
		ret = dws->dma_ops->dma_setup(dws, transfer);
		if (ret < 0) {
			spi_enable_chip(dws, 1);
			return ret;
		}
	}

	spi_enable_chip(dws, 1);

	if (dws->dma_mapped)
		return dws->dma_ops->dma_transfer(dws, transfer);
	else if (dws->irq == IRQ_NOTCONNECTED)
		return dw_spi_poll_transfer(dws, transfer);

	dw_spi_irq_setup(dws);

	return 1;
}

static void dw_spi_handle_err(struct spi_controller *master,
		struct spi_message *msg)
{
	struct dw_spi *dws = spi_controller_get_devdata(master);

	if (dws->dma_mapped)
		dws->dma_ops->dma_stop(dws);

	spi_reset_chip(dws);
}

/* yangguang: 
*  dwc_ssi_mem_ops: spi-mem exec_op impleamentation to support the spinand operations 
*  based on the spi-mem layer. The original dw_spi_transfer_one seems not working with 
*  spi-mem, when split the op to several transfer, it don't work.
*
*/

static int dw_spi_adjust_mem_op_size(struct spi_mem *mem, struct spi_mem_op *op)
{
	if (op->data.dir == SPI_MEM_DATA_IN)
		op->data.nbytes = clamp_val(op->data.nbytes, 0, SPI_NDF_MASK + 1);

	return 0;
}

static int dw_spi_init_mem_buf(struct dw_spi *dws, const struct spi_mem_op *op)
{
	if (op->data.dir == SPI_MEM_DATA_OUT) {
		dws->tx = op->data.buf.out;
		dws->tx_len = op->data.nbytes;
	} else {
		dws->tx = NULL;
		dws->tx_len = 0;
	}

	if (op->data.dir == SPI_MEM_DATA_IN) {
		dws->rx = op->data.buf.in;
		dws->rx_len = op->data.nbytes;
	} else {
		dws->rx = NULL;
		dws->rx_len = 0;
	}

	return 0;
}

static int dw_spi_write_then_read(struct dw_spi *dws, const struct spi_mem_op *op)
{
    u32 room, entries, sts;
	unsigned int len;
	u8 *buf;

	dw_write_io_reg(dws, DW_SPI_DR, op->cmd.opcode);
	
	if (op->cmd.buswidth > 1 || op->addr.buswidth > 1 || op->dummy.buswidth > 1 || op->data.buswidth > 1)
	{
		if (op->addr.nbytes)
			dw_write_io_reg(dws, DW_SPI_DR, op->addr.val);
		/*use the spi_ctrlr0 register rxdelay, so ignore dummy*/
		/*
		len = op->dummy.nbytes;
		if (op->data.dir != SPI_MEM_DATA_IN)
		{
			while (len)
			{
				dw_write_io_reg(dws, DW_SPI_DR, 0x00);
				len--;
			}
		}
		*/
	}
	else
	{
		len = op->addr.nbytes;
		while (len)
			dw_write_io_reg(dws, DW_SPI_DR, SPI_GET_BYTE(op->addr.val, --len));
		
		len = op->dummy.nbytes;
		while (len--)
			dw_write_io_reg(dws, DW_SPI_DR, 0x00);
	}
		
	/*
	* After setting any bit in the SER register the transmission will
	* start automatically. We have to keep up with that procedure
	* otherwise the CS de-assertion will happen whereupon the memory
	* operation will be pre-terminated.
	*/
	buf = dws->tx;
	if (dws->tx && dws->tx_len)
		len = dws->tx_len;
	else
		len = 0;

	/* fill the fifo remaining space first to avoid the fifo empty after starting the transfer */
	entries = readl_relaxed(dws->regs + DW_SPI_TXFLR);
	room = min(dws->fifo_len - entries, len);
	for (; room; --room, --len)
		dw_write_io_reg(dws, DW_SPI_DR, *buf++);

	dw_writel(dws, DW_SPI_SER, 0x01);

	while (len) {
		entries = readl_relaxed(dws->regs + DW_SPI_TXFLR);
		/*
		if (!entries) { 
			dev_err(&dws->master->dev, "CS de-assertion on Tx\n");
			return -EIO;
		}
		*/
		room = min(dws->fifo_len - entries, len);
		for (; room; --room, --len)
			dw_write_io_reg(dws, DW_SPI_DR, *buf++);
	}

	/*
	* Data fetching will start automatically if the EEPROM-read mode is
	* activated. We have to keep up with the incoming data pace to
	* prevent the Rx FIFO overflow causing the inbound data loss.
	*/

	buf = dws->rx;
	if (dws->rx && dws->rx_len)
		len = dws->rx_len;
	else
		len = 0;
	while (len) {
		entries = readl_relaxed(dws->regs + DW_SPI_RXFLR);
		if (!entries) {
			sts = readl_relaxed(dws->regs + DW_SPI_RISR);
			if (sts & SPI_INT_RXOI) {
				dev_err(&dws->master->dev, "FIFO overflow on Rx\n");
				//return -EIO;
			}
			continue;
		}
		entries = min(entries, len);
		for (; entries; --entries, --len)
			*buf++ = dw_read_io_reg(dws, DW_SPI_DR);
	}

    // waiting for fifo to be empty and idle
    //while ((dw_readl(dws, DW_SPI_SR) & 0x05) != 0x04);
	
    //dw_writel(dws, DW_SPI_SER, 0x00);

	return 0;
}

static void dw_spi_free_mem_buf(struct dw_spi *dws)
{
	return;
}

static inline bool dw_spi_ctlr_busy(struct dw_spi *dws)
{
	return dw_readl(dws, DW_SPI_SR) & SR_BUSY;
}

#define NSEC_PER_USEC	1000L
#define NSEC_PER_SEC	1000000000L

static int dw_spi_wait_mem_op_done(struct dw_spi *dws)
{
	int retry = 10*SPI_WAIT_RETRIES;
	struct spi_delay delay;
	unsigned long ns, us;
	u32 nents;

	nents = dw_readl(dws, DW_SPI_TXFLR);
	ns = NSEC_PER_SEC / dws->current_freq * nents;
	ns *= dws->n_bytes * BITS_PER_BYTE;
	if (ns <= NSEC_PER_USEC) {
		delay.unit = SPI_DELAY_UNIT_NSECS;
		delay.value = ns;
	} else {
		us = DIV_ROUND_UP(ns, NSEC_PER_USEC);
		delay.unit = SPI_DELAY_UNIT_USECS;
		delay.value = clamp_val(us, 0, USHRT_MAX);
	}

	//printk("nents: %d, ns:%lu, delay: %d, unit: %d\n", nents, ns, delay.value, delay.unit);

	while (dw_spi_ctlr_busy(dws) && retry--)
		spi_delay_exec(&delay, NULL);

	if (retry < 0) {
		dev_err(&dws->master->dev, "Mem op hanged up\n");
		return -EIO;
	}

	return 0;
}

static void dw_spi_stop_mem_op(struct dw_spi *dws, struct spi_device *spi)
{
	spi_enable_chip(dws, 0);
	//dw_spi_set_cs(spi, true);
	dw_writel(dws, DW_SPI_SER, 0x00);
	spi_enable_chip(dws, 1);
}

void dw_spi_update_freq(struct dw_spi *dws, struct spi_device *spi,
			u32 freq)
{
	struct chip_data *chip = spi_get_ctldata(spi);
	u32 speed_hz;
	u16 clk_div;

	/* Note DW APB SSI clock divider doesn't support odd numbers */
	clk_div = (DIV_ROUND_UP(dws->max_freq, freq) + 1) & 0xfffe;
	speed_hz = dws->max_freq / clk_div;
	
	if (dws->current_freq != speed_hz) {
		spi_set_clk(dws, clk_div);
		dws->current_freq = speed_hz;
		printk("dws->current_freq: %d, div: %d\n", dws->current_freq, clk_div);
	}
	
	//printk("mem_op max_freq: %d, freq: %d, current_freq: %d, speed_hz: %d, div: %d\n", dws->max_freq, freq, dws->current_freq, speed_hz, clk_div);
}

static int dwc_ssi_exec_mem_op(struct spi_mem *mem, const struct spi_mem_op *op)
{
    struct dw_spi *dws = spi_master_get_devdata(mem->spi->controller);
    struct chip_data *chip = spi_get_ctldata(mem->spi);
	unsigned long flags;
	int ret;
	uint32_t delay = 0;

    u32 cr0 = 0, _cr0 = 0;
    u16 div;
    u8 tmod = 0, dfs = 8;
	u32 ndf = 0;
	u32 freq;
	
	ret = dw_spi_init_mem_buf(dws, op);
	if (ret)
	{
		printk(KERN_ERR "fill dws buf failed\n");
		return ret;
	}

	/* CTRLR0[ 7: 6] Frame Format */
	cr0 |= chip->type << DWC_SSI_CTRLR0_FRF_OFFSET;

	/*
	* SPI mode (SCPOL|SCPH)
	* CTRLR0[ 8] Serial Clock Phase
	* CTRLR0[ 9] Serial Clock Polarity
	*/
	cr0 |= ((mem->spi->mode & SPI_CPOL) ? 1 : 0) << DWC_SSI_CTRLR0_SCPOL_OFFSET;
	cr0 |= ((mem->spi->mode & SPI_CPHA) ? 1 : 0) << DWC_SSI_CTRLR0_SCPH_OFFSET;

	/* CTRLR0[13] Shift Register Loop */
	cr0 |= ((mem->spi->mode & SPI_LOOP) ? 1 : 0) << DWC_SSI_CTRLR0_SRL_OFFSET;

	spi_enable_chip(dws, 0);

	freq = clamp(mem->spi->max_speed_hz, 0U, dws->max_freq);
	dw_spi_update_freq(dws, mem->spi, freq);
    
	tmod = SPI_TMOD_TR;
    if (SPI_MEM_DATA_IN == op->data.dir) {
        tmod =  SPI_TMOD_EPROMREAD;
		ndf = op->data.nbytes;
    }
    else {
        tmod = SPI_TMOD_TO;
    }

	if (op->cmd.buswidth > 1 || op->addr.buswidth > 1 || op->dummy.buswidth > 1 || op->data.buswidth > 1)
	{
		cr0 |= ((op->data.buswidth >> 1) << DWC_SSI_CTRLR0_SPI_FRF_OFFSET);

		if (SPI_MEM_DATA_IN == op->data.dir)
		{
			delay = 8 * op->dummy.nbytes / op->dummy.buswidth;
			_cr0 |= (delay << DWC_SSI_SPI_CTRLR0_WAIT_CYCLES_OFFSET);
			tmod = SPI_TMOD_RO;
		}

		if (op->cmd.buswidth == 1 && op->addr.buswidth > 1)
		{
			_cr0 |= (0x1 << DWC_SSI_SPI_CTRLR0_TRANS_TYPE_OFFSET);
		}
		else if (op->cmd.buswidth > 1 && op->addr.buswidth > 1)	
		{
			_cr0 |= (0x2 << DWC_SSI_SPI_CTRLR0_TRANS_TYPE_OFFSET);
		}
		
		_cr0 |= ((op->addr.nbytes * 2) << DWC_SSI_SPI_CTRLR0_ADDR_L_OFFSET);
		_cr0 |= (0x2 << DWC_SSI_SPI_CTRLR0_INST_L_OFFSET);
	}
	else
	{
		_cr0 = 0;
	}
	dw_writel(dws, DW_SPI_SPI_CTRLR0, _cr0);

	/* CTRLR0[11:10] Transfer Mode */
	cr0 |= (tmod << DWC_SSI_CTRLR0_TMOD_OFFSET);

	/* CTRLR0[ 4: 0] Data Frame Size */
	/*cr0 |= (mem->spi->bits_per_word - 1);*/
	cr0 |= (dfs - 1);
	dws->n_bytes = dfs / BITS_PER_BYTE;

    dw_writel(dws, DW_SPI_CTRL0, cr0);

	if (tmod == SPI_TMOD_EPROMREAD || tmod == SPI_TMOD_RO)
		dw_writel(dws, DW_SPI_CTRL1, ndf ? ndf - 1 : 0);

	dw_writel(dws, DW_SPI_RX_SAMPLE_DLY, 0x2);

	spi_mask_intr(dws, 0xff);

	spi_enable_chip(dws, 1);

    preempt_disable();    
    local_irq_save(flags);

	ret = dw_spi_write_then_read(dws, op);
	if(ret){
		local_irq_restore(flags);
		preempt_enable();
		return ret;
	}	
		

	local_irq_restore(flags);
	preempt_enable();

	if (!ret) {
		ret = dw_spi_wait_mem_op_done(dws);
		if (!ret)
			ret = dw_spi_check_status(dws, true);
	}

	dw_spi_stop_mem_op(dws, mem->spi);

    return 0;
}

static const struct spi_controller_mem_ops dwc_ssi_mem_ops = {
        .exec_op = dwc_ssi_exec_mem_op,
		.adjust_op_size = dw_spi_adjust_mem_op_size,
};

/* This may be called twice for each spi dev */
static int dw_spi_setup(struct spi_device *spi)
{
	struct dw_spi_chip *chip_info = NULL;
	struct chip_data *chip;
	int ret;

	/* Only alloc on first setup */
	chip = spi_get_ctldata(spi);
	if (!chip) {
		chip = kzalloc(sizeof(struct chip_data), GFP_KERNEL);
		if (!chip)
			return -ENOMEM;
		spi_set_ctldata(spi, chip);
	}

	/*
	 * Protocol drivers may change the chip settings, so...
	 * if chip_info exists, use it
	 */
	chip_info = spi->controller_data;

	/* chip_info doesn't always exist */
	if (chip_info) {
		if (chip_info->cs_control)
			chip->cs_control = chip_info->cs_control;

		chip->poll_mode = chip_info->poll_mode;
		chip->type = chip_info->type;
	}

	chip->tmode = SPI_TMOD_TR;

	if (gpio_is_valid(spi->cs_gpio)) {
		ret = gpio_direction_output(spi->cs_gpio,
				!(spi->mode & SPI_CS_HIGH));
		if (ret)
			return ret;
	}

	return 0;
}

static void dw_spi_cleanup(struct spi_device *spi)
{
	struct chip_data *chip = spi_get_ctldata(spi);

	kfree(chip);
	spi_set_ctldata(spi, NULL);
}

/* Restart the controller, disable all interrupts, clean rx fifo */
static void spi_hw_init(struct device *dev, struct dw_spi *dws)
{
	spi_reset_chip(dws);

	/*
	 * Try to detect the FIFO depth if not set by interface driver,
	 * the depth could be from 2 to 256 from HW spec
	 */
	if (!dws->fifo_len) {
		u32 fifo;

		for (fifo = 1; fifo < 256; fifo++) {
			dw_writel(dws, DW_SPI_TXFLTR, fifo);
			if (fifo != dw_readl(dws, DW_SPI_TXFLTR))
				break;
		}
		dw_writel(dws, DW_SPI_TXFLTR, 0);
		dw_writel(dws, DW_SPI_RXFLTR, 0x1F);

		dws->fifo_len = (fifo == 1) ? 0 : fifo;
		dev_dbg(dev, "Detected FIFO size: %u bytes\n", dws->fifo_len);
	}
}

int dw_spi_add_host(struct device *dev, struct dw_spi *dws)
{
	struct spi_controller *master;
	int ret;

	BUG_ON(dws == NULL);

	master = spi_alloc_master(dev, 0);
	if (!master)
		return -ENOMEM;

	dws->master = master;
	dws->type = SSI_MOTO_SPI;
	dws->dma_inited = 0;
	dws->dma_addr = (dma_addr_t)(dws->paddr + DW_SPI_DR);

    /*yangguang: call spi_controller_set_devdata before request_irq to avoid race condition in interrupt context*/
	spi_controller_set_devdata(master, dws); 
	ret = request_irq(dws->irq, dw_spi_irq, IRQF_SHARED, dev_name(dev),
			  master);
	if (ret < 0) {
		dev_err(dev, "can not get IRQ\n");
		goto err_free_master;
	}

	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_LOOP;

	switch (dws->reg_io_width)
	{
	case 2:
		master->mode_bits |= SPI_TX_DUAL | SPI_RX_DUAL;
		break;
	case 4:
		master->mode_bits |= SPI_TX_QUAD | SPI_RX_QUAD;
		break;
	case 8:
		master->mode_bits |= SPI_TX_OCTAL | SPI_RX_OCTAL;
		break;
	default:
		break;
	}

	master->bits_per_word_mask = SPI_BPW_MASK(8) | SPI_BPW_MASK(16) | SPI_BPW_MASK(32);
	master->bus_num = dws->bus_num;
	master->num_chipselect = dws->num_cs;
	master->setup = dw_spi_setup;
	master->cleanup = dw_spi_cleanup;
	master->set_cs = dw_spi_set_cs;
	master->transfer_one = dw_spi_transfer_one;
	master->handle_err = dw_spi_handle_err;
	master->max_speed_hz = dws->max_freq;
	master->dev.of_node = dev->of_node;
	master->flags = SPI_MASTER_GPIO_SS;

        /*add the mem_ops to support spinand for dwc_ssi with spi-mem layer */
        master->mem_ops = &dwc_ssi_mem_ops;

	/* Basic HW init */
	spi_hw_init(dev, dws);

	if (dws->dma_ops && dws->dma_ops->dma_init) {
		ret = dws->dma_ops->dma_init(dev, dws);
		if (ret) {
			dev_warn(dev, "DMA init failed\n");
			dws->dma_inited = 0;
		} else {
			master->can_dma = dws->dma_ops->can_dma;
		}
	}
        
	ret = devm_spi_register_controller(dev, master);
	if (ret) {
		dev_err(&master->dev, "problem registering spi master\n");
		goto err_dma_exit;
	}

	dw_spi_debugfs_init(dws);
	return 0;

err_dma_exit:
	if (dws->dma_ops && dws->dma_ops->dma_exit)
		dws->dma_ops->dma_exit(dws);
	spi_enable_chip(dws, 0);
	free_irq(dws->irq, master);
err_free_master:
	spi_controller_put(master);
	return ret;
}
EXPORT_SYMBOL_GPL(dw_spi_add_host);

void dw_spi_remove_host(struct dw_spi *dws)
{
	dw_spi_debugfs_remove(dws);

	if (dws->dma_ops && dws->dma_ops->dma_exit)
		dws->dma_ops->dma_exit(dws);

	spi_shutdown_chip(dws);

	free_irq(dws->irq, dws->master);
}
EXPORT_SYMBOL_GPL(dw_spi_remove_host);

int dw_spi_suspend_host(struct dw_spi *dws)
{
	int ret;

	ret = spi_controller_suspend(dws->master);
	if (ret)
		return ret;

	spi_shutdown_chip(dws);
	return 0;
}
EXPORT_SYMBOL_GPL(dw_spi_suspend_host);

int dw_spi_resume_host(struct dw_spi *dws)
{
	int ret;

	spi_hw_init(&dws->master->dev, dws);
	ret = spi_controller_resume(dws->master);
	if (ret)
		dev_err(&dws->master->dev, "fail to start queue (%d)\n", ret);
	return ret;
}
EXPORT_SYMBOL_GPL(dw_spi_resume_host);

MODULE_AUTHOR("Feng Tang <feng.tang@intel.com>");
MODULE_DESCRIPTION("Driver for DesignWare SPI controller core");
MODULE_LICENSE("GPL v2");
