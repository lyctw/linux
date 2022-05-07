/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <sound/core.h>
#include <sound/jack.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "canaan_audio.h"

#define DRV_NAME "canaan-snd-nau8822"

static const struct snd_soc_dapm_widget canaan_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone", NULL),
	SND_SOC_DAPM_MIC("Headset Mic", NULL),
	SND_SOC_DAPM_MIC("Int Mic", NULL),
	SND_SOC_DAPM_SPK("Speaker", NULL),
};

static const struct snd_soc_dapm_route canaan_audio_map[] = {
	{"LMICP", 		NULL, "Headset Mic"},
	{"LMICN", 		NULL, "Headset Mic"},
	{"LMICP", 		NULL, "Mic Bias"},
	{"LMICN", 		NULL, "Mic Bias"},
	{"Headset Mic", NULL, "Mic Bias"},
	
	{"RMICP", 		NULL, "Int Mic"},
	{"RMICN", 		NULL, "Int Mic"},
	{"RMICP", 		NULL, "Mic Bias"},
	{"RMICN", 		NULL, "Mic Bias"},
	{"Int Mic", 	NULL, "Mic Bias"},
	
	{"Headphone", 	NULL, "LHP"},
	{"Headphone", 	NULL, "RHP"},
	
	{"Speaker", 	NULL, "LSPK"},
	{"Speaker", 	NULL, "RSPK"},
};

static const struct snd_kcontrol_new canaan_mc_controls[] = {
	SOC_DAPM_PIN_SWITCH("Headphone"),
	SOC_DAPM_PIN_SWITCH("Headset Mic"),
	SOC_DAPM_PIN_SWITCH("Int Mic"),
	SOC_DAPM_PIN_SWITCH("Speaker"),
};

static int canaan_nau8822_asoc_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int mclk;

	switch (params_rate(params)) {
	case 8000:
	case 16000:
	case 24000:
	case 32000:
	case 48000:
	case 64000:
	case 96000:
		mclk = 12288000;
		break;
	case 11025:
	case 22050:
	case 44100:
	case 88200:
		mclk = 11289600;
		break;
	default:
		return -EINVAL;
	}

	ret = snd_soc_dai_set_sysclk(codec_dai, 0, mclk,
				     SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(codec_dai->dev, "Can't set codec clock %d\n", ret);
		return ret;
	}

	return ret;
}

static const struct snd_soc_ops canaan_nau8822_ops = {
	.hw_params = canaan_nau8822_asoc_hw_params,
};

static struct snd_soc_jack canaan_nau8822_hp_jack;
static struct snd_soc_jack_pin canaan_nau8822_hp_jack_pins[] = {
	{
		.pin = "Headphones",
		.mask = SND_JACK_HEADPHONE,
	},
};

static struct snd_soc_jack_gpio canaan_nau8822_hp_jack_gpio = {
	.name = "Headphone detection",
	.report = SND_JACK_HEADPHONE,
	.debounce_time = 150,
	.invert = 1,
};

static int canaan_nau8822_asoc_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_card *card = rtd->card;
	// snd_soc_card_jack_new(rtd->card, "Headphones",
	// 				SND_JACK_HEADPHONE,
	// 				&canaan_nau8822_hp_jack,
	// 				canaan_nau8822_hp_jack_pins,
	// 				ARRAY_SIZE(canaan_nau8822_hp_jack_pins));
	// snd_soc_jack_add_gpios(&canaan_nau8822_hp_jack,
	// 				1,
	// 				&canaan_nau8822_hp_jack_gpio);
	snd_soc_dapm_force_enable_pin(&card->dapm, "Mic Bias");

	snd_soc_dapm_sync(&card->dapm);

	return 0;
}

static struct snd_soc_dai_link canaan_dailink = {
	.name = "nau8822",
	.stream_name = "Audio",
	.codec_dai_name = "nau8822-hifi",
	.init = canaan_nau8822_asoc_init,
	.ops = &canaan_nau8822_ops,
	/* set nau8822 as slave */
	.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
		SND_SOC_DAIFMT_CBS_CFS,
};

static struct snd_soc_card snd_soc_card_canaan = {
	.name = "CANAAN-I2S",
	.owner = THIS_MODULE,
	.dai_link = &canaan_dailink,
	.num_links = 1,
	.dapm_widgets = canaan_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(canaan_dapm_widgets),
	.dapm_routes = canaan_audio_map,
	.num_dapm_routes = ARRAY_SIZE(canaan_audio_map),
	.controls = canaan_mc_controls,
	.num_controls = ARRAY_SIZE(canaan_mc_controls),
};

static int canaan_nau8822_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct snd_soc_card *card = &snd_soc_card_canaan;
	struct device_node *np = pdev->dev.of_node;

	/* register the soc card */
	card->dev = &pdev->dev;

	canaan_dailink.codec_of_node = of_parse_phandle(np,
			"canaan,audio-codec", 0);
	if (!canaan_dailink.codec_of_node) {
		dev_err(&pdev->dev,
			"Property 'canaan,audio-codec' missing or invalid\n");
		return -EINVAL;
	}

	canaan_dailink.cpu_of_node = of_parse_phandle(np,
			"canaan,i2s-controller", 0);
	if (!canaan_dailink.cpu_of_node) {
		dev_err(&pdev->dev,
			"Property 'canaan,i2s-controller' missing or invalid\n");
		return -EINVAL;
	}

	canaan_dailink.platform_of_node = canaan_dailink.cpu_of_node;

	ret = snd_soc_of_parse_card_name(card, "canaan,model");
	if (ret) {
		dev_err(&pdev->dev,
			"Soc parse card name failed %d\n", ret);
		return ret;
	}

	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret) {
		dev_err(&pdev->dev,
			"Soc register card failed %d\n", ret);
		return ret;
	}
	audio_i2s_in_init();
	audio_i2s_out_init(32);

	return ret;
}

static const struct of_device_id canaan_nau8822_of_match[] = {
	{ .compatible = "canaan,canaan-audio-nau8822", },
	{},
};

MODULE_DEVICE_TABLE(of, canaan_nau8822_of_match);

static struct platform_driver canaan_nau8822_driver = {
	.probe = canaan_nau8822_probe,
	.driver = {
		.name = DRV_NAME,
		.pm = &snd_soc_pm_ops,
		.of_match_table = canaan_nau8822_of_match,
	},
};

module_platform_driver(canaan_nau8822_driver);

MODULE_AUTHOR("xubaikun");
MODULE_DESCRIPTION("CANAAN nau8822 machine ASoC driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRV_NAME);
