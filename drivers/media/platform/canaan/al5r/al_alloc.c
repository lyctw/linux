#include <linux/module.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include "al_alloc.h"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kevin Grandemange");
MODULE_AUTHOR("Sebastien Alaiwan");
MODULE_AUTHOR("Antoine Gruzelle");
MODULE_DESCRIPTION("Allegro Common");

struct al5_dma_buffer *al5_alloc_dma(struct device *dev, size_t size)
{
	struct al5_dma_buffer *buf =
		kmalloc(sizeof(struct al5_dma_buffer),
			GFP_KERNEL);

	if (!buf)
		return NULL;

	buf->size = size;
	buf->cpu_handle = dma_alloc_coherent(dev, buf->size,
					     &buf->dma_handle,
					     GFP_KERNEL | GFP_DMA);

	if (!buf->cpu_handle) {
		kfree(buf);
		return NULL;
	}

	return buf;
}

void al5_free_dma(struct device *dev, struct al5_dma_buffer *buf)
{
	if (buf)
		dma_free_coherent(dev, buf->size, buf->cpu_handle,
				  buf->dma_handle);
	kfree(buf);
}


