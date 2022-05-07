/**********************************************************************
 * Copyright (C) 2014 Cadence Design Systems, Inc. 
 * All rights reserved worldwide.
 ***********************************************************************
 * log.h
 * System wide debug log messaging framework
 ***********************************************************************/

#ifndef _HAVE_DBG_LOG_

#define _HAVE_DBG_LOG_ 1
#include <linux/kernel.h>

#ifdef DEBUG
 #define CFP_DBG_MSG 1
#endif

/**
 * Modules definitions
 */
#define JXD_MSG            0x00000001
#define NFF_MSG            0x00000002
#define CLIENT_MSG         0x01000000
#define JX_MODEL_MSG       0x00000100
#define NVME_MODEL_MSG     0x00000200
#define CPS_OP             0x00200000

#define DBG_GEN_MSG        0xFFFFFFFF
#define DBG_FCD_MSG        0xFFFFFFFE
#define _HAVE_DBG_LOG_INT_
/* module mask: */
#ifdef _HAVE_DBG_LOG_INT_
unsigned int g_dbg_enable_log  = 0;
#else
extern unsigned int g_dbg_enable_log;
#endif

/* level, counter, state: */
#ifdef _HAVE_DBG_LOG_INT_
unsigned int g_dbg_log_lvl = 0;
unsigned int g_dbg_log_cnt = 0;
unsigned int g_dbg_state = 0;
#else
extern unsigned int g_dbg_log_lvl;
extern unsigned int g_dbg_log_cnt;
extern unsigned int g_dbg_state;
#endif


/**
 * Log level:
 * 0 - critical
 * 5 - warning
 * 10 - fyi
 * 100 - highly verbose
 * 200 - infinite loop debug
 */

/*
 * cDbgMsg is reserved for error messages that MUST be part of the binary even if
 * the debug subsystem is turned off. This is reserved for critical error messages.
 */
/*
#define cDbgMsg( t, x, ...) if ( ((x)==  0) || \
                                (((t) & g_dbg_enable_log) && ((x) <= g_dbg_log_lvl))) \
                                { printk( __VA_ARGS__); fflush(stdout); }
*/
/*
#define cDbgMsg( _t, _x, ...) ( ((_x)==  0) || \
                                (((_t) & g_dbg_enable_log) && ((_x) <= g_dbg_log_lvl)) ? \
                                (g_dbg_log_pointer+=((g_dbg_tmp_len=sprintf((char *)g_dbg_log_pointer, __VA_ARGS__))>0)?g_dbg_tmp_len:0): 0 )
*/
#define cDbgMsg( _t, _x, ...) ( ((_x)==  0) || \
                                (((_t) & g_dbg_enable_log) && ((_x) <= g_dbg_log_lvl)) ? \
                                printk(KERN_INFO __VA_ARGS__): 0 )


#ifdef CFP_DBG_MSG
#define DbgMsg( t, x, ...)  cDbgMsg( t, x, __VA_ARGS__ )
#else
#define DbgMsg( t, x, ...)
#endif
#define vDbgMsg( l, m, n, ...) DbgMsg( l, m, "[%-20.20s %4d %4d]-" n, __func__,\
                                                   __LINE__, g_dbg_log_cnt++, __VA_ARGS__)
#define cvDbgMsg( l, m, n, ...) cDbgMsg( l, m, "[%-20.20s %4d %4d]-" n, __func__,\
                                                   __LINE__, g_dbg_log_cnt++, __VA_ARGS__)
#define evDbgMsg( l, m, n, ...) { cDbgMsg( l, m, "[%-20.20s %4d %4d]-" n, __func__,         \
                                                   __LINE__, g_dbg_log_cnt++, __VA_ARGS__); \
                                  /*assert(0);*/ }

#define DbgMsgSetLvl( x ) (g_dbg_log_lvl = x)
#define DbgMsgEnableModule( x ) (g_dbg_enable_log |= (x) )
#define DbgMsgDisableModule( x ) (g_dbg_enable_log &= ~( (unsigned int) (x) ))
#define DbgMsgClearAll( _x ) ( g_dbg_enable_log = _x )

#define SetDbgState( _x ) (g_dbg_state = _x )
#define GetDbgState       (g_dbg_state)

//#define SetDbgLogPointer(_x) ( g_dbg_log_pointer = _x )

/*
 * Complex display elements
 */
#define vDbgPBar(l, m, n, o ) vDbgMsg( l, m, "[ %8d/%8d ]", n, o )
#define vDbgUpdatePBar(l, m, n, o ) DbgMsg( l, m, "[ %8d/%8d ]", n, o )
// #define vDbgUpdatePBar(l, m, n, o ) DbgMsg( l, m, "[ %8d/%8d ]\n", n, o )
#define vDbgComplPBar(l,m) DbgMsg(l, m, "\n");

#endif
