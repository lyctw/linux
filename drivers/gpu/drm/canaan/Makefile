# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the drm device driver.  This driver provides support for the
# Direct Rendering Infrastructure (DRI) in XFree86 4.1.0 and higher.

kendrytedrm-y := kendryte_fb.o kendryte_drv.o kendryte_vo.o  kendryte_crtc.o kendryte_layer.o			

kendrytedrm-$(CONFIG_KENDRYTE_CADENCE_MIPI_DSI) += kendryte_dsi.o

obj-$(CONFIG_DRM_KENDRYTE) += kendrytedrm.o



