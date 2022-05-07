/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

//#define DEBUG
//#define VERBOSE_DEBUG

#include <linux/bitmap.h>
#include <linux/delay.h>
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/dmaengine.h>
#include <linux/dmapool.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_dma.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/delay.h>

#include "virt-dma.h"


/** Common macros to  PERIDMA registers **/

/** DMA register offsets **/

/* Global register offsets */
#define PDMA_INT_RAW0		0x0
#define PDMA_INT_RAW1		0x4
#define PDMA_INT_EN0		0x8
#define PDMA_INT_EN1		0xc
#define PDMA_INT_STAT0		0x10
#define PDMA_INT_STAT1		0x14


/* Channel register offsets */
#define CH_CTL			0x0
#define CH_CFG0			0x4
#define CH_CFG1			0x8
#define CH_SADDR		0xc
#define CH_DADDR		0x10
#define CH_DEV_TOUT_OFF		0x14
#define CH_STAT			0x18

#define CH_OFF			0x20
#define CH0_BASE		0x20

#define CH_NUM			16

/**   PDMA_INT_RAW0 fields **/
//#define CHX_TR_DONE_SHIFT	0x0
//#define CHX_HALF_DONE_SHIFT	0x10

//#define CHX_TR_DONE(ch)		((ch) & 0xffff)
//#define CHX_HALF_DONE(ch)	(((ch) & 0xffff) << 16)
#define CHX_TR_DONE(ch)		BIT(ch)
#define CHX_HALF_DONE(ch)	BIT(ch+16)

/**   PDMA_INT_RAW1 fields **/
//#define CHX_DEV_TOUT_SHIFT	0x0

//#define CHX_DEV_TOUT(ch)	((ch) & 0xffff)
#define CHX_DEV_TOUT(ch)	BIT(ch)

/**   PDMA_INT_EN0 fields  **/
//#define CHX_TR_DONE_EN_SHIFT	0x0
//#define CHX_HALF_DONE_EN_SHIFT	0x10


/**   PDMA_INT_EN1 fields  **/
//#define CHX_DEV_TOUT_EN_SHIFT	0x0


#define CH_INT_MASK		0xffff

/**    CH_CTL              **/
//#define CH_START		0x0
//#define WE_CH_START		0x1
//#define CH_DAT_ENDIAN		0x2
//#define CH_DAT_ENDIAN_MASK	(0x3 << 2)
//#define CH_STOP			0x4
//#define WE_CH_STOP		0x5
//#define CH_PAUSE		0x8
//#define WE_CH_PAUSE		0x9

#define MAX_LINE_SIZE		0xffff
#define MAX_LINE_NUM		0x3ff
#define RECT_MAX_TRANS_LEN		(MAX_LINE_SIZE*MAX_LINE_NUM)

#define CH_START		BIT(0)
#define WE_CH_START		BIT(1)
#define CH_DAT_ENDIAN(endian)	(((endian) & 0x3) << 2)
#define CH_STOP			BIT(4)
#define WE_CH_STOP		BIT(5)
#define CH_PAUSE		BIT(8)
#define WE_CH_PAUSE		BIT(9)


/**    CH_CFG0            **/
//#define CH_SRC_TYPE_SHIFT	0
//#define CH_SRC_TYPE_MEM		0x0
//#define CH_SRC_TYPE_DEV		0x1
//#define CH_DEV_HSIZE_SHIFT	2
//#define CH_DEV_HSIZE_ONE_BYTE	0x0		
//#define CH_DEV_HSIZE_TWO_BYTE	0x1		
//#define CH_DEV_HSIZE_THREE_BYTE	0x2
//#define CH_TRAN_MODE_SHIFT	4
//#define CH_TRAN_MODE_LINE	0x0
//#define CH_TRAN_MODE_RECT	0x1
//#define CH_PRIORITY_SHIFT	8
//#define CH_PRIORITY_MASK	0xf
//#define CH_DEV_BLEN_SHIFT	12
//#define CH_DEV_BLENMASK		0xf
//#define CH_LINE_NUM_SHIFT	16
//#define CH_LINE_NUM_MASK	0x3ff

#define CH_SRC_TYPE(type)	((type) & 0x1)
#define CH_DEV_HSIZE(hsize)	(((hsize) & 0x3) << 2)
#define CH_TRAN_MODE(mode)	(((mode) & 0x1) << 4)
#define CH_PRIORITY(prio)	(((prio) & 0xf) << 8)
#define CH_DEV_BLEN(blen)	(((blen) & 0xf) << 12)
#define CH_LINE_NUM(num)	(((num) & 0x3ff) << 16)



/**    CH_CFG1             **/
//#define CH_LINE_SIZE_SHIFT	0
//#define CH_LINE_SIZE_MASK	0xffff
//#define CH_LINE_SPACING_SHIFT	16
//#define CH_LINE_SPACING_MASK	0xffff

#define CH_LINE_SIZE(size)	((size) & 0xffff)
#define CH_LINE_SPACE(space)	((space) << 16)

/**    CH_DEV_TOUT         **/
//#define CH_DEV_TOUT_SHIFT	0
//#define CH_DEV_TOUT_MASK	0xfff

#define CH_DEV_TOUT(cycles)	((cycles) & 0xfff)
#define CH_DEV_TOUT_DEF		0x20
#define CH_DEV_TOUT_MIN		0x8

/**    CH_STAT             **/
//#define CH_STAT_IDLE		0x1
//#define CH_STAT_PAUSE		0x10
//#define CH_STAT_BUSY		0x100

#define CH_STAT_IDLE		BIT(0)
#define CH_STAT_PAUSE		BIT(4)
#define CH_STAT_BUSY		BIT(8)

/**    CH_PERI_DEV_SEL     **/
#define CH_PERI_DEV_SEL_OFF	0x60


typedef enum ch_peri_dev_sel {
    UART0_TX,
    UART0_RX,
    UART1_TX,
    UART1_RX,
    UART2_TX,
    UART2_RX,
    SPI0_TX,
    SPI0_RX,
    SPI1_TX,
    SPI1_RX,
    SPI2_TX,
    SPI2_RX,
    SPI3_TX,
    SPI3_RX,
    I2S0_TX,
    I2S0_RX,
    I2S1_TX,
    I2S1_RX,
    I2S2_TX,
    I2S2_RX,
    I2C0_TX,
    I2C0_RX,
    I2C1_TX,
    I2C1_RX,
    I2C2_TX,
    I2C2_RX,
    AUDIF_PDM_RX,
    AUDIF_TDM_RX,
    AUDIF_PDM_8CH_TX,
    AUDIF_REF_TX,
    AUDIF_AUD_TX,
    VAD_RX,
    SHA_TX,
    AES_TX,
    AES_RX,
    PERI_DEV_SEL_MAX
} ch_peri_dev_sel_t;

typedef enum peridma_mode {
    LINE_MODE,
    RECT_MODE,
    DMA_MODE_MAX
} peridma_mode_t;

typedef enum ch_src_type {
    SRC_TYPE_MEM,
    SRC_TYPE_DEV,
    SRC_TYPE_MAX
} ch_src_type_t;

typedef enum ch_dat_endian {
    DAT_ENDIAN_DEF,
    DAT_ENDIAN_TWO_BYTE,
    DAT_ENDIAN_FOUR_BYTE,
    DAT_ENDIAN_EIGHT_BYTE,
    DAT_ENDIAN_MAX
} ch_dat_endian_t;

typedef enum ch_dev_hsize {
    DEV_HSIZE_ONE_BYTE,
    DEV_HSIZE_TWO_BYTE,
    DEV_HSIZE_FOUR_BYTE,
    DEV_HSIZE_INVALID
} ch_dev_hsize_t;

/** DMA Driver **/
struct k510_peridma_hwdesc {
    peridma_mode_t                  trans_mode;
    u32                             line_num;
    u32                             line_size;
    u32                             line_space;
    u32				ctl;
    u32				cfg0;
    u32				cfg1;
    u32				dev_tout;
    u32				dev_sel;
    dma_addr_t                      dst_addr;
    dma_addr_t                      src_addr;
    bool             last;
    struct list_head		list;
};

struct k510_peridma_desc {
    struct virt_dma_desc            vd;
    struct k510_peridma_vchan	*vchan;
    struct list_head                xfer_list;
    struct list_head                completed_list;
    struct k510_peridma_hwdesc	*hwdesc;
    atomic_t			hwdesc_remain;
    bool				next;
    struct k510_peridma_hwdesc *cyclic;
    u32 remain;
    u32 buf_len;
};


struct k510_peridma_vchan {
        struct k510_peridma_dev		*pdev;
	struct virt_dma_chan		vchan;
	struct dma_slave_config		cfg;
	struct k510_peridma_desc	*desc;
	struct k510_peridma_hwdesc	*cur_hwdesc;
	atomic_t			descs_allocated;
	struct k510_peridma_pchan	*pchan;
	enum   dma_transfer_direction 	dir;
	dma_addr_t			dev_addr;
	u32				priority;
	u32				dev_tout;
	ch_dat_endian_t			dat_endian;
	ch_peri_dev_sel_t		dev_sel;
	enum dma_status			status;
        bool                            is_paused;
        
};

struct k510_peridma_pchan {
        struct k510_peridma_dev         *pdev;
        /* Register base of channel */
        void __iomem                    *ch_regs;
        u8                              id;
        struct k510_peridma_vchan	*vchan;
        bool                            is_paused;

};

struct k510_peridma_dev {
	DECLARE_BITMAP(pchans_used, CH_NUM);
        struct device			*dev;
	struct dma_device		slave;
	struct k510_peridma_vchan	*vchan;
	struct k510_peridma_pchan	*pchan;
        u32				nr_channels;
        u32				nr_requests;
	void __iomem			*base;
	struct clk			*clk;
	int				irq;
	spinlock_t			lock;
};

static void free_pchan(struct k510_peridma_dev *pdev, struct k510_peridma_pchan *pchan);

static struct k510_peridma_dev *to_k510_peridma_dev(struct dma_device *dev)
{
	return container_of(dev, struct k510_peridma_dev, slave);
}

static struct k510_peridma_vchan *to_k510_peridma_vchan(struct dma_chan *chan)
{
        return container_of(chan, struct k510_peridma_vchan, vchan.chan);
}

static struct device *vchan2dev(struct k510_peridma_vchan *vchan)
{
	return &vchan->vchan.chan.dev->device;
}

static inline struct k510_peridma_desc *vd_to_k510_peridma_desc(struct virt_dma_desc *vd)
{
        return container_of(vd, struct k510_peridma_desc, vd);
}

static inline struct k510_peridma_vchan *vc_to_k510_peridma_vchan(struct virt_dma_chan *vc)
{
        return container_of(vc, struct k510_peridma_vchan, vchan);
}

static inline struct k510_peridma_vchan *dchan_to_k510_peridma_vchan(struct dma_chan *dchan)
{
        return vc_to_k510_peridma_vchan(to_virt_chan(dchan));
}

static inline const char *peridma_vchan_name(struct k510_peridma_vchan *vchan)
{
        return dma_chan_name(&vchan->vchan.chan);
}

static inline bool peridma_pchan_is_idle(struct k510_peridma_pchan *pchan)
{
        u32 val;

        val = ioread32(pchan->ch_regs + CH_STAT);

        return (val & CH_STAT_IDLE);
}

static inline bool peridma_pchan_is_busy(struct k510_peridma_pchan *pchan)
{
        u32 val;

        val = ioread32(pchan->ch_regs + CH_STAT);

        return (val & CH_STAT_BUSY);
}

static inline bool peridma_pchan_is_paused(struct k510_peridma_pchan *pchan)
{
        u32 val;

        val = ioread32(pchan->ch_regs + CH_STAT);

        return (val & CH_STAT_PAUSE);
}

static void k510_peridma_free_chan_resources(struct dma_chan *chan)
{
	struct k510_peridma_vchan *pvchan = to_k510_peridma_vchan(chan);

	vchan_free_chan_resources(&pvchan->vchan);
}

static inline void peridma_pchan_irq_clear(struct k510_peridma_pchan *pchan)
{
        struct k510_peridma_dev *pdev = pchan->pdev;
        u32 reg;
        u32 ch_id = pchan->id;

        reg = ioread32(pdev->base + PDMA_INT_RAW0);
        reg |= CHX_TR_DONE(ch_id);
//        reg |= CHX_HALF_DONE(ch_id);

        iowrite32(reg, pdev->base + PDMA_INT_RAW0);

        reg = ioread32(pdev->base + PDMA_INT_RAW1);
        reg |= CHX_DEV_TOUT(ch_id);

        iowrite32(reg, pdev->base + PDMA_INT_RAW1);
}

static inline void peridma_pchan_irq_enable(struct k510_peridma_pchan *pchan)
{
	struct k510_peridma_dev *pdev = pchan->pdev;
        u32 reg;
	u32 ch_id = pchan->id;
//        struct k510_peridma_desc *desc;
//        struct virt_dma_desc *vd;

//        vd = vchan_next_desc(&vchan->vchan);
//        if (!vd)
//                return;

//        desc = vd_to_k510_peridma_desc(vd);

        reg = ioread32(pdev->base + PDMA_INT_EN0);
        reg |= CHX_TR_DONE(ch_id);
//        reg |= CHX_HALF_DONE(ch_id);

        iowrite32(reg, pdev->base + PDMA_INT_EN0);

        reg = ioread32(pdev->base + PDMA_INT_EN1);
        reg |= CHX_DEV_TOUT(ch_id);

        iowrite32(reg, pdev->base + PDMA_INT_EN1);
}

static inline void peridma_pchan_irq_disable(struct k510_peridma_pchan *pchan)
{
	struct k510_peridma_dev *pdev = pchan->pdev;
	u32 reg;
	u32 ch_id = pchan->id;

        reg = ioread32(pdev->base + PDMA_INT_EN0);
        reg &= ~CHX_TR_DONE(ch_id);
//        reg &= ~CHX_HALF_DONE(ch_id);
	
	iowrite32(reg, pdev->base + PDMA_INT_EN0);

        reg = ioread32(pdev->base + PDMA_INT_EN1);
        reg &= ~CHX_DEV_TOUT(ch_id);
 
        iowrite32(reg, pdev->base + PDMA_INT_EN0);
}

static inline void peridma_pchan_start(struct k510_peridma_pchan *pchan)
{
        u32 reg = 0;

//        reg = ioread32(pchan->ch_regs + CH_CTL);
        reg |= CH_START;
        reg |= WE_CH_START;

        iowrite32(reg, pchan->ch_regs + CH_CTL);
}

static inline void peridma_pchan_stop(struct k510_peridma_pchan *pchan)
{
        u32 reg = 0;
 
//	reg = ioread32(pchan->ch_regs + CH_CTL);
        reg |= CH_STOP;
        reg |= WE_CH_STOP;

        iowrite32(reg, pchan->ch_regs + CH_CTL);
}

static inline void peridma_pchan_pause(struct k510_peridma_pchan *pchan)
{
        u32 reg;

        reg = ioread32(pchan->ch_regs + CH_CTL);
        reg |= CH_PAUSE;
        reg |= WE_CH_PAUSE;

        iowrite32(reg, pchan->ch_regs + CH_CTL);
}

static inline void peridma_pchan_resume(struct k510_peridma_pchan *pchan)
{
        u32 reg;

        reg = ioread32(pchan->ch_regs + CH_CTL);
        reg &= ~(CH_PAUSE);
        reg |= WE_CH_PAUSE;

        iowrite32(reg, pchan->ch_regs + CH_CTL);
}

static void k510_peridma_hw_init(struct k510_peridma_dev *pdev)
{
        u32 i;

        iowrite32(0, pdev->base + PDMA_INT_EN0);
        iowrite32(0, pdev->base + PDMA_INT_EN1);
        iowrite32(0xFFFFFFFF, pdev->base + PDMA_INT_RAW0);
        iowrite32(0xFFFFFFFF, pdev->base + PDMA_INT_RAW1);

        for (i = 0; i < pdev->nr_channels; i++) {
                peridma_pchan_stop(&pdev->pchan[i]);
        }
}

static void peridma_desc_put(struct k510_peridma_desc *desc)
{
        struct k510_peridma_vchan *vchan = desc->vchan;
        unsigned int descs_put = 1;
        struct k510_peridma_hwdesc *hwdesc, *tmp;

//	vchan->desc = NULL;

        list_for_each_entry_safe(hwdesc, tmp, &desc->xfer_list, list)
                kfree(hwdesc);
        if(!desc->cyclic)
        {
            list_for_each_entry_safe(hwdesc, tmp, &desc->completed_list, list)
                    kfree(hwdesc);
        }

	kfree(desc);

        atomic_sub(descs_put, &vchan->descs_allocated);
        dev_vdbg(vchan2dev(vchan), ": %d descs put, %d still allocated\n",
                descs_put,
                atomic_read(&vchan->descs_allocated));
}

static void vchan_desc_put(struct virt_dma_desc *vdesc)
{
        peridma_desc_put(vd_to_k510_peridma_desc(vdesc));
}

static ch_dev_hsize_t slave_buswidth_to_hsize(enum dma_slave_buswidth buswidth)
{
    ch_dev_hsize_t hsize = DEV_HSIZE_INVALID;    

    switch (buswidth) {
    case DMA_SLAVE_BUSWIDTH_1_BYTE:
        hsize = DEV_HSIZE_ONE_BYTE;
        break;

    case DMA_SLAVE_BUSWIDTH_2_BYTES:
        hsize = DEV_HSIZE_TWO_BYTE;
        break;

    case DMA_SLAVE_BUSWIDTH_4_BYTES:
        hsize = DEV_HSIZE_FOUR_BYTE;
        break;

    default:
        hsize = DEV_HSIZE_INVALID;
        break;
    }

    return hsize;
}

static struct k510_peridma_hwdesc *generate_hwdesc(
	struct k510_peridma_vchan *vchan, dma_addr_t src, dma_addr_t dest,
	size_t len, struct dma_slave_config *sconfig, enum dma_transfer_direction dir)
{
	struct k510_peridma_hwdesc *hwdesc = NULL;
	ch_dev_hsize_t hsize = DEV_HSIZE_INVALID;
	
	hwdesc = kzalloc(sizeof(*hwdesc), GFP_NOWAIT);
	if(!hwdesc){
		dev_err(vchan2dev(vchan), "alloc hwdesc failure");
		return NULL;
	}

	hwdesc->src_addr = src;
	hwdesc->dst_addr = dest;
	hwdesc->ctl = CH_DAT_ENDIAN(vchan->dat_endian);

        if (dir == DMA_MEM_TO_DEV) {
		hsize = slave_buswidth_to_hsize(sconfig->dst_addr_width);
                if(hsize == DEV_HSIZE_INVALID)
			goto err_exit;
//		hwdesc->cfg0 = CH_SRC_TYPE(SRC_TYPE_MEM) | CH_DEV_BLEN(sconfig->dst_maxburst - 1);
		hwdesc->cfg0 = CH_SRC_TYPE(SRC_TYPE_MEM) | CH_DEV_BLEN(0);
        } else if(dir == DMA_DEV_TO_MEM){
                hsize = slave_buswidth_to_hsize(sconfig->src_addr_width);
                if(hsize == DEV_HSIZE_INVALID)
                        goto err_exit;
//		hwdesc->cfg0 = CH_SRC_TYPE(SRC_TYPE_DEV) | CH_DEV_BLEN(sconfig->src_maxburst - 1);
		hwdesc->cfg0 = CH_SRC_TYPE(SRC_TYPE_DEV) | CH_DEV_BLEN(0);
        }

//        hwdesc->cfg0 |= CH_TRAN_MODE(LINE_MODE) | CH_PRIORITY(vchan->priority);
        hwdesc->cfg0 |= CH_TRAN_MODE(LINE_MODE) | CH_PRIORITY(vchan->priority) | CH_DEV_HSIZE(hsize);

        hwdesc->cfg1 = CH_LINE_SIZE(len);
        hwdesc->dev_tout = CH_DEV_TOUT(vchan->dev_tout);
        hwdesc->dev_sel = vchan->dev_sel;

	dev_vdbg(vchan2dev(vchan), "generate hwdesc(%px): ctl=%x, cfg0=%x, cfg1=%x, dev_tout=%x, dev_sel=%x, src_addr=%x, dst_addr=%x \n", hwdesc, hwdesc->ctl, hwdesc->cfg0, hwdesc->cfg1, hwdesc->dev_tout, hwdesc->dev_sel, hwdesc->src_addr, hwdesc->dst_addr);

        return hwdesc;

err_exit:
    kfree(hwdesc);
    return NULL;
	
}

static struct dma_async_tx_descriptor *k510_peridma_prep_slave_sg(
	struct dma_chan *chan, struct scatterlist *sgl, unsigned int sglen,
	enum dma_transfer_direction dir, unsigned long flags, void *context)
{
        struct k510_peridma_desc *desc = NULL;
	struct k510_peridma_hwdesc *hwdesc = NULL;
	struct k510_peridma_hwdesc *tmp = NULL;
        struct k510_peridma_vchan *vchan = dchan_to_k510_peridma_vchan(chan);
	struct dma_slave_config *sconfig = &vchan->cfg;
	struct scatterlist *sg;
        size_t xfer_size, avail, total = 0;
	dma_addr_t addr, src = 0, dst = 0;
	int i = 0;

        dev_vdbg(vchan2dev(vchan), ": prep_slave_sg: sgl: %px sglen: %zd flags: %#lx",
                sgl, sglen, flags);
//        printk("k510_peridma_prep_slave_sg: sgl: %px sglen: %zd flags: %#lx",
//                sgl, sglen, flags);

	desc = kzalloc(sizeof(struct k510_peridma_desc), GFP_NOWAIT);

	if(!desc)
		return NULL;

	desc->vchan = vchan;
//	vchan->desc = desc;
	INIT_LIST_HEAD(&desc->xfer_list);
	INIT_LIST_HEAD(&desc->completed_list);
//	atomic_set(&desc->hwdesc_remain, 0);
	atomic_inc(&vchan->descs_allocated);
//	desc->next = false;


	for_each_sg(sgl, sg, sglen, i) {
		addr = sg_dma_address(sg);
		avail = sg_dma_len(sg);

		do {
			xfer_size = min_t(size_t, avail, MAX_LINE_SIZE);
			
			if (dir == DMA_MEM_TO_DEV) {
                                src = addr;
                                dst = sconfig->dst_addr;
                        } else if (dir == DMA_DEV_TO_MEM) {
                                src = sconfig->src_addr;
                                dst = addr;
                        }
	
			dev_vdbg(vchan2dev(vchan), "dma_async_tx_descriptor: src=%x, dst=%x, xfer_size=%x \n", src, dst, xfer_size);

			hwdesc = generate_hwdesc(vchan, src, dst, xfer_size, sconfig, dir);
			if(hwdesc)
				list_add_tail(&hwdesc->list, &desc->xfer_list);
			else {
				list_for_each_entry_safe(hwdesc, tmp, &desc->xfer_list, list)
					kfree(hwdesc);

				kfree(desc);
				dev_err(vchan2dev(vchan), "dma_async_tx_descriptor: generate hwdesc failure \n");
				return NULL;
			}
//			atomic_inc(&desc->hwdesc_remain);

			addr += xfer_size;
			avail -= xfer_size; 

		}while(avail);

	}

	dev_vdbg(vchan2dev(vchan), "dma_async_tx_descriptor: prepare desc(%px) \n", desc);

        return vchan_tx_prep(&vchan->vchan, &desc->vd, flags);
}

static struct dma_async_tx_descriptor *k510_peridma_prep_dma_cyclic(
	struct dma_chan *chan, dma_addr_t buf_addr, size_t buf_len,
	size_t period_len, enum dma_transfer_direction dir,
	unsigned long flags)
{
    struct k510_peridma_desc *desc = NULL;
    struct k510_peridma_hwdesc *hwdesc = NULL;
    struct k510_peridma_hwdesc *tmp = NULL;
    struct k510_peridma_vchan *vchan = dchan_to_k510_peridma_vchan(chan);
    struct dma_slave_config *sconfig = &vchan->cfg;
    struct scatterlist *sg;
    size_t xfer_size, avail, total = 0;
    u32 num_periods;
    dma_addr_t addr, src = 0, dst = 0;
    int i = 0;

    desc = kzalloc(sizeof(struct k510_peridma_desc), GFP_NOWAIT);

    if(!desc)
        return NULL;

    desc->vchan = vchan;
    desc->buf_len = buf_len;
    desc->remain = buf_len;
    INIT_LIST_HEAD(&desc->xfer_list);

    atomic_inc(&vchan->descs_allocated);

    num_periods = buf_len / period_len;
    for(i = 0; i < num_periods; i++)
    {
        addr = buf_addr + period_len*i;
        avail = period_len;

        do {
            xfer_size = min_t(size_t, avail, MAX_LINE_SIZE);

            if (dir == DMA_MEM_TO_DEV) {
                src = addr;
                dst = sconfig->dst_addr;
            } else if (dir == DMA_DEV_TO_MEM) {
                src = sconfig->src_addr;
                dst = addr;
            }

            dev_vdbg(vchan2dev(vchan),  "dma_async_tx_descriptor: src=%x, dst=%x, xfer_size=%x \n", src, dst, xfer_size);

            hwdesc = generate_hwdesc(vchan, src, dst, xfer_size, sconfig, dir);
            if(hwdesc)
            {
                list_add_tail(&hwdesc->list, &desc->xfer_list);
                if(desc->cyclic == NULL)
                    desc->cyclic = hwdesc;
            }
            else {
                list_for_each_entry_safe(hwdesc, tmp, &desc->xfer_list, list)
                kfree(hwdesc);

                kfree(desc);
                dev_vdbg(vchan2dev(vchan), "dma_async_tx_descriptor: generate hwdesc failure \n");
                return NULL;
            }

            addr += xfer_size;
            avail -= xfer_size; 
            if(avail == 0)
                hwdesc->last = true;
        } while(avail);
    }

    return vchan_tx_prep(&vchan->vchan, &desc->vd, flags);
}

static int k510_peridma_terminate_all(struct dma_chan *dchan)
{
        struct k510_peridma_vchan *vchan = dchan_to_k510_peridma_vchan(dchan);
        unsigned long flags;
        LIST_HEAD(head);

        spin_lock_irqsave(&vchan->vchan.lock, flags);

	if(vchan->pchan){
		dev_vdbg(vchan2dev(vchan), "k510_peridma_terminate_all: release pchan %x \n", vchan->pchan->id);
		peridma_pchan_irq_disable(vchan->pchan);
		peridma_pchan_stop(vchan->pchan);
		free_pchan(vchan->pdev,vchan->pchan);
		vchan->pchan = NULL;
	} else
		dev_vdbg(vchan2dev(vchan), "k510_peridma_terminate_all: vchan->pchan is NULL \n");

	if(vchan->desc){
//		vchan_terminate_vdesc(&vchan->desc->vd);
		vchan->desc = NULL;
	}
	
	vchan->is_paused = false;

        vchan_get_all_descriptors(&vchan->vchan, &head);

        /*
         * As vchan_dma_desc_free_list can access to desc_allocated list
         * we need to call it in vc.lock context.
         */
        vchan_dma_desc_free_list(&vchan->vchan, &head);

        spin_unlock_irqrestore(&vchan->vchan.lock, flags);

        dev_vdbg(vchan2dev(vchan), "terminated \n");

        return 0;
}

// 无论是cycle还是sg类型的buffer，
// 从desc的第一个hwdesc节点开始遍历到最后一个hwdesc，得到剩余下的需要传输数据长度
static size_t k510_peridma_desc_residue(struct k510_peridma_vchan *chan,
				     struct k510_peridma_desc *desc)
{
	struct k510_peridma_hwdesc *hwdesc = NULL;
	u32 residue = 0;
        residue = desc->remain;

	return residue;
}

static enum dma_status k510_peridma_tx_status(struct dma_chan *dchan,
					   dma_cookie_t cookie,
					   struct dma_tx_state *txstate)
{
        struct k510_peridma_vchan *vchan = dchan_to_k510_peridma_vchan(dchan);
        enum dma_status ret;
        struct virt_dma_desc *vdesc;
        unsigned long flags;
	    u32 residue = 0;

        ret = dma_cookie_status(dchan, cookie, txstate);
        if (ret == DMA_COMPLETE)
		    return ret;

        spin_lock_irqsave(&vchan->vchan.lock, flags);
        ret = dma_cookie_status(dchan, cookie, txstate);
        if (ret != DMA_COMPLETE) 
        {
            vdesc = vchan_find_desc(&vchan->vchan, cookie);
            if (vdesc)
            {
                residue = k510_peridma_desc_residue(vchan,
                                vd_to_k510_peridma_desc(vdesc));
            }
        }
        spin_unlock_irqrestore(&vchan->vchan.lock, flags);
        dma_set_residue(txstate, residue);

        if (vchan->is_paused && ret == DMA_IN_PROGRESS)
            ret = DMA_PAUSED;
        return ret;
}


static void free_pchan(struct k510_peridma_dev *pdev, struct k510_peridma_pchan *pchan)
{
        unsigned long flags;
        int nr = pchan - pdev->pchan;
	struct k510_peridma_vchan *vchan;

        spin_lock_irqsave(&pdev->lock, flags);
	
	vchan = pchan->vchan;
	vchan->pchan = NULL;
        pchan->vchan = NULL;
        clear_bit(nr, pdev->pchans_used);

        spin_unlock_irqrestore(&pdev->lock, flags);

}

static struct k510_peridma_pchan *find_phy_chan(struct k510_peridma_dev *pdev, struct k510_peridma_vchan *vchan)
{
        struct k510_peridma_pchan *pchan = NULL, *pchans = pdev->pchan;
        unsigned long flags;
        int i=0, ch_num;

        spin_lock_irqsave(&pdev->lock, flags);
        for_each_clear_bit_from(i, pdev->pchans_used, CH_NUM) {
                pchan = &pchans[i];
                pchan->vchan = vchan;
                set_bit(i, pdev->pchans_used);
                break;
        }
        spin_unlock_irqrestore(&pdev->lock, flags);

        return pchan;
}

static void configure_pchan(struct k510_peridma_pchan *pchan, struct k510_peridma_hwdesc *hwdesc)
{
	struct k510_peridma_dev *pdev = pchan->pdev;
	unsigned long flags;

        if (unlikely(!peridma_pchan_is_idle(pchan))) {
                dev_err(vchan2dev(pchan->vchan), "pchan %d is non-idle!\n",
                        pchan->id);

                return;
        }

	iowrite32(hwdesc->ctl, pchan->ch_regs+CH_CTL);
        iowrite32(hwdesc->cfg0, pchan->ch_regs+CH_CFG0);
        iowrite32(hwdesc->cfg1, pchan->ch_regs+CH_CFG1);
        iowrite32(hwdesc->dev_tout, pchan->ch_regs+CH_DEV_TOUT_OFF);
        iowrite32(hwdesc->dst_addr, pchan->ch_regs+CH_DADDR);
        iowrite32(hwdesc->src_addr, pchan->ch_regs+CH_SADDR);
        iowrite32(hwdesc->dev_sel, (pchan->ch_regs + (CH_NUM - pchan->id -1)*CH_OFF + CH_PERI_DEV_SEL_OFF));

}

static void peridma_vchan_start_first_queued(struct k510_peridma_vchan *vchan)
{
        struct k510_peridma_desc *desc;
        struct virt_dma_desc *vd;
        struct k510_peridma_dev *pdev = vchan->pdev;
        struct k510_peridma_pchan *pchan = NULL;
        struct k510_peridma_hwdesc *hwdesc = NULL;
	unsigned long flags;


        if(vchan->desc) {
                dev_err(vchan2dev(vchan), "%s already processing something \n", peridma_vchan_name(vchan));
		dump_stack();
                return;
        }

        if(vchan->pchan) {
		pchan = vchan->pchan;
                dev_vdbg(vchan2dev(vchan), "%s already allocated phy channel \n", peridma_vchan_name(vchan));
        } else {
                pchan = find_phy_chan(pdev, vchan);
                if(pchan) 
                        vchan->pchan = pchan;
                else 
                        return;
        }
        


        vd = vchan_next_desc(&vchan->vchan);
        if (!vd) {
		dev_vdbg(vchan2dev(vchan), "peridma_vchan_start_first_queued: no next vd found \n");
                goto freepchan;
	}

        desc = vd_to_k510_peridma_desc(vd);
        dev_vdbg(vchan2dev(vchan), "peridma_vchan_start_first_queued: desc %px \n", desc);

	if(!list_empty(&desc->xfer_list)){
        	hwdesc = list_first_entry(&desc->xfer_list, struct k510_peridma_hwdesc, list);
//		list_del(&hwdesc->list);
//		atomic_sub(1, &desc->hwdesc_remain);

		dev_vdbg(vchan2dev(vchan), "peridma_vchan_start_first_queued: hwdesc(%px) \n", hwdesc);
		dev_vdbg(vchan2dev(vchan), " ctl=%x, cfg0=%x, cfg1=%x, dev_tout=%x, dev_sel=%x, src_addr=%x, dst_addr=%x \n", hwdesc->ctl, hwdesc->cfg0, hwdesc->cfg1, hwdesc->dev_tout, hwdesc->dev_sel, hwdesc->src_addr, hwdesc->dst_addr);
                vchan->desc = desc;
                desc->hwdesc = hwdesc;

		peridma_pchan_irq_clear(vchan->pchan);
                configure_pchan(vchan->pchan, desc->hwdesc);
                peridma_pchan_start(vchan->pchan);
                peridma_pchan_irq_enable(vchan->pchan);

                return;
        }


freepchan:
//	dev_vdbg(vchan2dev(vchan), "peridma_vchan_start_first_queued: free_pchan \n");
//        free_pchan(pdev, pchan);
        return;

}


static void peridma_vchan_xfer_complete(struct k510_peridma_vchan *vchan)
{
        struct virt_dma_desc *vd;
        unsigned long flags;

//        spin_lock_irqsave(&vchan->vchan.lock, flags);

        if (unlikely(!peridma_pchan_is_idle(vchan->pchan))) {
                dev_err(vchan2dev(vchan), "BUG: %s caught chx_tr_done, but channel not idle!\n",
                        peridma_vchan_name(vchan));
                peridma_pchan_stop(vchan->pchan);
        }

        /* The completed descriptor currently is in the head of vc list */
        vd = vchan_next_desc(&vchan->vchan);

        /* Remove the completed descriptor from issued list before completing */
        list_del(&vd->node);
        vchan_cookie_complete(vd);

        /* Submit queued descriptors after processing the completed ones */
        peridma_vchan_start_first_queued(vchan);

//        spin_unlock_irqrestore(&vchan->vchan.lock, flags);
}


static void k510_peridma_issue_pending(struct dma_chan *dchan)
{
        struct k510_peridma_vchan *vchan = dchan_to_k510_peridma_vchan(dchan);
        unsigned long flags;

        spin_lock_irqsave(&vchan->vchan.lock, flags);
	
        if (vchan_issue_pending(&vchan->vchan)){
	        dev_vdbg(vchan2dev(vchan), "k510_peridma_issue_pending: 1 \n");
                peridma_vchan_start_first_queued(vchan);
	}
	dev_vdbg(vchan2dev(vchan), "k510_peridma_issue_pending: 2 \n");

        spin_unlock_irqrestore(&vchan->vchan.lock, flags);
}

static irqreturn_t k510_peridma_interrupt(int irq, void *dev_id)
{
    struct k510_peridma_dev *priv = dev_id;
    struct k510_peridma_vchan *vchan;
    struct k510_peridma_desc *desc = NULL;
    struct k510_peridma_hwdesc *hwdesc = NULL;
    int i;
    u32 int_en0_save, int_en1_save;
    u32 ch_id;
    unsigned long int_stat0 = 0, int_stat1 = 0;
    unsigned long int_raw0 = 0, int_raw1 = 0;
    u16 int_tr_done = 0, int_half_done = 0, int_dev_tout = 0;


    /*  read SDMA_INT_XXX registers  */
    int_stat0 = ioread32(priv->base + PDMA_INT_STAT0);
    int_stat1 = ioread32(priv->base + PDMA_INT_STAT1);
    int_raw0 = ioread32(priv->base + PDMA_INT_RAW0);
    int_raw1 = ioread32(priv->base + PDMA_INT_RAW1);

    dev_vdbg(priv->dev, "k510_peridma_interrupt: int_stat0=%x , int_stat1=%x, int_raw0=%x, int_raw1=%x \n", int_stat0, int_stat1, int_raw0, int_raw1);


    /*  save PDMA_INT_EN0 register  */
    int_en0_save = ioread32(priv->base + PDMA_INT_EN0);
    int_en1_save = ioread32(priv->base + PDMA_INT_EN1);

    dev_vdbg(priv->dev, "k510_peridma_interrupt: int_en0=%x , int_en1=%x \n", int_en0_save, int_en0_save);

    /*  disable all the interrupts  */
    iowrite32(0, priv->base + PDMA_INT_EN0);
    iowrite32(0, priv->base + PDMA_INT_EN1);

    dev_vdbg(priv->dev, "k510_peridma_interrupt: disabled all interrupts \n");

    for_each_set_bit(i, &int_stat0, 16) {

        /*clear the interrupt we handled*/
        iowrite32(0x1<<i, priv->base+PDMA_INT_RAW0);

        ch_id = i;
        vchan = priv->pchan[ch_id].vchan;

        spin_lock(&vchan->vchan.lock);

                dev_vdbg(vchan2dev(vchan), "k510_peridma_interrupt: process Transfer done irq of phy ch%d, int_status(%x) \n", ch_id, int_stat0);

        desc = vchan->desc;

        if(desc->cyclic)
        {
            hwdesc = desc->cyclic;
            dev_vdbg(vchan2dev(vchan),"hwdesc(%px): last=%d ctl=%x, cfg0=%x, \n cfg1=%x, dev_tout=%x, dev_sel=%x, src_addr=%x, dst_addr=%x \n", hwdesc, hwdesc->last, hwdesc->ctl, hwdesc->cfg0, hwdesc->cfg1, hwdesc->dev_tout, hwdesc->dev_sel, hwdesc->src_addr, hwdesc->dst_addr);
            desc->remain -= hwdesc->cfg1;
            if(desc->remain == 0)
                desc->remain = desc->buf_len;
            if(hwdesc->last)
                vchan_cyclic_callback(&vchan->desc->vd);

            if(desc->cyclic->list.next == &(desc->xfer_list))
                desc->cyclic = list_entry(desc->xfer_list.next, typeof(*(desc->cyclic)), list);
            else
                desc->cyclic = list_next_entry(desc->hwdesc, list);

            hwdesc = desc->cyclic;
            desc->hwdesc = hwdesc;

            configure_pchan(vchan->pchan, hwdesc);
            peridma_pchan_start(vchan->pchan);
        }
        else
        {
            list_del(&desc->hwdesc->list);
            list_add_tail(&desc->hwdesc->list, &desc->completed_list);

            if(!list_empty(&desc->xfer_list)){
                hwdesc = list_first_entry(&desc->xfer_list, struct k510_peridma_hwdesc, list);
                //		        list_del(&hwdesc->list);
                //		        atomic_sub(1, &desc->hwdesc_remain);

                dev_vdbg(vchan2dev(vchan), "hwdesc(%px): ctl=%x, cfg0=%x, cfg1=%x, dev_tout=%x, dev_sel=%x, src_addr=%x, dst_addr=%x \n", hwdesc, hwdesc->ctl, hwdesc->cfg0, hwdesc->cfg1, hwdesc->dev_tout, hwdesc->dev_sel, hwdesc->src_addr, hwdesc->dst_addr);
                desc->hwdesc = hwdesc;
                //			desc->next = true;
                configure_pchan(vchan->pchan, hwdesc);
                peridma_pchan_start(vchan->pchan);
            } else	{
                vchan->desc = NULL;
                //			desc->next = false;
                peridma_vchan_xfer_complete(vchan);
            }
        }

        spin_unlock(&vchan->vchan.lock);

        continue;
    }

    if(int_stat1){
        for_each_set_bit(i, &int_stat1, 16){
            /*clear the interrupt we handled*/
            iowrite32(0x1<<i, priv->base+PDMA_INT_RAW1);

            /*channel timeout interrupt*/
            ch_id = i;
            vchan = priv->pchan[ch_id].vchan;
            desc = vchan->desc;
            dev_err(vchan2dev(vchan), "channel %d timeout \n", ch_id);
        }
    }

    /**    restore the PDMA_INT_ENx register  **/
    int_en0_save |= ioread32(priv->base + PDMA_INT_EN0);
    iowrite32(int_en0_save, priv->base + PDMA_INT_EN0);
    int_en1_save |= ioread32(priv->base + PDMA_INT_EN1);
    iowrite32(int_en1_save, priv->base + PDMA_INT_EN1);

    return IRQ_HANDLED;
}

static int k510_peridma_vchan_pause(struct dma_chan *dchan)
{
        struct k510_peridma_vchan *vchan = dchan_to_k510_peridma_vchan(dchan);
        unsigned long flags; 
        unsigned int timeout = 20; /* timeout iterations */
        u32 val;

	if(!vchan->pchan){
		dev_err(vchan2dev(vchan), "k510_peridma_vchan_pause: vchan->pchan is NULL \n");
		dump_stack();
		return -EBUSY;
	}

        spin_lock_irqsave(&vchan->vchan.lock, flags);

        if (!peridma_pchan_is_busy(vchan->pchan)) {
                dev_err(vchan2dev(vchan), "%s is non-busy!\n",
                        peridma_vchan_name(vchan));

		spin_unlock_irqrestore(&vchan->vchan.lock, flags);

                return -EBUSY;
        }

	peridma_pchan_pause(vchan->pchan);

        do  {   
                if (peridma_pchan_is_paused(vchan->pchan))
                        break;

                udelay(2);
        } while (--timeout);

        peridma_pchan_irq_disable(vchan->pchan);

        vchan->is_paused = true;

        spin_unlock_irqrestore(&vchan->vchan.lock, flags);

        return timeout ? 0 : -EAGAIN;
}

static int k510_peridma_vchan_resume(struct dma_chan *dchan)
{
        struct k510_peridma_vchan *vchan = dchan_to_k510_peridma_vchan(dchan);
        unsigned long flags;

        spin_lock_irqsave(&vchan->vchan.lock, flags);

        if (vchan->is_paused){
		peridma_pchan_irq_enable(vchan->pchan);
                peridma_pchan_resume(vchan->pchan);
	}

	vchan->is_paused = false;

        spin_unlock_irqrestore(&vchan->vchan.lock, flags);

        return 0;
}


static void k510_peridma_vchan_free_chan_resources(struct dma_chan *dchan)
{
        struct k510_peridma_vchan *vchan = dchan_to_k510_peridma_vchan(dchan);

        if(vchan->pchan) {
            /* ASSERT: channel is idle */
            if (!peridma_pchan_is_idle(vchan->pchan)) {
                    dev_err(vchan2dev(vchan), "%s is non-idle!\n",
                        peridma_vchan_name(vchan));
            }

        peridma_pchan_stop(vchan->pchan);
        peridma_pchan_irq_disable(vchan->pchan);
        }


        vchan_free_chan_resources(&vchan->vchan);

        dev_vdbg(vchan2dev(vchan),
                 "%s: free resources, descriptor still allocated: %u\n",
                 peridma_vchan_name(vchan), atomic_read(&vchan->descs_allocated));

}

static int k510_peridma_config(struct dma_chan *chan,
                            struct dma_slave_config *config)
{
        struct k510_peridma_vchan *vchan = to_k510_peridma_vchan(chan);

        memcpy(&vchan->cfg, config, sizeof(*config));

        return 0;
}


static int parse_device_properties(struct k510_peridma_dev *pdev)
{
        struct device *dev = pdev->dev;
        u32 tmp;
        int ret;

        ret = device_property_read_u32(dev, "dma-channels", &tmp);
        if (ret)
                return ret;

        if (tmp == 0 || tmp > CH_NUM)
                return -EINVAL;

        pdev->nr_channels = tmp;

        ret = device_property_read_u32(dev, "dma-requests", &tmp);
        if (ret)
                return ret; 

        if (tmp == 0 || tmp > 35)
                return -EINVAL;

        pdev->nr_requests = tmp;

        return 0;
}

static struct dma_chan *k510_pdma_of_xlate(struct of_phandle_args *dma_spec,
                                                struct of_dma *ofdma)
{
        struct k510_peridma_dev *priv = ofdma->of_dma_data;
        struct k510_peridma_vchan *vchan;
        struct dma_chan *chan;
        unsigned int priority = dma_spec->args[0];
        unsigned int dev_tout = dma_spec->args[1];
        unsigned int dat_endian = dma_spec->args[2];
        unsigned int dev_sel = dma_spec->args[3];
	
	dev_vdbg(priv->dev, "k510_pdma_of_xlate: priority=%x, dev_tout=%x, dat_endian=%x, dev_sel=%x \n", priority, dev_tout, dat_endian, dev_sel);

        if (dev_sel > (priv->nr_requests -1))
                return NULL;

        chan = dma_get_slave_channel(&(priv->vchan[dev_sel].vchan.chan));

        if(!chan)
                return NULL;
        
        vchan = to_k510_peridma_vchan(chan);

        vchan->priority = priority;
        vchan->dev_tout = dev_tout;
        vchan->dat_endian = dat_endian;
        vchan->dev_sel = dev_sel;

	dev_vdbg(priv->dev, "k510_pdma_of_xlate: get vchan %s \n", peridma_vchan_name(vchan));
        return chan;
}

static int k510_peridma_probe(struct platform_device *pdev)
{
	struct k510_peridma_dev *priv;
	struct resource *res;
	int i, j, ret;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

        priv->dev = &pdev->dev;
        priv->slave.dev = &pdev->dev;

	priv->irq = platform_get_irq(pdev, 0);
	if (priv->irq < 0) {
		dev_err(&pdev->dev, "Cannot claim IRQ\n");
		return priv->irq;
	}

	priv->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(priv->clk)) {
		dev_err(&pdev->dev, "No clock specified\n");
		return PTR_ERR(priv->clk);
	}

        ret = parse_device_properties(priv);
        if (ret)
                return ret;

	/*  Initialize physical channels  */
        priv->pchan = devm_kcalloc(priv->dev, priv->nr_channels,
                                sizeof(struct k510_peridma_pchan), GFP_KERNEL);
        if (!priv->pchan)
                return -ENOMEM;

//	INIT_LIST_HEAD(&priv->slave.channels);
        for (i = 0; i < priv->nr_channels; i++) {
                struct k510_peridma_pchan *pchan = &priv->pchan[i];

                pchan->pdev = priv;
                pchan->id = i;
                pchan->ch_regs = priv->base + CH0_BASE + i * CH_OFF;
//                atomic_set(&chan->descs_allocated, 0);

//                chan->vchan.desc_free = vchan_desc_put;
//                vchan_init(&chan->vchan, &priv->slave);
        }

        /*  Initialize virtual channels  */
        priv->vchan = devm_kcalloc(priv->dev, priv->nr_requests,
                                sizeof(struct k510_peridma_vchan), GFP_KERNEL);
        if (!priv->vchan)
                return -ENOMEM;

        INIT_LIST_HEAD(&priv->slave.channels);
        for (i = 0; i < priv->nr_requests; i++) {
                struct k510_peridma_vchan *vchan = &priv->vchan[i];

                vchan->pdev = priv;
		vchan->desc = NULL;
                atomic_set(&vchan->descs_allocated, 0);

                vchan->vchan.desc_free = vchan_desc_put;
                vchan_init(&vchan->vchan, &priv->slave);
        }

	platform_set_drvdata(pdev, priv);
	spin_lock_init(&priv->lock);

	dma_cap_zero(priv->slave.cap_mask);
	dma_cap_set(DMA_SLAVE, priv->slave.cap_mask);

//	priv->slave.chancnt			= priv->nr_channels;
//        priv->slave.src_addr_widths = DMA_SLAVE_BUSWIDTH_8_BYTES;
//        priv->slave.dst_addr_widths = DMA_SLAVE_BUSWIDTH_8_BYTES;
//        priv->slave.directions = BIT(DMA_MEM_TO_MEM);
//        priv->slave.residue_granularity = DMA_RESIDUE_GRANULARITY_DESCRIPTOR;

        priv->slave.directions                  = BIT(DMA_DEV_TO_MEM) |
                                                  BIT(DMA_MEM_TO_DEV);
	priv->slave.residue_granularity         = DMA_RESIDUE_GRANULARITY_BURST;
        priv->slave.src_addr_widths             = BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) |
                                                  BIT(DMA_SLAVE_BUSWIDTH_2_BYTES) |
                                                  BIT(DMA_SLAVE_BUSWIDTH_4_BYTES);
        priv->slave.dst_addr_widths             = BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) |
                                                  BIT(DMA_SLAVE_BUSWIDTH_2_BYTES) |
                                                  BIT(DMA_SLAVE_BUSWIDTH_4_BYTES);


	priv->slave.device_tx_status		= k510_peridma_tx_status;
	priv->slave.device_issue_pending	= k510_peridma_issue_pending;
	priv->slave.device_terminate_all	= k510_peridma_terminate_all;
        priv->slave.device_pause 		= k510_peridma_vchan_pause;
        priv->slave.device_resume 		= k510_peridma_vchan_resume;

//        priv->slave.device_alloc_chan_resources	= k510_peridma_vchan_alloc_chan_resources;
        priv->slave.device_free_chan_resources	= k510_peridma_vchan_free_chan_resources;

	priv->slave.device_prep_slave_sg	= k510_peridma_prep_slave_sg;
    priv->slave.device_prep_dma_cyclic	= k510_peridma_prep_dma_cyclic;
	priv->slave.device_config		= k510_peridma_config;

	priv->slave.dev = &pdev->dev;

	ret = clk_prepare_enable(priv->clk);
	if (ret) {
		dev_err(&pdev->dev, "Couldn't enable the clock\n");
		return ret;
	}

	/*
	 * Initialize peridma controller
	 */

	k510_peridma_hw_init(priv);

	ret = devm_request_irq(&pdev->dev, priv->irq, k510_peridma_interrupt,
			       0, dev_name(&pdev->dev), priv);
	if (ret) {
		dev_err(&pdev->dev, "Cannot request IRQ\n");
		goto err_clk_disable;
	}

	ret = dma_async_device_register(&priv->slave);
	if (ret) {
		dev_warn(&pdev->dev, "Failed to register DMA engine device\n");
		goto err_clk_disable;
	}

        ret = of_dma_controller_register(pdev->dev.of_node, k510_pdma_of_xlate,
                                         priv);
        if (ret) {
                dev_err(&pdev->dev, "of_dma_controller_register failed\n");
                goto err_clk_disable;
        }


	dev_vdbg(&pdev->dev, "Successfully probed K510 peridma controller, register mapped at %px(PHY %px) \n", priv->base ,res->start);

	return 0;

err_clk_disable:
	clk_disable_unprepare(priv->clk);
	return ret;
}

static int k510_peridma_remove(struct platform_device *pdev)
{
	struct k510_peridma_dev *priv = platform_get_drvdata(pdev);

	/* Disable IRQ so no more work is scheduled */
	disable_irq(priv->irq);

	of_dma_controller_free(pdev->dev.of_node);
	dma_async_device_unregister(&priv->slave);

	clk_disable_unprepare(priv->clk);

	return 0;
}

static const struct of_device_id k510_peridma_match[] = {
	{ .compatible = "canaan,k510-pdma" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, k510_peridma_match);

static struct platform_driver k510_peridma_driver = {
	.probe	= k510_peridma_probe,
	.remove	= k510_peridma_remove,
	.driver	= {
		.name		= "k510-peridma",
		.of_match_table	= k510_peridma_match,
	},
};

module_platform_driver(k510_peridma_driver);

MODULE_DESCRIPTION("K510 peridma driver");
MODULE_LICENSE("GPL");
