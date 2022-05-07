/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/crypto.h>
#include <linux/spinlock.h>
#include <crypto/algapi.h>
#include <crypto/aes.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <crypto/internal/aead.h>
#include <crypto/internal/skcipher.h>
#include <crypto/scatterwalk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <crypto/algapi.h>
#include <crypto/aes.h>
#include <crypto/gcm.h>
#include <crypto/xts.h>
#include <crypto/internal/aead.h>
#include "kendryte-aes.h"
#include <linux/dmaengine.h>

#define STR_CBC_AES "cbc(aes)"
#define STR_ECB_AES "ecb(aes)"
#define STR_OFB_AES "ofb(aes)"
#define STR_CFB_AES "cfb(aes)"
#define STR_GCM_AES "gcm(aes)"

static int cryp_mode = 0;

struct gcm_sg_walk {
	struct scatter_walk walk;
	unsigned int walk_bytes;
	u8 *walk_ptr;
	unsigned int walk_bytes_remain;
	u8 buf[80];
	unsigned int buf_bytes;
	u8 *ptr;
	unsigned int nbytes;
};

struct kendryte_aes_info aes_info;

static void aes_write_aad(uint32_t aad_data, struct kendryte_aes_ctx *ctx)
{
    writel(aad_data, ctx->dev->regs+AES_AAD_DATA);
}

static void aes_write_text(uint32_t text_data, struct kendryte_aes_ctx *ctx)
{
    writel(text_data, ctx->dev->regs+AES_TEXT_DATA);
}

static void gcm_write_tag(uint32_t *tag, struct kendryte_aes_ctx *ctx)
{
    writel(tag[3], ctx->dev->regs+GCM_IN_TAG);
    writel(tag[2], ctx->dev->regs+GCM_IN_TAG+0x4);
    writel(tag[1], ctx->dev->regs+GCM_IN_TAG+0x8);
    writel(tag[0], ctx->dev->regs+GCM_IN_TAG+0xC);
}

static uint32_t aes_get_data_in_flag(struct kendryte_aes_ctx *ctx)
{
    return readl(ctx->dev->regs+DATA_IN_FLAG);
}

static uint32_t aes_get_data_out_flag(struct kendryte_aes_ctx *ctx)
{
    return readl(ctx->dev->regs+DATA_OUT_FLAG);
}

static uint32_t gcm_get_tag_in_flag(struct kendryte_aes_ctx *ctx)
{
    return readl(ctx->dev->regs+TAG_IN_FLAG);
}

static uint32_t aes_read_out_data(struct kendryte_aes_ctx *ctx)
{
    return readl(ctx->dev->regs+AES_OUT_DATA);
}

static uint32_t gcm_get_tag_chk(struct kendryte_aes_ctx *ctx)
{
    return readl(ctx->dev->regs+TAG_CHK);
}

static void gcm_clear_chk_tag(struct kendryte_aes_ctx *ctx)
{
    writel(0, ctx->dev->regs+TAG_CLEAR);
}

static uint32_t gcm_check_tag(uint32_t *gcm_tag, struct kendryte_aes_ctx *ctx)
{
    while(!gcm_get_tag_in_flag(ctx))
        cpu_relax();
    gcm_write_tag(gcm_tag, ctx);
    while(!gcm_get_tag_chk(ctx))
        cpu_relax();
    if(gcm_get_tag_chk(ctx) == 0x2)
    {
        gcm_clear_chk_tag(ctx);
        return 1;
    } else
    {
        gcm_clear_chk_tag(ctx);
        return 0;
    }
}

void aes_input_bytes(uint8_t *input_data, size_t input_data_len, struct kendryte_aes_ctx *ctx)
{
    size_t uint32_num, uint8_num, remainder, i;
    uint32_t uint32_data;
    uint8_t uint8_data[4] = {0};

    uint32_num = input_data_len / 4;
    for(i = 0; i < uint32_num; i++)
    {
        uint32_data = *((uint32_t *)(&input_data[i * 4]));
        while(!aes_get_data_in_flag(ctx))
        {
            cpu_relax();
        }
        aes_write_text(uint32_data, ctx);
    }

    uint8_num = 4 * uint32_num;
    remainder = input_data_len % 4;
    if(remainder)
    {
        switch(remainder)
        {
            case 1:
                uint8_data[0] = input_data[uint8_num];
                break;
            case 2:
                uint8_data[0] = input_data[uint8_num];
                uint8_data[1] = input_data[uint8_num + 1];
                break;
            case 3:
                uint8_data[0] = input_data[uint8_num];
                uint8_data[1] = input_data[uint8_num + 1];
                uint8_data[2] = input_data[uint8_num + 2];
                break;
            default:
                break;
        }
        uint32_data = *((uint32_t *)(&uint8_data[0]));
        while(!aes_get_data_in_flag(ctx))
            cpu_relax();
        aes_write_text(uint32_data, ctx);
    }
}

static int k510_aes_xmit_dma(struct kendryte_aes_info *aes_info,
			       struct scatterlist *sg, struct kendryte_aes_ctx *ctx)
{
	struct dma_async_tx_descriptor *in_desc;
	dma_cookie_t cookie;
	static u32 count = 0;
	int err;
    count++;

	in_desc = dmaengine_prep_slave_sg(aes_info->dma_lch, sg, 1,
					  DMA_DEV_TO_MEM, DMA_PREP_INTERRUPT |
					  DMA_CTRL_ACK);
	if (!in_desc) {
		dev_err(aes_info->dev, "dmaengine_prep_slave error\n");
		return -ENOMEM;
	}

	cookie = dmaengine_submit(in_desc);
	err = dma_submit_error(cookie);
	if (err)
		return -ENOMEM;

	dma_async_issue_pending(aes_info->dma_lch);
    aes_input_bytes(ctx->src, ctx->len, ctx);

    while(dma_async_is_tx_complete(aes_info->dma_lch, cookie, NULL, NULL) != DMA_COMPLETE);

	if (err) {
		dev_err(aes_info->dev, "DMA Error %i\n", err);
		dmaengine_terminate_all(aes_info->dma_lch);
		return err;
	}

    dma_release_channel(aes_info->dma_lch);  
}

static int k510_aes_dma_init(struct kendryte_aes_info *aes_info, struct kendryte_aes_ctx *ctx)
{
    struct dma_slave_config dma_conf;
	int err;

	memset(&dma_conf, 0, sizeof(dma_conf));
    writel(1, ctx->dev->regs+DMA_SEL);

	dma_conf.direction = DMA_DEV_TO_MEM;
    dma_conf.src_addr = 0x91000060;
	dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;

	aes_info->dma_lch = dma_request_slave_channel(aes_info->dev, "rx");
	if (!aes_info->dma_lch) {
		dev_err(aes_info->dev, "Couldn't acquire a slave DMA channel.\n");
		return -EBUSY;
	}

	err = dmaengine_slave_config(aes_info->dma_lch, &dma_conf);
	if (err) {
		dma_release_channel(aes_info->dma_lch);
		aes_info->dma_lch = NULL;
		dev_err(aes_info->dev, "Couldn't configure DMA slave.\n");
		return err;
	}

	return 0;
}

static void aes_process_with_dma(struct kendryte_aes_ctx *ctx)
{
    unsigned int padding_length;
    struct scatterlist *sg;
    dma_addr_t dma_handle;
    uint8_t *dma_virt_addr;

    sg = devm_kzalloc(ctx->dev->dev, sizeof(*sg), GFP_KERNEL);
    padding_length = ((ctx->len + 15) / 16) * 16;
    sg->length = padding_length;
    dma_virt_addr = dma_alloc_coherent(ctx->dev->dev, padding_length, &dma_handle, GFP_KERNEL);
    sg->dma_address = dma_handle;
    
    k510_aes_dma_init(&aes_info, ctx);
    k510_aes_xmit_dma(&aes_info, sg, ctx);
    memcpy(ctx->dst, dma_virt_addr, padding_length);
    dma_free_coherent(ctx->dev->dev, padding_length, dma_virt_addr, dma_handle);
}

static void process_less_80_bytes(uint8_t *input_data, uint8_t *output_data, size_t input_data_len, struct kendryte_aes_ctx *ctx)
{
    size_t uint32_num, uint8_num, remainder, i;
    uint32_t uint32_data;
    uint8_t uint8_data[4] = {0};

    uint32_num = input_data_len / 4;
    for(i = 0; i < uint32_num; i++)
    {
        uint32_data = *((uint32_t *)(&input_data[i * 4]));
        while(!aes_get_data_in_flag(ctx))
        {
            cpu_relax();
        }
        aes_write_text(uint32_data, ctx);
    }

    uint8_num = 4 * uint32_num;
    remainder = input_data_len % 4;
    if(remainder)
    {
        switch(remainder)
        {
            case 1:
                uint8_data[0] = input_data[uint8_num];
                break;
            case 2:
                uint8_data[0] = input_data[uint8_num];
                uint8_data[1] = input_data[uint8_num + 1];
                break;
            case 3:
                uint8_data[0] = input_data[uint8_num];
                uint8_data[1] = input_data[uint8_num + 1];
                uint8_data[2] = input_data[uint8_num + 2];
                break;
            default:
                break;
        }
        uint32_data = *((uint32_t *)(&uint8_data[0]));
        while(!aes_get_data_in_flag(ctx))
            cpu_relax();
        aes_write_text(uint32_data, ctx);
    }

    for(i = 0; i < uint32_num; i++)
    {
        while(!aes_get_data_out_flag(ctx))
        {
            cpu_relax();
        }
        *((uint32_t *)(&output_data[i * 4])) = aes_read_out_data(ctx);
    }
    if(remainder)
    {
        while(!aes_get_data_out_flag(ctx))
            cpu_relax();
        *((uint32_t *)(&uint8_data[0])) = aes_read_out_data(ctx);
        switch(remainder)
        {
            case 1:
                output_data[uint32_num * 4] = uint8_data[0];
                break;
            case 2:
                output_data[uint32_num * 4] = uint8_data[0];
                output_data[(uint32_num * 4) + 1] = uint8_data[1];
                break;
            case 3:
                output_data[uint32_num * 4] = uint8_data[0];
                output_data[(uint32_num * 4) + 1] = uint8_data[1];
                output_data[(uint32_num * 4) + 2] = uint8_data[2];
                break;
            default:
                break;
        }
    }
}

void gcm_get_tag(uint8_t *gcm_tag, struct kendryte_aes_ctx *ctx)
{
    uint32_t uint32_tag;
    uint8_t i = 0;

    uint32_tag = readl(ctx->dev->regs+GCM_OUT_TAG+0xC);
    gcm_tag[i++] = (uint8_t)((uint32_tag >> 24) & 0xff);
    gcm_tag[i++] = (uint8_t)((uint32_tag >> 16) & 0xff);
    gcm_tag[i++] = (uint8_t)((uint32_tag >> 8) & 0xff);
    gcm_tag[i++] = (uint8_t)((uint32_tag)&0xff);

    uint32_tag = readl(ctx->dev->regs+GCM_OUT_TAG+0x8);
    gcm_tag[i++] = (uint8_t)((uint32_tag >> 24) & 0xff);
    gcm_tag[i++] = (uint8_t)((uint32_tag >> 16) & 0xff);
    gcm_tag[i++] = (uint8_t)((uint32_tag >> 8) & 0xff);
    gcm_tag[i++] = (uint8_t)((uint32_tag)&0xff);

    uint32_tag = readl(ctx->dev->regs+GCM_OUT_TAG+0x4);
    gcm_tag[i++] = (uint8_t)((uint32_tag >> 24) & 0xff);
    gcm_tag[i++] = (uint8_t)((uint32_tag >> 16) & 0xff);
    gcm_tag[i++] = (uint8_t)((uint32_tag >> 8) & 0xff);
    gcm_tag[i++] = (uint8_t)((uint32_tag)&0xff);

    uint32_tag = readl(ctx->dev->regs+GCM_OUT_TAG);
    gcm_tag[i++] = (uint8_t)((uint32_tag >> 24) & 0xff);
    gcm_tag[i++] = (uint8_t)((uint32_tag >> 16) & 0xff);
    gcm_tag[i++] = (uint8_t)((uint32_tag >> 8) & 0xff);
    gcm_tag[i++] = (uint8_t)((uint32_tag)&0xff);
    if(ctx->total)
    {
        gcm_check_tag((uint32_t *)gcm_tag, ctx);
    }
}

void aes_init(struct kendryte_aes_ctx *ctx)
{
    size_t remainder, uint32_num, uint8_num, i;
    uint8_t uint8_data[4] = {0};
    uint32_t uint32_data;
    u32 mode_ctl;

    writel(0x1, ctx->dev->regs+AES_ENDIAN);

    for(i = 0; i < ctx->key_len>>2; i++)
    {
        if(i < 4)
            writel(*(uint32_t *)(ctx->key+ctx->key_len-(4 * i)-4), ctx->dev->regs+AES_KEY+4*i);
        else
            writel(*(uint32_t *)(ctx->key+ctx->key_len-(4 * i)-4), ctx->dev->regs+AES_KEY_EXT+4*(i-4));
    }

    for(i = 0; i < ctx->iv_len / 4; i++)
    {
        writel(*((uint32_t *)(ctx->iv+ctx->iv_len-(4 * i)-4)), ctx->dev->regs+AES_IV+4*i);
    }

    mode_ctl = readl(ctx->dev->regs+MODE_CTL);
    ((aes_mode_ctl_t *)&mode_ctl)->kmode = ctx->key_len/8 - 2;/* b'00:AES_128 b'01:AES_192 b'10:AES_256 b'11:RESERVED */
    ((aes_mode_ctl_t *)&mode_ctl)->cipher_mode = ctx->mode;
    writel(mode_ctl, ctx->dev->regs+MODE_CTL);

    writel(ctx->dir, ctx->dev->regs+ENCRYPT_SEL);

    writel(ctx->aad_len-1, ctx->dev->regs+GB_AAD_NUM);

    writel(ctx->total-1, ctx->dev->regs+GB_PC_NUM);

    writel(1, ctx->dev->regs+GB_AES_EN);
    
    if(ctx->mode == AES_GCM)
    {
        uint32_num = ctx->aad_len / 4;
        for(i = 0; i < uint32_num; i++)
        {
            uint32_data = *(uint32_t *)(ctx->aad + i*4);
            while(!aes_get_data_in_flag(ctx))
                cpu_relax();
            aes_write_aad(uint32_data, ctx);
        }
        uint8_num = 4 * uint32_num;
        remainder = ctx->aad_len % 4;
        if(remainder)
        {
            switch(remainder)
            {
                case 1:
                    uint8_data[0] = ctx->aad[uint8_num];
                    break;
                case 2:
                    uint8_data[0] = ctx->aad[uint8_num];
                    uint8_data[1] = ctx->aad[uint8_num + 1];
                    break;
                case 3:
                    uint8_data[0] = ctx->aad[uint8_num];
                    uint8_data[1] = ctx->aad[uint8_num + 1];
                    uint8_data[2] = ctx->aad[uint8_num + 2];
                    break;
                default:
                    break;
            }
            uint32_data = *((uint32_t *)(&uint8_data[0]));
            while(!aes_get_data_in_flag(ctx))
                cpu_relax();
            aes_write_aad(uint32_data, ctx);
        }
    }
}

void aes_process(struct kendryte_aes_ctx *ctx)
{
    size_t temp_len = 0;
    uint32_t i = 0;

    writel(0, ctx->dev->regs+DMA_SEL);
    if(ctx->len >= 80)
    {
        for(i = 0; i < (ctx->len / 80); i++)
            process_less_80_bytes(ctx->src + i*80, ctx->dst + i*80, 80, ctx);
    }
    temp_len = ctx->len % 80;
    if(temp_len)
        process_less_80_bytes(ctx->src + i*80, ctx->dst + i*80, temp_len, ctx);
}

static int kendryte_setkey(struct crypto_tfm *tfm, const u8 *key, unsigned int len)
{
	struct kendryte_aes_ctx *ctx = crypto_tfm_ctx(tfm);

	ctx->key_len = len;

	memcpy(ctx->key, key, len);
	return 0;
}

static int kendryte_aes_gcm_setkey(struct crypto_aead *tfm, const u8 *key, unsigned int keylen)
{
	struct kendryte_aes_ctx *ctx = crypto_aead_ctx(tfm);

	ctx->key_len = keylen;

	memcpy(ctx->key, key, keylen);
	return 0;
}

static void gcm_sg_walk_start(struct gcm_sg_walk *gw, struct scatterlist *sg,
			      unsigned int len)
{
	memset(gw, 0, sizeof(*gw));
	gw->walk_bytes_remain = len;
	scatterwalk_start(&gw->walk, sg);
}

static int gcm_sg_walk_go(struct gcm_sg_walk *gw, unsigned int minbytesneeded)
{
	int n;

	/* 锟斤拷0 锟斤拷pn锟� */
	if (gw->buf_bytes == minbytesneeded) {
		gw->ptr = gw->buf;
		gw->nbytes = gw->buf_bytes;
		goto out;
	}

	if (gw->walk_bytes_remain == 0) {
		gw->ptr = NULL;
		gw->nbytes = 0;
		goto out;
	}

	gw->walk_bytes = scatterwalk_clamp(&gw->walk, minbytesneeded);
	if (!gw->walk_bytes) {
		scatterwalk_start(&gw->walk, sg_next(gw->walk.sg));
		gw->walk_bytes = scatterwalk_clamp(&gw->walk,
						   minbytesneeded);
	}
    // pr_info("[%s:%d]sg len:%d sg off:%d\n", __FUNCTION__, __LINE__, gw->walk.sg->length, gw->walk.sg->offset);

	gw->walk_ptr = scatterwalk_map(&gw->walk);

	if (!gw->buf_bytes && gw->walk_bytes == minbytesneeded) {
		gw->ptr = gw->walk_ptr;
		gw->nbytes = gw->walk_bytes;
		goto out;
	}

	while (1) {
		n = min(gw->walk_bytes, minbytesneeded - gw->buf_bytes);
		memcpy(gw->buf + gw->buf_bytes, gw->walk_ptr, n);
		gw->buf_bytes += n;
		gw->walk_bytes_remain -= n;
		scatterwalk_unmap(&gw->walk);
		scatterwalk_advance(&gw->walk, n);
		scatterwalk_done(&gw->walk, 0, gw->walk_bytes_remain);

		if (gw->buf_bytes == minbytesneeded) {
			gw->ptr = gw->buf;
			gw->nbytes = gw->buf_bytes;
			goto out;
		}

		gw->walk_bytes = scatterwalk_clamp(&gw->walk,
						   minbytesneeded - gw->buf_bytes);
		if (!gw->walk_bytes) {
			scatterwalk_start(&gw->walk, sg_next(gw->walk.sg));
			gw->walk_bytes = scatterwalk_clamp(&gw->walk,
							minbytesneeded - gw->buf_bytes);
		}
        // pr_info("[%s:%d]sg len:%d sg off:%d\n", __FUNCTION__, __LINE__, gw->walk.sg->length, gw->walk.sg->offset);
		gw->walk_ptr = scatterwalk_map(&gw->walk);
	}

out:
	return gw->nbytes;
}

static void gcm_sg_walk_done(struct gcm_sg_walk *gw, unsigned int bytesdone)
{
	int n;

	if (gw->ptr == NULL)
		return;

	if (gw->ptr == gw->buf) {
		n = gw->buf_bytes - bytesdone;
		if (n > 0) {
			memmove(gw->buf, gw->buf + bytesdone, n);
			gw->buf_bytes -= n;
		} else
			gw->buf_bytes = 0;
	} else {
		gw->walk_bytes_remain -= bytesdone;
		scatterwalk_unmap(&gw->walk);
		scatterwalk_advance(&gw->walk, bytesdone);
		scatterwalk_done(&gw->walk, 0, gw->walk_bytes_remain);
	}
}

static int kendryte_do_crypt(struct blkcipher_desc *desc,
		  struct scatterlist *dst, struct scatterlist *src,
		  unsigned int nbytes, aes_encrypt_sel_t dir)
{
	struct kendryte_aes_ctx *ctx = crypto_blkcipher_ctx(desc->tfm);
	struct blkcipher_walk walk;
	int err;

	blkcipher_walk_init(&walk, dst, src, nbytes);
	err = blkcipher_walk_virt(desc, &walk);
    ctx->iv = walk.iv;
    ctx->iv_len = crypto_blkcipher_ivsize(desc->tfm);
    ctx->dir = dir;
    ctx->total = nbytes;

    mutex_lock(&ctx->dev->aes_mutex);
	aes_init(ctx);

	while ((nbytes = walk.nbytes)) {
		ctx->src = walk.src.virt.addr,
		ctx->dst = walk.dst.virt.addr;
		ctx->len = nbytes - (nbytes % AES_BLOCK_SIZE);

        if(cryp_mode == 0)
        {
		    aes_process(ctx);
        }
        else if(cryp_mode == 1)
        {
            aes_process_with_dma(ctx);
        }
		nbytes -= ctx->len;
		err = blkcipher_walk_done(desc, &walk, nbytes);
	}
    mutex_unlock(&ctx->dev->aes_mutex);
	return err;
}

static int kendryte_encrypt(struct blkcipher_desc *desc,
		  struct scatterlist *dst, struct scatterlist *src,
		  unsigned int nbytes)
{
    return kendryte_do_crypt(desc, dst, src, nbytes, AES_HARD_ENCRYPTION);
}

static int kendryte_decrypt(struct blkcipher_desc *desc,
		  struct scatterlist *dst, struct scatterlist *src,
		  unsigned int nbytes)
{
    return kendryte_do_crypt(desc, dst, src, nbytes, AES_HARD_DECRYPTION);
}

static int kendryte_aes_gcm_setauthsize(struct crypto_aead *aead, u32 authsize)
{
    switch (authsize) {
	case 16:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
* encrypt
*   req->src: aad||plaintext
*   req->dst: aad||ciphertext||tag
* decrypt
*   req->src: aad||ciphertext||tag
*   req->dst: aad||plaintext, return 0 or -EBADMSG
* aad, plaintext and ciphertext may be empty.
*/
static int kendryte_aes_gcm_crypt(struct aead_request *req, aes_encrypt_sel_t dir)
{
    int ret = 0;
    unsigned int len, in_bytes, out_bytes,
		     min_bytes, bytes, aad_bytes, pc_bytes;
	struct gcm_sg_walk gw_in, gw_out;
    u8 tag[16];

	struct crypto_aead *tfm = crypto_aead_reqtfm(req);
	struct kendryte_aes_ctx *ctx = crypto_aead_ctx(tfm);

	unsigned int aadlen = req->assoclen;
	unsigned int pclen = req->cryptlen;

    u32 *reset_reg = ioremap(0x97002000, 0x1000);
    u32 *aes_reset = reset_reg+(0x9C/4);

    mutex_lock(&ctx->dev->aes_mutex);
    *aes_reset |= (1 << 31);      /* clear done bit */

    *aes_reset |= (1 << 0);     /* set reset bit */
    while(!((*aes_reset) & (1 << 31)))
        ;
    iounmap(reset_reg);

	ctx->tag_len = crypto_aead_authsize(tfm);
	ctx->aad_len = req->assoclen;
    ctx->total = req->cryptlen;
    if(dir == AES_HARD_DECRYPTION)
    {
        ctx->total -= ctx->tag_len;
    }

    ctx->iv_len = crypto_aead_ivsize(tfm);
    ctx->iv = req->iv;

    ctx->dir = dir;

    // pr_info("[%s:%d]taglen:%d aad_len:%d total:%d iv_len:%d key_len:%d dir:%d\n", 
    //         __FUNCTION__, __LINE__, 
    //         ctx->tag_len, ctx->aad_len, ctx->total, ctx->iv_len, ctx->key_len, dir);

    if (dir == AES_HARD_DECRYPTION)
		pclen -= ctx->tag_len;
	len = aadlen + pclen;
    gcm_sg_walk_start(&gw_in, req->src, len);
	gcm_sg_walk_start(&gw_out, req->dst, len);

    // pr_info("[%s:%d]aadlen:%d pclen:%d \n", __FUNCTION__, __LINE__, aadlen, pclen);
    do {
		min_bytes = min_t(unsigned int,
				  aadlen > 0 ? aadlen : pclen, 80);
		in_bytes = gcm_sg_walk_go(&gw_in, min_bytes);
		out_bytes = gcm_sg_walk_go(&gw_out, min_bytes);
		bytes = min(in_bytes, out_bytes);
        // pr_info("[%s:%d]bytes:%d \n", __FUNCTION__, __LINE__, bytes);
		if (aadlen + pclen <= bytes) {
			aad_bytes = aadlen;
			pc_bytes = pclen;
		} else {
			if (aadlen <= bytes) {
				aad_bytes = aadlen;
				pc_bytes = (bytes - aadlen) > 80 ? 80:(bytes - aadlen);
			} else {
				aad_bytes = bytes;
				pc_bytes = 0;
			}
		}
        // pr_info("[%s:%d]aad_bytes:%d pc_bytes:%d\n", __FUNCTION__, __LINE__, aad_bytes, pc_bytes);
		if (aad_bytes > 0)
			memcpy(ctx->aad, gw_in.ptr, aad_bytes);

        if(!ctx->flags)
        {
            ctx->flags = 1;
            // pr_info("[%s:%d]aes_init: total:%d\n", __FUNCTION__, __LINE__, ctx->total);
            aes_init(ctx);
            // pr_info("[%s:%d]aes_init done\n", __FUNCTION__, __LINE__);
        }
        if(pc_bytes)
        {
            ctx->src = gw_in.ptr + aad_bytes;
            ctx->dst = gw_out.ptr + aad_bytes,
            ctx->len = pc_bytes;
            // pr_info("[%s:%d]aes_process: len:%d\n", __FUNCTION__, __LINE__, ctx->len);
            aes_process(ctx);
            // pr_info("[%s:%d]aes_process done\n", __FUNCTION__, __LINE__);

            if(gw_out.ptr == gw_out.buf)
            {
                scatterwalk_map_and_copy(ctx->dst, req->dst, ctx->aad_len+ctx->total-pclen, pc_bytes, 1);
            }
        }

		gcm_sg_walk_done(&gw_in, aad_bytes + pc_bytes);
		gcm_sg_walk_done(&gw_out, aad_bytes + pc_bytes);
		aadlen -= aad_bytes;
		pclen -= pc_bytes;
	} while (aadlen + pclen > 0);
    ctx->flags = 0;

    // pr_info("[%s:%d]gcm_get_tag\n", __FUNCTION__, __LINE__);
    gcm_get_tag(ctx->tag, ctx);
    // pr_info("[%s:%d]gcm_get_tag done\n", __FUNCTION__, __LINE__);
    // pr_info("[%s:%d]tag:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
    //         __FUNCTION__, __LINE__, 
    //         ctx->tag[0], ctx->tag[1], ctx->tag[2], ctx->tag[3], ctx->tag[4], ctx->tag[5], ctx->tag[6], ctx->tag[7]);
    if(dir == AES_HARD_DECRYPTION) 
    {
		scatterwalk_map_and_copy(tag, req->src, len, ctx->tag_len, 0);
		if (crypto_memneq(ctx->tag, tag, ctx->tag_len))
			ret = -EBADMSG;
	} else
		scatterwalk_map_and_copy(ctx->tag, req->dst, len, ctx->tag_len, 1);

    mutex_unlock(&ctx->dev->aes_mutex);
    
    return ret;
}

static int kendryte_aes_gcm_encrypt(struct aead_request *req)
{
    return kendryte_aes_gcm_crypt(req, AES_HARD_ENCRYPTION);
}

static int kendryte_aes_gcm_decrypt(struct aead_request *req)
{
    return kendryte_aes_gcm_crypt(req, AES_HARD_DECRYPTION);
}

static int kendryte_cra_init(struct crypto_tfm *tfm)
{
	struct kendryte_aes_ctx *ctx = crypto_tfm_ctx(tfm);
    struct crypto_alg *alg = tfm->__crt_alg;
    ctx->dev = &aes_info;

    if(!strcmp(alg->cra_name, STR_CBC_AES))
    {
        ctx->mode = AES_CBC;
    }
    else if(!strcmp(alg->cra_name, STR_ECB_AES))
    {
        ctx->mode = AES_ECB;
    }
    else if(!strcmp(alg->cra_name, STR_OFB_AES))
    {
        ctx->mode = AES_OFB;
    }
    else if(!strcmp(alg->cra_name, STR_CFB_AES))
    {
        ctx->mode = AES_CFB;
    }
    else if(!strcmp(alg->cra_name, STR_GCM_AES))
    {
        ctx->mode = AES_GCM;
    }

    return 0;
}

static void kendryte_cra_exit(struct crypto_tfm *tfm)
{
}

static int kendryte_aes_gcm_init(struct crypto_aead *tfm)
{
    return kendryte_cra_init(&tfm->base);
}

static struct crypto_alg kendryte_cbc_alg = {
	.cra_name		=	STR_CBC_AES,
	.cra_driver_name	=	"cbc-aes-kendryte",
	.cra_priority		=	400,
	.cra_flags		=	CRYPTO_ALG_TYPE_BLKCIPHER,
	.cra_blocksize		=	16,
	.cra_ctxsize		=	sizeof(struct kendryte_aes_ctx),
	.cra_alignmask		=	15,
	.cra_type		=	&crypto_blkcipher_type,
	.cra_module		=	THIS_MODULE,
    .cra_init		= kendryte_cra_init,
	.cra_exit		= kendryte_cra_exit,
	.cra_u			=	{
		.blkcipher = {
			.min_keysize		=	AES_MIN_KEY_SIZE,
			.max_keysize		=	AES_MAX_KEY_SIZE,
			.ivsize			=	AES_BLOCK_SIZE,
			.setkey	   		= 	kendryte_setkey,
			.encrypt		=	kendryte_encrypt,
			.decrypt		=	kendryte_decrypt,
		}
	}
};

static struct crypto_alg kendryte_ecb_alg = {
	.cra_name		=	STR_ECB_AES,
	.cra_driver_name	=	"ecb-aes-kendryte",
	.cra_priority		=	400,
	.cra_flags		=	CRYPTO_ALG_TYPE_BLKCIPHER,
	.cra_blocksize		=	16,
	.cra_ctxsize		=	sizeof(struct kendryte_aes_ctx),
	.cra_alignmask		=	15,
	.cra_type		=	&crypto_blkcipher_type,
	.cra_module		=	THIS_MODULE,
    .cra_init		= kendryte_cra_init,
	.cra_exit		= kendryte_cra_exit,
	.cra_u			=	{
		.blkcipher = {
			.min_keysize		=	AES_MIN_KEY_SIZE,
			.max_keysize		=	AES_MAX_KEY_SIZE,
			.ivsize			=	AES_BLOCK_SIZE,
			.setkey	   		= 	kendryte_setkey,
			.encrypt		=	kendryte_encrypt,
			.decrypt		=	kendryte_decrypt,
		}
	}
};

static struct crypto_alg kendryte_cfb_alg = {
	.cra_name		=	STR_CFB_AES,
	.cra_driver_name	=	"cfb-aes-kendryte",
	.cra_priority		=	400,
	.cra_flags		=	CRYPTO_ALG_TYPE_BLKCIPHER,
	.cra_blocksize		=	1,
	.cra_ctxsize		=	sizeof(struct kendryte_aes_ctx),
	.cra_type		=	&crypto_blkcipher_type,
	.cra_module		=	THIS_MODULE,
    .cra_init		= kendryte_cra_init,
	.cra_exit		= kendryte_cra_exit,
	.cra_u			=	{
		.blkcipher = {
			.min_keysize		=	AES_MIN_KEY_SIZE,
			.max_keysize		=	AES_MAX_KEY_SIZE,
			.ivsize			=	AES_BLOCK_SIZE,
			.setkey	   		= 	kendryte_setkey,
			.encrypt		=	kendryte_encrypt,
			.decrypt		=	kendryte_decrypt,
		}
	}
};

static struct crypto_alg kendryte_ofb_alg = {
	.cra_name		=	STR_OFB_AES,
	.cra_driver_name	=	"ofb-aes-kendryte",
	.cra_priority		=	400,
	.cra_flags		=	CRYPTO_ALG_TYPE_BLKCIPHER,
	.cra_blocksize		=	1,
	.cra_ctxsize		=	sizeof(struct kendryte_aes_ctx),
	.cra_type		=	&crypto_blkcipher_type,
	.cra_module		=	THIS_MODULE,
    .cra_init		= kendryte_cra_init,
	.cra_exit		= kendryte_cra_exit,
	.cra_u			=	{
		.blkcipher = {
			.min_keysize		=	AES_MIN_KEY_SIZE,
			.max_keysize		=	AES_MAX_KEY_SIZE,
			.ivsize			=	AES_BLOCK_SIZE,
			.setkey	   		= 	kendryte_setkey,
			.encrypt		=	kendryte_encrypt,
			.decrypt		=	kendryte_decrypt,
		}
	}
};

static struct aead_alg kendryte_gcm_alg = {
	.setkey		= kendryte_aes_gcm_setkey,
	.setauthsize	= kendryte_aes_gcm_setauthsize,
	.encrypt		=	kendryte_aes_gcm_encrypt,
	.decrypt		=	kendryte_aes_gcm_decrypt,
	.init		= kendryte_aes_gcm_init,
	.ivsize		= 12,
	.maxauthsize	= AES_BLOCK_SIZE,

	.base = {
		.cra_name		= STR_GCM_AES,
		.cra_driver_name	= "gcm-aes-canaan",
		.cra_priority		= 400,
		.cra_blocksize		= 1,
        .cra_flags		= CRYPTO_ALG_TYPE_AEAD,
		.cra_ctxsize		= sizeof(struct kendryte_aes_ctx),
		.cra_alignmask		= 0xf,
		.cra_module		= THIS_MODULE,
	},
};

static const struct of_device_id kendryte_aes_dt_ids[] = {
	{ .compatible = "canaan,k510-aes"},
	{}
};
MODULE_DEVICE_TABLE(of, kendryte_aes_dt_ids);

static int kendryte_aes_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	
	struct resource *res;
	int ret;

	aes_info.dev = dev;
    mutex_init(&aes_info.aes_mutex);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	aes_info.regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(aes_info.regs))
		return PTR_ERR(aes_info.regs);

	platform_set_drvdata(pdev, &aes_info);

	ret = crypto_register_alg(&kendryte_cbc_alg);
    if (ret < 0)
		goto cbc_err;
    ret = crypto_register_alg(&kendryte_ecb_alg);
    if (ret < 0)
		goto ecb_err;
    ret = crypto_register_alg(&kendryte_ofb_alg);
    if (ret < 0)
		goto ofb_err;
    ret = crypto_register_alg(&kendryte_cfb_alg);
    if (ret < 0)
		goto cfb_err;
    ret = crypto_register_aead(&kendryte_gcm_alg);
    if (ret < 0)
		goto gcm_err;
out:
    return ret;

gcm_err:
	crypto_unregister_alg(&kendryte_cfb_alg);
cfb_err:
	crypto_unregister_alg(&kendryte_ofb_alg);
ofb_err:
	crypto_unregister_alg(&kendryte_ecb_alg);
ecb_err:
	crypto_unregister_alg(&kendryte_cbc_alg);
cbc_err:
	printk(KERN_ERR "KENDRYTE AES initialization failed.\n");
	goto out;
}

static int kendryte_aes_remove(struct platform_device *pdev)
{
	crypto_unregister_alg(&kendryte_cbc_alg);
    crypto_unregister_alg(&kendryte_ecb_alg);
    crypto_unregister_alg(&kendryte_ofb_alg);
    crypto_unregister_alg(&kendryte_cfb_alg);
    crypto_unregister_aead(&kendryte_gcm_alg);

	return 0;
}

static struct platform_driver kendryte_aes_driver = {
	.probe          = kendryte_aes_probe,
	.remove         = kendryte_aes_remove,
	.driver         = {
		.name           = "canaan-aes",
		.of_match_table	= kendryte_aes_dt_ids,
	},
};

module_platform_driver(kendryte_aes_driver);

module_param(cryp_mode, int,  S_IRUGO | S_IWUSR);

MODULE_DESCRIPTION("CANAAN Hardware AES driver");
MODULE_LICENSE("GPL");
