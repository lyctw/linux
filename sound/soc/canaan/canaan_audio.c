#include <linux/module.h>
#include <linux/platform_device.h>
#include "canaan_audio.h"
#include <asm/io.h> 

struct canaan_audio_data {
	struct platform_device *pdev;
	void __iomem *base;
};

static struct canaan_audio_data sai = {0};

void audio_i2s_in_init(void)
{
    if(sai.base)
    {
        volatile audio_in_reg_s  *audio_in_reg = sai.base;
        audio_in_reg->audio_in_agc_para_4.agc_bypass        = AUDIO_ENABLE;
    }
}

EXPORT_SYMBOL_GPL(audio_i2s_in_init);

void audio_i2s_out_init(uint32_t word_len)
{
    if(sai.base)
    {
        volatile audio_out_reg_s *audio_out_reg = sai.base + 0x800;
        audio_out_reg->audio_out_ctl.data_type              = (word_len == 24) ? AUDIO_OUT_TYPE_24BIT:AUDIO_OUT_TYPE_32BIT;
        audio_out_reg->audio_out_ctl.mode                   = AUDIO_OUT_MODE_I2S;      /* i2s/pdm/tdm mode */
        audio_out_reg->audio_out_ctl.enable                 = AUDIO_ENABLE;            /* enable audio out */
    }
}

EXPORT_SYMBOL_GPL(audio_i2s_out_init);

static int canaan_audio_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret = 0;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	sai.base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(sai.base))
		return PTR_ERR(sai.base);

    platform_set_drvdata(pdev, &sai);

	return ret;
}

static int canaan_audio_remove(struct platform_device *pdev)
{
	struct canaan_audio_data *priv = platform_get_drvdata(pdev);

	return 0;
}

static const struct of_device_id canaan_audio_ids[] = {
	{ .compatible = "canaan,k510-audio" },
	{ /* sentinel */ },
};

static struct platform_driver canaan_audio_driver = {
	.driver = {
		.name = "k510-audio",
		.of_match_table = canaan_audio_ids,
	},
	.probe = canaan_audio_probe,
	.remove = canaan_audio_remove,
};

module_platform_driver(canaan_audio_driver);

MODULE_DESCRIPTION("CANAAN AUDIO Interface");
MODULE_AUTHOR("JiangXB");
MODULE_LICENSE("GPL");