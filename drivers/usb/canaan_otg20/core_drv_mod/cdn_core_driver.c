/*************************************************************\********
 * Copyright (C) 2014-2015 Cadence Design Systems, Inc.
 * All rights reserved worldwide.
 ***********************************************************************
 * cdn_core_driver.c
 *
 * Cadence USB Core Driver for Linux OS
 * This code implements package USB Core Driver into Module and export
 * only ***_GetInstance functions
 *
 * Core Driver is independent firmware that can be running without OS.
 *
 * The Cadence Core Driver architecture relies on an Object model to
 * provide access to "api" routines that can be used to perform different
 * functions required by the driver. For USB, this interface is
 * termed CUSBD (""Cadence USB Device Interface") or
 * CUSBH ("Cadence USB Host Interface").
 *
 * The CUSBD_OBJ struct is used to contain a list of apis for Device only
 * USB Controller. Additional details about this interface and associated
 * routines may be found in another places in the documentation
 * set that ships with the Core Driver.
 *
 * The CUSBH_OBJ struct is used to contain a list of apis for USB Embedded
 * Host controller and USB OTG Controller. Additional
 * details about this interface and associated routines may be found at
 * other places in the documentation set that ships with the Core Driver.
 *
 * The CUSBD_OBJ and CUSBH_OBJ struct and associated programming model includes the
 * use of a "probe" and an "init" routine (in addition to others) which
 * may be used by the current driver to get information about the amount
 * of memory required by the Core driver, etc.
 *
 ***********************************************************************/

#include <linux/module.h>

#include "cdn_errno.h"
#include "log.h"
#include "cdn_stdint.h"

#define DRIVER_DESC "Cadence USB Core Driver"

#include "sdudc.c"
#include "sduhcVHub.c"
#include "sduhc.c"
#include "sgdma.c"

/*For debug purpose*/
// unsigned int g_dbg_log_lvl = 0;
// unsigned int g_dbg_log_cnt = 0;
// unsigned int g_dbg_state = 0;
// unsigned int g_dbg_enable_log =1;

/**
 * Module information
 */
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("Cadence");
MODULE_AUTHOR("Cadence FW");
MODULE_ALIAS("platform:cdn_cusb_core_driver");

#define DRIVER_VERSION	"2014-09-21"

EXPORT_SYMBOL_GPL(CUSBD_GetInstance);

EXPORT_SYMBOL_GPL(CUSBH_GetInstance);

static int __init cdn_core_dirver_init (void) {
	DbgMsgSetLvl(0);
	DbgMsgEnableModule(0);
	return 0;
}

static void __exit cdn_core_driver_exit (void) {
}

/**
 * Module init and exit
 */
module_init(cdn_core_dirver_init);
module_exit(cdn_core_driver_exit);
