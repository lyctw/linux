#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/crypto.h>
#include <linux/spinlock.h>
#include <crypto/algapi.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <crypto/scatterwalk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <crypto/algapi.h>
#include <crypto/internal/hash.h>
#include <crypto/sha.h>
#include <linux/dmaengine.h>

#include "kendryte-hash.h"

#define DEBUG

struct kendryte_sha256_info sha256_info;

static int hash_mode = 0;

#define ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define BYTESWAP(x) ((ROTR((x), 8) & 0xff00ff00L) | (ROTL((x), 8) & 0x00ff00ffL))
#define BYTESWAP64(x) byteswap64(x)

static const uint8_t padding[64] =
    {
        0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static inline uint64_t byteswap64(uint64_t x)
{
    uint32_t a = (uint32_t)(x >> 32);
    uint32_t b = (uint32_t)x;
    return ((uint64_t)BYTESWAP(b) << 32) | (uint64_t)BYTESWAP(a);
}

static int k510_hash_xmit_dma(struct kendryte_sha256_info *sha256_info,
			       struct scatterlist *sg, int length, int mdma)
{
	struct dma_async_tx_descriptor *in_desc;
	dma_cookie_t cookie;
	static u32 count = 0;
	int err;
    count++;

	in_desc = dmaengine_prep_slave_sg(sha256_info->dma_lch, sg, 1,
					  DMA_MEM_TO_DEV, DMA_PREP_INTERRUPT |
					  DMA_CTRL_ACK);
	if (!in_desc) {
		dev_err(sha256_info->dev, "dmaengine_prep_slave error\n");
		return -ENOMEM;
	}

	//in_desc->callback = stm32_hash_dma_callback;
	//in_desc->callback_param = hdev;

	cookie = dmaengine_submit(in_desc);
	err = dma_submit_error(cookie);
	if (err)
		return -ENOMEM;

	dma_async_issue_pending(sha256_info->dma_lch);

    while(dma_async_is_tx_complete(sha256_info->dma_lch, cookie, NULL, NULL) != DMA_COMPLETE);

	if (err) {
		dev_err(sha256_info->dev, "DMA Error %i\n", err);
		dmaengine_terminate_all(sha256_info->dma_lch);
		return err;
	}

    dma_release_channel(sha256_info->dma_lch);  
}

static int k510_hash_dma_init(struct kendryte_sha256_info *sha256_info, struct kendryte_sha256_ctx *ctx)
{
    struct dma_slave_config dma_conf;
	int err;

	memset(&dma_conf, 0, sizeof(dma_conf));

	dma_conf.direction = DMA_MEM_TO_DEV;
    dma_conf.src_addr = &ctx->context.buffer.words[0];
    dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	//dma_conf.dst_addr = sha256_info->regs + SHA_DATA_IN0;
    dma_conf.dst_addr = 0x91010020;
    dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;


	sha256_info->dma_lch = dma_request_slave_channel(sha256_info->dev, "tx");
	if (!sha256_info->dma_lch) {
		dev_err(sha256_info->dev, "Couldn't acquire a slave DMA channel.\n");
		return -EBUSY;
	}

	err = dmaengine_slave_config(sha256_info->dma_lch, &dma_conf);
	if (err) {
		dma_release_channel(sha256_info->dma_lch);
		sha256_info->dma_lch = NULL;
		dev_err(sha256_info->dev, "Couldn't configure DMA slave.\n");
		return err;
	}

	return 0;

}

void sha256_init(struct kendryte_sha256_ctx *ctx, size_t input_len, sha256_cfg_t* cfg)
{
    uint32_t sha_data_th;
    pr_info("[%s:%d]input_len:%ld\n", __FUNCTION__, __LINE__, input_len);

    sha_data_th = (uint32_t)((input_len + SHA256_BLOCK_LEN + 8) / SHA256_BLOCK_LEN);
    writel(sha_data_th, ctx->dev->regs+SHA_NUM_REG);

    /* dma test */
    if(hash_mode == 1)
        writel((SHA256_DMA_EN), ctx->dev->regs+SHA_FUNCTION_REG_1);
    else if(hash_mode == 0)
    {
        writel((~SHA256_DMA_EN), ctx->dev->regs+SHA_FUNCTION_REG_1);
    }

    writel((SHA256_BIG_ENDIAN<<16)|(ENABLE_SHA), ctx->dev->regs+SHA_FUNCTION_REG_0);

    ctx->context.total_len = 0L;
    ctx->context.buffer_len = 0L;
}

void sha256_update(struct kendryte_sha256_ctx *ctx, const void *input, size_t input_len)
{
    const uint8_t *data = input;
    size_t buffer_bytes_left;
    size_t bytes_to_copy;
    uint32_t i, j;
    /* dma test */
    struct scatterlist *sg;
    dma_addr_t dma_handle;
    uint8_t *dma_virt_addr;
    sg = devm_kzalloc(ctx->dev->dev, sizeof(*sg), GFP_KERNEL);
    sg->length = 64;

    while(input_len)
    {
        buffer_bytes_left = SHA256_BLOCK_LEN - ctx->context.buffer_len;
        bytes_to_copy = buffer_bytes_left;
        if(bytes_to_copy > input_len)
            bytes_to_copy = input_len;
        memcpy(&ctx->context.buffer.bytes[ctx->context.buffer_len], data, bytes_to_copy);

        ctx->context.total_len += bytes_to_copy * 8L;
        ctx->context.buffer_len += bytes_to_copy;
        data += bytes_to_copy;
        input_len -= bytes_to_copy;
        if(ctx->context.buffer_len == SHA256_BLOCK_LEN)
        {
            if(hash_mode == 1)
            {
                dma_virt_addr = dma_alloc_coherent(ctx->dev->dev, 64, &dma_handle, GFP_KERNEL);
                sg->dma_address = dma_handle;
				memcpy(dma_virt_addr, &ctx->context.buffer.words[0], 64);
                
                /* dma test */
                k510_hash_dma_init(&sha256_info, ctx);
                k510_hash_xmit_dma(&sha256_info, sg, 1, 0);
                dma_free_coherent(ctx->dev->dev, 64, dma_virt_addr, dma_handle);
            }
            else if(hash_mode == 0)
            {
                for(i = 0; i < 16; i++)
                {
                    while(readl(ctx->dev->regs + SHA_FUNCTION_REG_1) & 0X100)
                        cpu_relax();
                    writel(ctx->context.buffer.words[i], ctx->dev->regs + SHA_DATA_IN0);
                }
            }


            ctx->context.buffer_len = 0L;
        }
    }

}

void sha256_final(struct kendryte_sha256_ctx *ctx, uint8_t *output)
{
    size_t bytes_to_pad;
    size_t length_pad;
    uint32_t i;

    bytes_to_pad = 120L - ctx->context.buffer_len;
    if(bytes_to_pad > 64L)
        bytes_to_pad -= 64L;
    length_pad = BYTESWAP64(ctx->context.total_len);

    sha256_update(ctx, padding, bytes_to_pad);
    sha256_update(ctx, &length_pad, 8L);

    while(!(readl(ctx->dev->regs + SHA_FUNCTION_REG_0) & 0x1))
        cpu_relax();
    if(output)
    {
        for(i = 0; i < SHA256_HASH_WORDS; i++)
        {
            *((uint32_t *)output) = readl(ctx->dev->regs + SHA_RESULT + (SHA256_HASH_WORDS - i - 1)*4);
            output += 4;
        }
    }
}

static int ahash_update(struct ahash_request *req)
{
	int ret = 0;
    struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct kendryte_sha256_ctx *ctx = crypto_ahash_ctx(tfm);

    u8 *data_buffer;
    struct crypto_hash_walk walk;
	int msg_length = crypto_hash_walk_first(req, &walk);

    while (0 != msg_length) {
		data_buffer = walk.data;
		sha256_update(ctx, data_buffer, msg_length);

		msg_length = crypto_hash_walk_done(&walk, 0);
	}

	return ret;
}

static int ahash_final(struct ahash_request *req)
{
	int ret = 0;
    struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct kendryte_sha256_ctx *ctx = crypto_ahash_ctx(tfm);

	sha256_final(ctx, req->result);

	return ret;
}

static int ahash_sha256_init(struct ahash_request *req)
{
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct kendryte_sha256_ctx *ctx = crypto_ahash_ctx(tfm);
    
    sha256_init(ctx, req->nbytes, &ctx->cfg);
    return 0;
}

static int ahash_sha256_digest(struct ahash_request *req)
{
	ahash_sha256_init(req);

	ahash_update(req);
	ahash_final(req);
    return 0;
}

static int ahash_noimport(struct ahash_request *req, const void *in)
{
	return 0;
}

static int ahash_noexport(struct ahash_request *req, void *out)
{
	return 0;
}

static int hash_cra_init(struct crypto_tfm *tfm)
{
	struct kendryte_sha256_ctx *ctx = crypto_tfm_ctx(tfm);

    ctx->dev = &sha256_info;
	return 0;
}


static struct ahash_alg kendryte_sha256 = {
    .init = ahash_sha256_init,
    .update	= ahash_update,
    .final = ahash_final,
    .digest = ahash_sha256_digest,
    .export = ahash_noexport,
    .import = ahash_noimport,
    .halg.digestsize = 32,
    .halg.statesize = sizeof(struct kendryte_sha256_ctx),
    .halg.base = {
        .cra_name = "sha256",
        .cra_driver_name = "sha256-kendryte",
        .cra_priority = 300,
        .cra_flags = (CRYPTO_ALG_TYPE_AHASH |
                    CRYPTO_ALG_ASYNC),
        .cra_blocksize = 64,
        .cra_ctxsize = sizeof(struct kendryte_sha256_ctx),
        .cra_type = &crypto_ahash_type,
        .cra_init = hash_cra_init,
        .cra_module = THIS_MODULE,
    }
};

static const struct of_device_id kendryte_sha256_dt_ids[] = {
	{ .compatible = "canaan,k510-sha256"},
	{}
};
MODULE_DEVICE_TABLE(of, kendryte_sha256_dt_ids);

static int kendryte_sha256_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	
	struct resource *res;
	int ret;

	sha256_info.dev = dev;
    mutex_init(&sha256_info.sha256_mutex);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	sha256_info.regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(sha256_info.regs))
		return PTR_ERR(sha256_info.regs);

	platform_set_drvdata(pdev, &sha256_info);

	ret = crypto_register_ahash(&kendryte_sha256);
    if (ret < 0)
	{
        printk(KERN_ERR "KENDRYTE SHA256 initialization failed.\n");
    }

    return ret;
}

static int kendryte_sha256_remove(struct platform_device *pdev)
{
	crypto_unregister_ahash(&kendryte_sha256);

	return 0;
}

static struct platform_driver kendryte_sha256_driver = {
	.probe          = kendryte_sha256_probe,
	.remove         = kendryte_sha256_remove,
	.driver         = {
		.name           = "canaan-sha256",
		.of_match_table	= kendryte_sha256_dt_ids,
	},
};

module_platform_driver(kendryte_sha256_driver);

module_param(hash_mode, int,  S_IRUGO | S_IWUSR);

MODULE_AUTHOR("Jiangxb.");
MODULE_DESCRIPTION("CANAAN Hardware SHA256 driver");
MODULE_LICENSE("GPL");
