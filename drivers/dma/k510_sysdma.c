/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define DEBUG
#define VERBOSE_DEBUG

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

#include "virt-dma.h"


/** Common macros to  SYSDMA registers **/

/** DMA register offsets **/

/* Global register offsets */
#define SDMA_INT_RAW		0x0
#define SDMA_INT_EN		0x4
#define SDMA_INT_STAT		0x8
#define SDMA_CH_WEIGHT		0xc

#define DEF_CH_WEIGHT		0xe4e4e4e4

/* Channel register offsets */
#define CH_CTL			0x0
#define CH_CFG0			0x4
#define CH_CFG1			0x8
#define CH_SADDR		0xc
#define CH_DADDR		0x10
#define CH_USR_DATA		0x14
#define CH_LLT_CFG		0x18
#define CH_STAT			0x1c
#define CH_OFF			0x20
#define CH0_BASE		0x20

#define CH_NUM			4

/**   SDMA_INT_RAW fields **/
#define CHX_TR_DONE_SHIFT	0x0
#define CHX_HALF_DONE_SHIFT	0x4
#define CHX_LLT_EVENT_SHIFT	0x8

/**   SDMA_INT_EN fields  **/
#define CHX_TR_DONE_EN_SHIFT	0x0
#define CHX_HALF_DONE_EN_SHIFT	0x4
#define CHX_LLT_EVENT_EN_SHIFT	0x8

/**   SDMA_INT_STAT fields  **/
//#define CHX_TR_DONE_SHIFT	0x0
//#define CHX_HALF_DONE_SHIFT	0x4
//#define CHX_LLT_EVENT_SHIFT	0x8

#define CH_INT_MASK		0xf

/**    CH_CTL              **/
#define CH_START		0x0
#define WE_CH_START		0x1
#define CH_STOP			0x4
#define WE_CH_STOP		0x5
#define CH_PAUSE		0x8
#define WE_CH_PAUSE		0x9

#define MAX_LINE_SIZE		0xfffff
#define MAX_LINE_NUM		0x7ff
#define MAX_TRANS_LEN		(MAX_LINE_SIZE*MAX_LINE_NUM)

/**    CH_CFG1             **/
#define CH_LINE_SPACE_SHIFT	0
#define CH_TRANS_MODE_SHIFT	16
#define CH_DAT_ENDIAN_SHIFT	18
#define CH_DAT_MODE_SHIFT	20
#define CH_USR_DAT_SIZ_SHIFT	24


/**    CH_STAT             **/
#define CH_STAT_IDLE		0x1
#define CH_STAT_PAUSE		0x10
#define CH_STAT_BUSY		0x100

/**    CH_LLT_CFG          **/
#define CH_LLT_SADDR_MASK	0xff
#define CH_LLT_SADDR_SHIFT	0
#define WE_CH_LLT_SADDR_SHIFT	8
#define CH_LLT_LENTH_MASK	0x7f
#define CH_LLT_LENTH_SHIFT	12
#define WE_CH_LLT_LENTH_SHIFT	19
#define	CH_LLT_GRP_NUM_MASK	0x3f
#define	CH_LLT_GRP_NUM_SHIFT	20
#define WE_CH_LLT_GRP_NUM_SHIFT	27

#define CH_LLT_LENTH		0x3c
#define CH_LLT_XFER_MAX_LEN	(CH_LLT_LENTH*MAX_LINE_SIZE)
#define LLT_OFF			0x100

typedef enum sysdma_mode {
    LINE_MODE,
    RECT_MODE,
    LLT_MODE,
    DMA_MODE_MAX
} sysdma_mode_t;

/** DMA Driver **/
struct k510_sysdma_llt {
        volatile unsigned int		llt_src_addr;
        volatile unsigned int          llt_dst_addr;
        volatile unsigned int          llt_line_size;
        volatile unsigned int          llt_cfg;
};

struct k510_sysdma_desc {
        struct k510_sysdma_llt          *llt_entry;
        u32                             llt_num;
        u32                             llt_grp_num;
	u32				llt_rest;
	u32				llt_lenth;
	bool				llt_need_update;
	bool				llt_update_flag;
        sysdma_mode_t                   trans_mode;
        u32                             line_num;
        u32                             line_size;
        u32                             line_space;
        dma_addr_t                      dst_addr;
        dma_addr_t                      src_addr;

        struct virt_dma_desc            vd;
        struct k510_sysdma_chan         *chan;
        struct list_head                xfer_list;
};



struct k510_sysdma_chan {
        struct k510_sysdma_dev		*pdev;
	/* Register base of channel */
	void __iomem			*ch_regs;
        u8				id;
        atomic_t			descs_allocated;
	struct virt_dma_chan		vchan;
	struct k510_sysdma_desc		*desc;
        bool				is_paused;
        
};

struct k510_sysdma_dev {
	DECLARE_BITMAP(pchans_used, CH_NUM);
        struct device			*dev;
	struct dma_device		slave;
	struct k510_sysdma_chan		*chans;
        u32				nr_channels;
	void __iomem			*base;
	struct clk			*clk;
	int				irq;
	spinlock_t			lock;
};

struct k510_sysdma_desc record_desc;

struct chan_line_mode_record {
    u32             line_num;
    u32             line_size;
    dma_addr_t      dst_addr;
    dma_addr_t      src_addr;
};

struct chan_line_mode_record chan_line_mode_record_t[4];

static struct k510_sysdma_dev *to_k510_sysdma_dev(struct dma_device *dev)
{
	return container_of(dev, struct k510_sysdma_dev, slave);
}

static struct k510_sysdma_chan *to_k510_sysdma_chan(struct dma_chan *chan)
{
        return container_of(chan, struct k510_sysdma_chan, vchan.chan);
}

static struct device *chan2dev(struct k510_sysdma_chan *chan)
{
	return &chan->vchan.chan.dev->device;
}

static inline struct k510_sysdma_desc *vd_to_k510_sysdma_desc(struct virt_dma_desc *vd)
{
        return container_of(vd, struct k510_sysdma_desc, vd);
}

static inline struct k510_sysdma_chan *vc_to_k510_sysdma_chan(struct virt_dma_chan *vc)
{
        return container_of(vc, struct k510_sysdma_chan, vchan);
}

static inline struct k510_sysdma_chan *dchan_to_k510_sysdma_chan(struct dma_chan *dchan)
{
        return vc_to_k510_sysdma_chan(to_virt_chan(dchan));
}

static inline const char *sysdma_chan_name(struct k510_sysdma_chan *chan)
{
        return dma_chan_name(&chan->vchan.chan);
}

static inline bool syadma_chan_is_idle(struct k510_sysdma_chan *chan)
{
        u32 val;

        val = ioread32(chan->ch_regs + CH_STAT);

        return (val & CH_STAT_IDLE);
}

static inline bool syadma_chan_is_busy(struct k510_sysdma_chan *chan)
{
        u32 val;

        val = ioread32(chan->ch_regs + CH_STAT);

        return (val & CH_STAT_BUSY);
}

static inline bool syadma_chan_is_paused(struct k510_sysdma_chan *chan)
{
        u32 val;

        val = ioread32(chan->ch_regs + CH_STAT);

        return (val & CH_STAT_PAUSE);
}

static void k510_sysdma_free_chan_resources(struct dma_chan *chan)
{
	struct k510_sysdma_chan *pchan = to_k510_sysdma_chan(chan);

	vchan_free_chan_resources(&pchan->vchan);
}

static inline void sysdma_chan_irq_enable(struct k510_sysdma_chan *chan)
{
	struct k510_sysdma_dev *pdev = chan->pdev;
        u32 reg;
	u32 ch_id = chan->id;
        struct k510_sysdma_desc *desc;
        struct virt_dma_desc *vd;

        vd = vchan_next_desc(&chan->vchan);
        if (!vd)
                return;

        desc = vd_to_k510_sysdma_desc(vd);

        reg = ioread32(pdev->base + SDMA_INT_EN);
        reg |= 0x1 << ch_id+CHX_TR_DONE_EN_SHIFT;
//        reg |= 0x1 << ch_id+CHX_HALF_DONE_EN_SHIFT;
	if(desc->trans_mode == LLT_MODE)
		reg |= 0x1 << ch_id+CHX_LLT_EVENT_EN_SHIFT;

        iowrite32(reg, pdev->base + SDMA_INT_EN);
}

static inline void sysdma_chan_irq_disable(struct k510_sysdma_chan *chan)
{
	struct k510_sysdma_dev *pdev = chan->pdev;
	u32 reg;
	u32 ch_id = chan->id;

        reg = ioread32(pdev->base + SDMA_INT_EN);
        reg &= ~(0x1 << ch_id+CHX_TR_DONE_EN_SHIFT);
        reg &= ~(0x1 << ch_id+CHX_HALF_DONE_EN_SHIFT);
        reg &= ~(0x1 << ch_id+CHX_LLT_EVENT_EN_SHIFT);
	
	iowrite32(reg, pdev->base + SDMA_INT_EN);
}

static inline void sysdma_chan_start(struct k510_sysdma_chan *chan)
{
        u32 reg;

        reg = ioread32(chan->ch_regs + CH_CTL);
        reg |= 0x1 << CH_START;
        reg |= 0x1 << WE_CH_START;

        iowrite32(reg, chan->ch_regs + CH_CTL);
}

static inline void sysdma_chan_stop(struct k510_sysdma_chan *chan)
{
        u32 reg;
 
	reg = ioread32(chan->ch_regs + CH_CTL);
        reg |= 0x1 << CH_STOP;
        reg |= 0x1 << WE_CH_STOP;

        iowrite32(reg, chan->ch_regs + CH_CTL);
}

static inline void sysdma_chan_pause(struct k510_sysdma_chan *chan)
{
        u32 reg;

        reg = ioread32(chan->ch_regs + CH_CTL);
        reg |= 0x1 << CH_PAUSE;
        reg |= 0x1 << WE_CH_PAUSE;

        iowrite32(reg, chan->ch_regs + CH_CTL);
}

static inline void sysdma_chan_resume(struct k510_sysdma_chan *chan)
{
        u32 reg;

        reg = ioread32(chan->ch_regs + CH_CTL);
        reg &= ~(0x1 << CH_PAUSE);
        reg |= 0x1 << WE_CH_PAUSE;

        iowrite32(reg, chan->ch_regs + CH_CTL);
}

static void k510_sysdma_hw_init(struct k510_sysdma_dev *pdev)
{
        u32 i;

        iowrite32(0, pdev->base + SDMA_INT_EN);

        for (i = 0; i < pdev->nr_channels; i++) {
                sysdma_chan_stop(&pdev->chans[i]);
        }
}

static void sysdma_desc_put(struct k510_sysdma_desc *desc)
{
        struct k510_sysdma_chan *chan = desc->chan;
        unsigned int descs_put = 1;
	struct k510_sysdma_llt *llt_ptr = desc->llt_entry;

	if(desc->llt_need_update)
		kfree(llt_ptr);
	
	kfree(desc);

        atomic_sub(descs_put, &chan->descs_allocated);
        dev_vdbg(chan2dev(chan), "%s: %d descs put, %d still allocated\n",
                sysdma_chan_name(chan), descs_put,
                atomic_read(&chan->descs_allocated));
}

static void vchan_desc_put(struct virt_dma_desc *vdesc)
{
        sysdma_desc_put(vd_to_k510_sysdma_desc(vdesc));
}
#if 0
static void update_line_mode(struct k510_sysdma_chan *chan)
{ 
    if(record_desc.line_size <= MAX_LINE_SIZE)
    {
        iowrite32(record_desc.src_addr + MAX_LINE_SIZE*record_desc.llt_num, chan->ch_regs + CH_SADDR);
        iowrite32(record_desc.dst_addr + MAX_LINE_SIZE*record_desc.llt_num, chan->ch_regs + CH_DADDR);
        iowrite32(record_desc.line_size, chan->ch_regs + CH_CFG0);
        record_desc.line_size = 0;
        record_desc.llt_num = 0;
    }
    else
    {
        iowrite32(record_desc.src_addr + MAX_LINE_SIZE*record_desc.llt_num, chan->ch_regs + CH_SADDR);
        iowrite32(record_desc.dst_addr + MAX_LINE_SIZE*record_desc.llt_num, chan->ch_regs + CH_DADDR);
        iowrite32(MAX_LINE_SIZE, chan->ch_regs + CH_CFG0);
        record_desc.line_size -= MAX_LINE_SIZE;
        record_desc.llt_num++;
    }
}
#endif
static void update_line_mode(struct k510_sysdma_chan *chan)
{ 
    if(chan_line_mode_record_t[chan->id].line_size <= MAX_LINE_SIZE)
    {
        iowrite32(chan_line_mode_record_t[chan->id].src_addr + MAX_LINE_SIZE*chan_line_mode_record_t[chan->id].line_num, chan->ch_regs + CH_SADDR);
        iowrite32(chan_line_mode_record_t[chan->id].dst_addr + MAX_LINE_SIZE*chan_line_mode_record_t[chan->id].line_num, chan->ch_regs + CH_DADDR);
        iowrite32(chan_line_mode_record_t[chan->id].line_size, chan->ch_regs + CH_CFG0);
        chan_line_mode_record_t[chan->id].line_size = 0;
        chan_line_mode_record_t[chan->id].line_num = 0;
    }
    else
    {
        iowrite32(chan_line_mode_record_t[chan->id].src_addr + MAX_LINE_SIZE*chan_line_mode_record_t[chan->id].line_num, chan->ch_regs + CH_SADDR);
        iowrite32(chan_line_mode_record_t[chan->id].dst_addr + MAX_LINE_SIZE*chan_line_mode_record_t[chan->id].line_num, chan->ch_regs + CH_DADDR);
        iowrite32(MAX_LINE_SIZE, chan->ch_regs + CH_CFG0);
        chan_line_mode_record_t[chan->id].line_size -= MAX_LINE_SIZE;
        chan_line_mode_record_t[chan->id].line_num++;
    }
}


static void update_llt(struct k510_sysdma_chan *chan)
{
        struct k510_sysdma_desc *desc;
        struct virt_dma_desc *vd;
	struct k510_sysdma_llt *llt_src;
	struct k510_sysdma_llt *llt_dest;
	u32 src_off;
	u32 dest_off;
	u32 count;

        vd = vchan_next_desc(&chan->vchan);
        if (!vd)
                return;

        desc = vd_to_k510_sysdma_desc(vd);
	
	if(!desc->llt_rest) {
		llt_src = desc->llt_entry;
		llt_dest = chan->pdev->base + LLT_OFF + chan->id*CH_LLT_LENTH;
		src_off = desc->llt_num - desc->llt_rest -1;
		dest_off = desc->llt_update_flag ? desc->llt_grp_num : 0;
		count = (desc->llt_rest >= desc->llt_grp_num) ? desc->llt_grp_num : desc->llt_rest;

		memcpy(&llt_dest[dest_off], &llt_src[src_off], count * sizeof(struct k510_sysdma_llt));

		desc->llt_rest -= count;
		desc->llt_update_flag = !desc->llt_update_flag;
	}

}

#if 0
static struct dma_async_tx_descriptor *
k510_sysdma_prep_dma_memcpy(struct dma_chan *dchan, dma_addr_t dest,
			  dma_addr_t src, size_t len, unsigned long flags)
{
        struct k510_sysdma_desc *desc = NULL;
        struct k510_sysdma_chan *chan = dchan_to_k510_sysdma_chan(dchan);
	struct k510_sysdma_llt *llt_ptr;
        size_t xfer_size;
	u32 llt_alloc_num = 0;
	int i = 0;


	printk("k510_sysdma_prep_dma_memcpy begin");
	printk("len: %ld, %x\r\n", len ,len);

        dev_vdbg(chan2dev(chan), "%s: memcpy: src: %pad dst: %pad length: %zd flags: %#lx",
                sysdma_chan_name(chan), &src, &dest, len, flags);

	desc = kzalloc(sizeof(struct k510_sysdma_desc), GFP_NOWAIT);

	
	printk("desc->llt_num: %d\r\n", desc->llt_num);

	if(!desc)
		return NULL;

	desc->chan = chan;
	chan->desc = desc;
	atomic_inc(&chan->descs_allocated);

	if(len <= MAX_LINE_SIZE){
		desc->trans_mode = LINE_MODE;
		desc->line_size = len;
		desc->src_addr = src;
		desc->dst_addr = dest;
	dev_vdbg(chan2dev(chan),"k510_sysdma_prep_dma_memcpy: desc=%px, desc->trans_mode=%d, desc->line_size=%x, desc->src_addr=%px, desc->dst_addr=%px \n", desc, desc->trans_mode, desc->line_size, desc->src_addr, desc->dst_addr);
	} else {
		desc->trans_mode = LLT_MODE;
		if(len <= CH_LLT_XFER_MAX_LEN) {
			desc->llt_entry = chan->pdev->base + LLT_OFF + chan->id*CH_LLT_LENTH;
			llt_ptr = desc->llt_entry;
	dev_vdbg(chan2dev(chan),"k510_sysdma_prep_dma_memcpy: desc=%px, desc->trans_mode=%d, desc->llt_entry=%px, llt_ptr=%px, len=%d, src=%px, dest=%px \n", desc, desc->trans_mode, desc->llt_entry, llt_ptr, len, src, dest);

			while(len){
				xfer_size = (len > MAX_LINE_SIZE) ? MAX_LINE_SIZE:len;
			
				llt_ptr[i].llt_line_size = xfer_size;
				llt_ptr[i].llt_src_addr = src;
				llt_ptr[i].llt_dst_addr = dest;
				llt_ptr[i].llt_cfg = chan->id << 0x4;

	dev_vdbg(chan2dev(chan), "k510_sysdma_prep_dma_memcpy: llt_line_size_addr=%px, llt_src_addr_addr=%px, llt_dst_addr_addr=%px, llt_cfg_addr=%px \n", &(llt_ptr[i].llt_line_size), &(llt_ptr[i].llt_src_addr), &(llt_ptr[i].llt_dst_addr), &(llt_ptr[i].llt_cfg));
	dev_vdbg(chan2dev(chan), "k510_sysdma_prep_dma_memcpy: llt_line_size=%d, llt_src_addr=%px, llt_dst_addr=%px, llt_cfg=%x \n", xfer_size, src, dest, chan->id << 0x4);
				len -= xfer_size;
				src += xfer_size;
				dest += xfer_size;
				i++;
			}
		
	dev_vdbg(chan2dev(chan), "k510_sysdma_prep_dma_memcpy:  llt_num=%d \n", i);
			desc->llt_num = i;
			desc->llt_rest = i;
			desc->llt_grp_num = i;

//			__set_bit(1 , &llt_ptr[0].llt_cfg);
			llt_ptr[0].llt_cfg |= 0x1 << 1;
//			__set_bit(0 , &llt_ptr[i-1].llt_cfg);
			llt_ptr[i-1].llt_cfg &= ~(0x1);
			
			desc->llt_need_update = false;
		} else {
			llt_alloc_num = (len/MAX_LINE_SIZE) + 1;
			
			desc->llt_entry = kzalloc(llt_alloc_num*sizeof(struct k510_sysdma_llt), GFP_NOWAIT);

			if(!desc->llt_entry) {
				dev_dbg(chan2dev(chan), "kzalloc failuer for llt \n");
				return NULL;
			}

			llt_ptr = desc->llt_entry;

                        while(len){
                                xfer_size = (len > MAX_LINE_SIZE) ? MAX_LINE_SIZE:len;

                                llt_ptr[i].llt_line_size = xfer_size;
                                llt_ptr[i].llt_src_addr = src;
                                llt_ptr[i].llt_dst_addr = dest;
                                llt_ptr[i].llt_cfg = chan->id << 0x4;

                                len -= xfer_size;
                                src += xfer_size;
                                dest += xfer_size;
                                i++;
                        }

                        desc->llt_num = i;
                        desc->llt_rest = i;
                        desc->llt_grp_num = CH_LLT_LENTH/2;

//                        set_bit(1 , &llt_ptr[0].llt_cfg);
//                        set_bit(0 , &llt_ptr[i-1].llt_cfg);
			llt_ptr[0].llt_cfg |= 0x1 << 1;
			llt_ptr[i-1].llt_cfg |= 0x1;

                        desc->llt_need_update = true;
                        desc->llt_update_flag = false;
			
		}
	}
	
	printk("k510_sysdma_prep_dma_memcpy end");
        return vchan_tx_prep(&chan->vchan, &desc->vd, flags);
}
#endif

#if 0
static struct dma_async_tx_descriptor *
k510_sysdma_prep_dma_memcpy(struct dma_chan *dchan, dma_addr_t dest,
			  dma_addr_t src, size_t len, unsigned long flags)
{
    struct k510_sysdma_desc *desc = NULL;
    struct k510_sysdma_chan *chan = dchan_to_k510_sysdma_chan(dchan);
	struct k510_sysdma_llt *llt_ptr;
    size_t xfer_size;
	u32 llt_alloc_num = 0;
	int i = 0;


	printk("k510_sysdma_prep_dma_memcpy begin");
	printk("len: %ld, %x\r\n", len ,len);

    dev_vdbg(chan2dev(chan), "%s: memcpy: src: %pad dst: %pad length: %zd flags: %#lx",
                sysdma_chan_name(chan), &src, &dest, len, flags);

	desc = kzalloc(sizeof(struct k510_sysdma_desc), GFP_NOWAIT);

	
	printk("desc->llt_num:%d, desc->llt_grp_num:%d, desc->line_size:%d\r\n", desc->llt_num, desc->llt_grp_num, desc->line_size);
    if(record_desc.llt_num == 0)
    {
        record_desc.line_size = len;
        record_desc.src_addr = src;
        record_desc.dst_addr = dest;
    }
    printk("record_desc.llt_num:%d, record_desc.llt_grp_num:%d, record_desc.line_size:%d\r\n", record_desc.llt_num, record_desc.llt_grp_num, record_desc.line_size);
	if(!desc)
		return NULL;

	desc->chan = chan;
	chan->desc = desc;
	atomic_inc(&chan->descs_allocated);

	if(len <= MAX_LINE_SIZE)
    {
        desc->trans_mode = LINE_MODE;
        desc->line_size = len;
        desc->src_addr = src;
        desc->dst_addr = dest;
        record_desc.llt_num = 0;
        record_desc.line_size = 0;
    }
    else 
    {
        desc->trans_mode = LINE_MODE;
        desc->line_size = MAX_LINE_SIZE;
        desc->src_addr = src;
        desc->dst_addr = dest;
        record_desc.llt_num++;
        record_desc.line_size -= MAX_LINE_SIZE;
    }
	dev_vdbg(chan2dev(chan),"k510_sysdma_prep_dma_memcpy: desc=%px, desc->trans_mode=%d, desc->line_size=%x, desc->src_addr=%px, desc->dst_addr=%px \n", desc, desc->trans_mode, desc->line_size, desc->src_addr, desc->dst_addr);
	
	
	printk("k510_sysdma_prep_dma_memcpy end");
    return vchan_tx_prep(&chan->vchan, &desc->vd, flags);
}
#endif
static struct dma_async_tx_descriptor *
k510_sysdma_prep_dma_memcpy(struct dma_chan *dchan, dma_addr_t dest,
			  dma_addr_t src, size_t len, unsigned long flags)
{
    struct k510_sysdma_desc *desc = NULL;
    struct k510_sysdma_chan *chan = dchan_to_k510_sysdma_chan(dchan);
	struct k510_sysdma_llt *llt_ptr;
    size_t xfer_size;
	u32 llt_alloc_num = 0;
	int i = 0;


	printk("k510_sysdma_prep_dma_memcpy begin");
	printk("len: %ld, %x\r\n", len ,len);

    dev_vdbg(chan2dev(chan), "%s: memcpy: src: %pad dst: %pad length: %zd flags: %#lx",
                sysdma_chan_name(chan), &src, &dest, len, flags);

	desc = kzalloc(sizeof(struct k510_sysdma_desc), GFP_NOWAIT);

	
	printk("desc->llt_num:%d, desc->llt_grp_num:%d, desc->line_size:%d\r\n", desc->llt_num, desc->llt_grp_num, desc->line_size);
    if(chan_line_mode_record_t[chan->id].line_num == 0)
    {
        chan_line_mode_record_t[chan->id].line_size = len;
        chan_line_mode_record_t[chan->id].src_addr = src;
        chan_line_mode_record_t[chan->id].dst_addr = dest;
    }
    printk("record_desc.llt_num:%d, record_desc.line_size:%x\r\n", 
            chan_line_mode_record_t[chan->id].line_num, chan_line_mode_record_t[chan->id].line_size);
	if(!desc)
		return NULL;

	desc->chan = chan;
	chan->desc = desc;
	atomic_inc(&chan->descs_allocated);

	if(len <= MAX_LINE_SIZE)
    {
        desc->trans_mode = LINE_MODE;
        desc->line_size = len;
        desc->src_addr = src;
        desc->dst_addr = dest;
        chan_line_mode_record_t[chan->id].line_num = 0;
        chan_line_mode_record_t[chan->id].line_size = 0;
    }
    else 
    {
        desc->trans_mode = LINE_MODE;
        desc->line_size = MAX_LINE_SIZE;
        desc->src_addr = src;
        desc->dst_addr = dest;
        chan_line_mode_record_t[chan->id].line_num++;
        chan_line_mode_record_t[chan->id].line_size -= MAX_LINE_SIZE;
    }
	dev_vdbg(chan2dev(chan),"k510_sysdma_prep_dma_memcpy: desc=%px, desc->trans_mode=%d, desc->line_size=%x, desc->src_addr=%px, desc->dst_addr=%px \n", desc, desc->trans_mode, desc->line_size, desc->src_addr, desc->dst_addr);
	
	
	printk("k510_sysdma_prep_dma_memcpy end");
    return vchan_tx_prep(&chan->vchan, &desc->vd, flags);
}


static int k510_sysdma_terminate_all(struct dma_chan *dchan)
{
        struct k510_sysdma_chan *chan = dchan_to_k510_sysdma_chan(dchan);
        unsigned long flags;
        LIST_HEAD(head);

        spin_lock_irqsave(&chan->vchan.lock, flags);

	sysdma_chan_irq_disable(chan);
        sysdma_chan_stop(chan);

        vchan_get_all_descriptors(&chan->vchan, &head);

        /*
         * As vchan_dma_desc_free_list can access to desc_allocated list
         * we need to call it in vc.lock context.
         */
        vchan_dma_desc_free_list(&chan->vchan, &head);

        spin_unlock_irqrestore(&chan->vchan.lock, flags);

        dev_vdbg(chan2dev(chan), "terminated: %s\n", sysdma_chan_name(chan));

        return 0;
}


static enum dma_status k510_sysdma_tx_status(struct dma_chan *dchan,
					   dma_cookie_t cookie,
					   struct dma_tx_state *txstate)
{
        struct k510_sysdma_chan *chan = dchan_to_k510_sysdma_chan(dchan);
        enum dma_status ret;

        ret = dma_cookie_status(dchan, cookie, txstate);

        if (chan->is_paused && ret == DMA_IN_PROGRESS)
                ret = DMA_PAUSED;

        return ret;
}

/* Called in chan locked context */
static void sysdma_chan_xfer_start(struct k510_sysdma_chan *chan,
                                      struct k510_sysdma_desc *first)
{
        u32 reg, irq_mask;

        if (unlikely(!syadma_chan_is_idle(chan))) {
                dev_err(chan2dev(chan), "%s is non-idle!\n",
                        sysdma_chan_name(chan));

                return;
        }

	dev_vdbg(chan2dev(chan), "sysdma_chan_xfer_start: first=%px, trans_mode=%d, line_size=%x, src_addr=%px, dst_addr=%px \n", first, first->trans_mode, first->line_size, first->src_addr, first->dst_addr);
	/**  set transfer mode in CH_CFG1 register   **/
        reg = first->trans_mode << CH_TRANS_MODE_SHIFT;
        iowrite32(reg, chan->ch_regs + CH_CFG1);


	dev_vdbg(chan2dev(chan), "sysdma_chan_xfer_start:  CH_CFG1=%x\n", ioread32(chan->ch_regs + CH_CFG1));
	if(first->trans_mode == LINE_MODE) {
		/**  set line size in CH_CFG0 register  **/
	        reg = first->line_size;
	        iowrite32(reg, chan->ch_regs + CH_CFG0);

		/**  set dest and src addr registers   **/
		iowrite32(first->src_addr, chan->ch_regs + CH_SADDR);
		iowrite32(first->dst_addr, chan->ch_regs + CH_DADDR);
	} else if(first->trans_mode == LLT_MODE){
		if(!first->llt_need_update) {
			/**  set CH_LLT_CFG register   **/
			reg = (chan->id*CH_LLT_LENTH) & CH_LLT_SADDR_MASK | 0x1 << WE_CH_LLT_SADDR_SHIFT;
			reg |= first->llt_grp_num << CH_LLT_LENTH_SHIFT | 0x1 << WE_CH_LLT_LENTH_SHIFT;
			reg |= first->llt_grp_num << CH_LLT_GRP_NUM_SHIFT | 0x1 << WE_CH_LLT_GRP_NUM_SHIFT;
			iowrite32(reg, chan->ch_regs + CH_LLT_CFG);
		} else {
			if(first->llt_num == first->llt_rest)
				update_llt(chan);

                        /**  set CH_LLT_CFG register   **/
                        reg = (chan->id*CH_LLT_LENTH) & CH_LLT_SADDR_MASK | 0x1 << WE_CH_LLT_SADDR_SHIFT;
                        reg |= (first->llt_grp_num *2) << CH_LLT_LENTH_SHIFT | 0x1 << WE_CH_LLT_LENTH_SHIFT;
                        reg |= first->llt_grp_num << CH_LLT_GRP_NUM_SHIFT | 0x1 << WE_CH_LLT_GRP_NUM_SHIFT;
                        iowrite32(reg, chan->ch_regs + CH_LLT_CFG);
		}
	}

        /**  enable channel transfer done interrupt   **/
	sysdma_chan_irq_enable(chan);

	/**  start channel transfer   **/
	sysdma_chan_start(chan);
}



static void sysdma_chan_start_first_queued(struct k510_sysdma_chan *chan)
{
        struct k510_sysdma_desc *desc;
        struct virt_dma_desc *vd;

        vd = vchan_next_desc(&chan->vchan);
        if (!vd) {
		printk("sysdma_chan_start_first_queued: no next vd found \n");
                return;
	}

        desc = vd_to_k510_sysdma_desc(vd);
        dev_vdbg(chan2dev(chan), "%s: started %u\n", sysdma_chan_name(chan),
                vd->tx.cookie);
        sysdma_chan_xfer_start(chan, desc);
}


static void sysdma_chan_xfer_complete(struct k510_sysdma_chan *chan)
{
        struct virt_dma_desc *vd;
        unsigned long flags;

        spin_lock_irqsave(&chan->vchan.lock, flags);
        if (unlikely(!syadma_chan_is_idle(chan))) {
                dev_err(chan2dev(chan), "BUG: %s caught chx_tr_done, but channel not idle!\n",
                        sysdma_chan_name(chan));
                sysdma_chan_stop(chan);
        }

        /* The completed descriptor currently is in the head of vc list */
        vd = vchan_next_desc(&chan->vchan);
        /* Remove the completed descriptor from issued list before completing */
        list_del(&vd->node);
        vchan_cookie_complete(vd);

        /* Submit queued descriptors after processing the completed ones */
        sysdma_chan_start_first_queued(chan);

        spin_unlock_irqrestore(&chan->vchan.lock, flags);
}


static void k510_sysdma_issue_pending(struct dma_chan *dchan)
{
        struct k510_sysdma_chan *chan = dchan_to_k510_sysdma_chan(dchan);
        unsigned long flags;

        spin_lock_irqsave(&chan->vchan.lock, flags);
	
        if (vchan_issue_pending(&chan->vchan)){
//		update_llt(chan);
                sysdma_chan_start_first_queued(chan);
	}

        spin_unlock_irqrestore(&chan->vchan.lock, flags);
}

#if 0
static irqreturn_t k510_sysdma_interrupt(int irq, void *dev_id)
{
	struct k510_sysdma_dev *priv = dev_id;
	struct k510_sysdma_chan *chan;
	int i;
	u32 int_en_save;
	u32 ch_id;
	unsigned long int_stat = 0;

	printk("k510_sysdma_interrupt begin\n");

        /*  read SDMA_INT_STAT register  */
        int_stat = ioread32(priv->base + SDMA_INT_STAT);
        dev_vdbg(priv->dev, "k510_sysdma_interrupt: int_stat=%x,  SDMA_INT_RAW=%x \n", int_stat, ioread32(priv->base + SDMA_INT_RAW));


	/*  save SDMA_INT_EN register  */
	int_en_save = ioread32(priv->base + SDMA_INT_EN);
	dev_vdbg(priv->dev, "k510_sysdma_interrupt: int_en_save=%x \n", int_en_save);
        /*  disable all the interrupts  */
	iowrite32(0, priv->base + SDMA_INT_EN);

	/*  read SDMA_INT_STAT register  */
//	int_stat = ioread32(priv->base + SDMA_INT_STAT);
//	printk("k510_sysdma_interrupt: 4, int_stat=%x \n", *((unsigned long *)priv->base + SDMA_INT_STAT));

	for_each_set_bit(i, &int_stat, 12) {
                iowrite32(0x1<<i, priv->base+SDMA_INT_RAW);

		if(i >= CHX_HALF_DONE_SHIFT && i < CHX_LLT_EVENT_SHIFT){
			ch_id = i-CHX_HALF_DONE_SHIFT;
			dev_vdbg(priv->dev, "%s %u halfdone IRQ status: 0x%x\n", 
				sysdma_chan_name(&priv->chans[ch_id]), ch_id, int_stat);
			continue;
		}

                if(i >= CHX_LLT_EVENT_SHIFT){
			ch_id = i-CHX_LLT_EVENT_SHIFT;
                        dev_vdbg(priv->dev, "%s %u llt IRQ status: 0x%x\n", 
				sysdma_chan_name(&priv->chans[ch_id]), ch_id, int_stat);

			chan = &priv->chans[ch_id];
			
			if(chan->desc->llt_need_update){
				update_llt(chan);
			}

		        /**    restore the SDMA_INT_EN register  **/
//		        int_en_save |= ioread32(priv->base + SDMA_INT_EN);
//		        iowrite32(int_en_save, priv->base + SDMA_INT_EN);

                        continue;
                }

		dev_vdbg(priv->dev, "%s %u Transfer done, IRQ status: 0x%x\n",
                                sysdma_chan_name(&priv->chans[ch_id]), ch_id, int_stat);

		chan = &priv->chans[i];

		sysdma_chan_xfer_complete(chan);

	}

	/**    restore the SDMA_INT_EN register  **/
	int_en_save |= ioread32(priv->base + SDMA_INT_EN);
	iowrite32(int_en_save, priv->base + SDMA_INT_EN);
	
	printk("k510_sysdma_interrupt end\n");

	return IRQ_HANDLED;
}
#endif

static irqreturn_t k510_sysdma_interrupt(int irq, void *dev_id)
{
	struct k510_sysdma_dev *priv = dev_id;
	struct k510_sysdma_chan *chan;
	int i;
	u32 int_en_save;
	u32 ch_id;
	unsigned long int_stat = 0;

	printk("k510_sysdma_interrupt begin\n");

    /*  read SDMA_INT_STAT register  */
    int_stat = ioread32(priv->base + SDMA_INT_STAT);
    dev_vdbg(priv->dev, "k510_sysdma_interrupt: int_stat=%x,  SDMA_INT_RAW=%x \n", int_stat, ioread32(priv->base + SDMA_INT_RAW));

	/*  save SDMA_INT_EN register  */
	int_en_save = ioread32(priv->base + SDMA_INT_EN);
	dev_vdbg(priv->dev, "k510_sysdma_interrupt: int_en_save=%x \n", int_en_save);
    /*  disable all the interrupts  */
	iowrite32(0, priv->base + SDMA_INT_EN);
	/*  read SDMA_INT_STAT register  */
    //	int_stat = ioread32(priv->base + SDMA_INT_STAT);
    //	printk("k510_sysdma_interrupt: 4, int_stat=%x \n", *((unsigned long *)priv->base + SDMA_INT_STAT));

	for_each_set_bit(i, &int_stat, 8) {
        iowrite32(0x1<<i, priv->base+SDMA_INT_RAW);
		if(i >= CHX_HALF_DONE_SHIFT && i < CHX_LLT_EVENT_SHIFT){
			ch_id = i-CHX_HALF_DONE_SHIFT;
			dev_vdbg(priv->dev, "%s %u halfdone IRQ status: 0x%x\n", 
				sysdma_chan_name(&priv->chans[ch_id]), ch_id, int_stat);
			continue;
		}
		//dev_vdbg(priv->dev, "%s %u Transfer done, IRQ status: 0x%x\n",
        //                        sysdma_chan_name(&priv->chans[ch_id]), ch_id, int_stat);
        printk("i:%d, chan->id:%d\n", i, chan->id);
		chan = &priv->chans[i];
        if(chan_line_mode_record_t[chan->id].line_num == 0)
            sysdma_chan_xfer_complete(chan);
        if(chan_line_mode_record_t[chan->id].line_size > 0)
        {
            printk("chan_line_mode_record_t[chan->id].line_size:%ld, %x\r\n", 
                chan_line_mode_record_t[chan->id].line_size, chan_line_mode_record_t[chan->id].line_size);
            update_line_mode(chan);
            sysdma_chan_irq_enable(chan);
            sysdma_chan_start(chan);
            continue;
        }
		
	}

	/**    restore the SDMA_INT_EN register  **/
	int_en_save |= ioread32(priv->base + SDMA_INT_EN);
	iowrite32(int_en_save, priv->base + SDMA_INT_EN);
	
	printk("k510_sysdma_interrupt end\n");

	return IRQ_HANDLED;
}

static int k510_sysdma_chan_pause(struct dma_chan *dchan)
{
        struct k510_sysdma_chan *chan = dchan_to_k510_sysdma_chan(dchan);
        unsigned long flags; 
        unsigned int timeout = 20; /* timeout iterations */
        u32 val;

        spin_lock_irqsave(&chan->vchan.lock, flags);

        if (!syadma_chan_is_busy(chan)) {
                dev_err(chan2dev(chan), "%s is non-busy!\n",
                        sysdma_chan_name(chan));
                return -EBUSY;
        }

	sysdma_chan_pause(chan);

        do  {   
                if (syadma_chan_is_paused(chan))
                        break;

                udelay(2);
        } while (--timeout);

        sysdma_chan_irq_disable(chan);

        chan->is_paused = true;

        spin_unlock_irqrestore(&chan->vchan.lock, flags);

        return timeout ? 0 : -EAGAIN;
}

static int k510_sysdma_chan_resume(struct dma_chan *dchan)
{
        struct k510_sysdma_chan *chan = dchan_to_k510_sysdma_chan(dchan);
        unsigned long flags;

        spin_lock_irqsave(&chan->vchan.lock, flags);

        if (chan->is_paused){
		sysdma_chan_irq_enable(chan);
                sysdma_chan_resume(chan);
	}

	chan->is_paused = false;

        spin_unlock_irqrestore(&chan->vchan.lock, flags);

        return 0;
}

static int k510_sysdma_chan_alloc_chan_resources(struct dma_chan *dchan)
{
        struct k510_sysdma_chan *chan = dchan_to_k510_sysdma_chan(dchan);

        /* ASSERT: channel is idle */
        if (!syadma_chan_is_idle(chan)) {
                dev_err(chan2dev(chan), "%s is non-idle!\n",
                        sysdma_chan_name(chan));
                return -EBUSY;
        }

        dev_vdbg(chan2dev(chan), "%s: allocating\n", sysdma_chan_name(chan));

        return 0;
}

static void k510_sysdma_chan_free_chan_resources(struct dma_chan *dchan)
{
        struct k510_sysdma_chan *chan = dchan_to_k510_sysdma_chan(dchan);

        /* ASSERT: channel is idle */
        if (!syadma_chan_is_idle(chan))
                dev_err(chan2dev(chan), "%s is non-idle!\n",
                        sysdma_chan_name(chan));

        sysdma_chan_stop(chan);
        sysdma_chan_irq_disable(chan);

        vchan_free_chan_resources(&chan->vchan);

        dev_vdbg(chan2dev(chan),
                 "%s: free resources, descriptor still allocated: %u\n",
                 sysdma_chan_name(chan), atomic_read(&chan->descs_allocated));

}

static int parse_device_properties(struct k510_sysdma_dev *pdev)
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

        return 0;
}


static int k510_sysdma_probe(struct platform_device *pdev)
{
	struct k510_sysdma_dev *priv;
	struct resource *res;
	int i, j, ret;
	
	printk("k510_sysdma_probe begin");

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

        priv->chans = devm_kcalloc(priv->dev, priv->nr_channels,
                                sizeof(struct k510_sysdma_chan), GFP_KERNEL);
        if (!priv->chans)
                return -ENOMEM;

	INIT_LIST_HEAD(&priv->slave.channels);
        for (i = 0; i < priv->nr_channels; i++) {
                struct k510_sysdma_chan *chan = &priv->chans[i];

                chan->pdev = priv;
                chan->id = i;
                chan->ch_regs = priv->base + CH0_BASE + i * CH_OFF;
                atomic_set(&chan->descs_allocated, 0);

                chan->vchan.desc_free = vchan_desc_put;
                vchan_init(&chan->vchan, &priv->slave);
        }

	platform_set_drvdata(pdev, priv);
	spin_lock_init(&priv->lock);

	dma_cap_zero(priv->slave.cap_mask);
	dma_cap_set(DMA_MEMCPY, priv->slave.cap_mask);

	priv->slave.chancnt			= priv->nr_channels;
        priv->slave.src_addr_widths = DMA_SLAVE_BUSWIDTH_8_BYTES;
        priv->slave.dst_addr_widths = DMA_SLAVE_BUSWIDTH_8_BYTES;
        priv->slave.directions = BIT(DMA_MEM_TO_MEM);
        priv->slave.residue_granularity = DMA_RESIDUE_GRANULARITY_DESCRIPTOR;


	priv->slave.device_tx_status		= k510_sysdma_tx_status;
	priv->slave.device_issue_pending	= k510_sysdma_issue_pending;
	priv->slave.device_terminate_all	= k510_sysdma_terminate_all;
        priv->slave.device_pause 		= k510_sysdma_chan_pause;
        priv->slave.device_resume 		= k510_sysdma_chan_resume;

        priv->slave.device_alloc_chan_resources	= k510_sysdma_chan_alloc_chan_resources;
        priv->slave.device_free_chan_resources	= k510_sysdma_chan_free_chan_resources;

	priv->slave.device_prep_dma_memcpy	= k510_sysdma_prep_dma_memcpy;

	priv->slave.dev = &pdev->dev;

	ret = clk_prepare_enable(priv->clk);
	if (ret) {
		dev_err(&pdev->dev, "Couldn't enable the clock\n");
		return ret;
	}

	/*
	 * Initialize sysdma controller
	 */

	k510_sysdma_hw_init(priv);

	ret = devm_request_irq(&pdev->dev, priv->irq, k510_sysdma_interrupt,
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


	dev_vdbg(&pdev->dev, "Successfully probed K510 sysdma controller, register mapped at %px(PHY %px) \n", res->start, priv->base);

	
	printk("k510_sysdma_probe end");

	return 0;

err_clk_disable:
	clk_disable_unprepare(priv->clk);
	return ret;
}

static int k510_sysdma_remove(struct platform_device *pdev)
{
	struct k510_sysdma_dev *priv = platform_get_drvdata(pdev);

	/* Disable IRQ so no more work is scheduled */
	disable_irq(priv->irq);

	of_dma_controller_free(pdev->dev.of_node);
	dma_async_device_unregister(&priv->slave);

	clk_disable_unprepare(priv->clk);

	return 0;
}

static const struct of_device_id k510_sysdma_match[] = {
	{ .compatible = "canaan,k510-sdma" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, k510_sysdma_match);

static struct platform_driver k510_sysdma_driver = {
	.probe	= k510_sysdma_probe,
	.remove	= k510_sysdma_remove,
	.driver	= {
		.name		= "k510-sysdma",
		.of_match_table	= k510_sysdma_match,
	},
};

module_platform_driver(k510_sysdma_driver);

MODULE_DESCRIPTION("K510 sysdma driver");
MODULE_LICENSE("GPL");
