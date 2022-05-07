/**********************************************************************
 * Copyright (C) 2014-2015 Cadence Design Systems, Inc.
 * All rights reserved worldwide.
 ***********************************************************************
 * cdn_core_wrapper.h
 ***********************************************************************/
struct wr_ep {
    struct wr_cusb          *cusb;
    u16                     max_packet;
    u8                      ep_num;
    CUSBD_Ep                *cd_ep;
    struct list_head        req_list;
    /* peripheral side */
    struct usb_ep           end_point;
    char                    name[12];
    u8                      is_tx; //if 1 then endpoint is used for transmiting data
    const struct usb_endpoint_descriptor    *desc;
    u8                      busy;
};


struct wr_cusb {
    /*device lock*/
    spinlock_t lock;
    struct platform_device  *platdev;
    int dma_irq;
    int usb_irq;

    CUSBH_OBJ               *cd_h_obj;
    CUSBH_Config            cd_h_config;
    CUSBH_Callbacks         cd_h_callback;
    CUSBH_SysReq            cd_h_sys_req; /*used for EH and OTG*/
    void                    *cd_h_priv;
    CUSBD_OBJ               *cd_g_obj;
    CUSBD_Config            cd_g_config;
    CUSBD_Callbacks         cd_g_callback;
    CUSBD_SysReq            cd_g_sys_req; /*used for device only*/
    CUSBD_Dev               *cd_g;
    void                    *cd_g_priv;
    u8                  ep0_data_stage_is_tx;
    u8                  is_host;
    struct wr_ep     endpoints_tx[16];
    struct wr_ep     endpoints_rx[16];
    u8               nr_endpoints;

    /**
     * Linux subsystem elements
     */
    void __iomem                *mregs;
    struct device               *controller;
    struct resource             *resource;
    struct usb_gadget           ls_g;           /* the gadget */
    struct usb_gadget_driver    *gadget_driver; /* its driver */
    struct usb_hcd              *hcd;           /* the usb hcd */
    /*
     * OTG controllers and transceivers need software interaction
     */
    struct usb_phy              *transceiver;
    struct phy                  *phy;
};

/* stored in "usb_host_endpoint.hcpriv" for scheduled endpoints */
struct wr_qh {
    struct usb_host_endpoint *hep;      /* usbcore info */
    struct usb_device   *dev;
    struct wr_ep    *hw_ep;     /* current binding */

    struct list_head    ring;       /* of musb_qh */
    CUSBH_Ep * cd_ep;
};

struct wr_usb_device {
    CUSBH_Ep cd_ep0_hep;
    CUSBH_Ep * cd_in_h_ep[16];
    CUSBH_Ep * cd_out_h_ep[16];
    CUSBH_Device cd_udev;
    struct usb_device * ld_udev;
};

inline struct wr_cusb *hcd_to_wr_cusb(struct usb_hcd *hcd)
{
    return *(struct wr_cusb **) hcd->hcd_priv;
}
