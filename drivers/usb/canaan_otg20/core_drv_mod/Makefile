ccflags-y			:= -I$(srctree)/drivers/usb/canaan_otg20/include/

# Change 'obj-m' to 'obj-y' to build into the kernel:
obj-$(CONFIG_USB_CANAAN_OTG20) += cusbCoreDrv.o
cusbCoreDrv-objs := cdn_core_driver.o
