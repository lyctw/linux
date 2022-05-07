#include <linux/device.h>
#include "al_ip.h"

int al5_ioctl_get_dma_fd(struct device *dev, unsigned long arg);
int al5_ioctl_get_dmabuf_dma_addr(struct device *dev, unsigned long arg);
int al5_ioctl_get_dma_mmap(struct device *dev, struct al5r_codec_chan *chan,
			   unsigned long arg);

