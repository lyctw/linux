#include <linux/device.h>
#include "al_alloc.h"

struct al5_buffer_info {
	u32 bus_address;
	u32 size;
};

int al5_create_dmabuf_fd(struct device *dev, unsigned long size,
			 struct al5_dma_buffer *buffer);
int al5_allocate_dmabuf(struct device *dev, int size, u32 *fd);
int al5_dmabuf_get_address(struct device *dev, u32 fd, u32 *bus_address);

