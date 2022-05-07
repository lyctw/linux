/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _MIPI_DPHY_REG_H_
#define _MIPI_DPHY_REG_H_

union U_MIPI_DPHY_RX_REG0
{
    /* Define the struct bits */
    struct
    {
        unsigned int    channelcfg                     : 1  ; /* [0 ]  */
        unsigned int    ClkHsMode                      : 1  ; /* [1 ]  */
        unsigned int    DeSkewEn                       : 1  ; /* [2 ]  */
        unsigned int    Polarity                       : 5  ; /* [8..3]  */
        unsigned int    t_clk_settle                   : 2  ; /* [10..9]  */
        unsigned int    t_clk_miss                     : 2  ; /* [12..11]  */
        unsigned int    t_hs_settle                    : 2  ; /* [14..13]  */
        unsigned int    t_term_en                      : 2  ; /* [16..15]  */
        unsigned int    Stdby_n                        : 1  ; /* [17]  */
        unsigned int    reserved0                      : 14 ; /* [31..18]  */
    } bits;    /* Define an unsigned member */

    unsigned int    udata;
} ;

union U_MIPI_DPHY_RX_REG1
{
    /* Define the struct bits */
    struct
    {
        unsigned int    madj_dll01                     : 8  ; /* [7..0]  */
        unsigned int    madj_dll23                     : 8  ; /* [15..8]  */
        unsigned int    adj_dll_c01_pos                : 8  ; /* [23..16]  */
        unsigned int    adj_dll_c01_neg                : 8  ; /* [31..24]  */
    } bits;    /* Define an unsigned member */

    unsigned int    udata;
} ;

union U_MIPI_DPHY_RX_REG2
{
    /* Define the struct bits */
    struct
    {
        unsigned int    adj_dll_c23_pos                : 8  ; /* [7..0]  */
        unsigned int    adj_dll_c23_neg                : 8  ; /* [15..8]  */
        unsigned int    adj_dll_data_ovrrde            : 4  ; /* [19..16]  */
        unsigned int    adj_dll_d0_ovrrd               : 8  ; /* [27..20]  */
        unsigned int    reserved0                      : 4  ; /* [31..28]  */
    } bits;    /* Define an unsigned member */

    unsigned int    udata;
} ;

union U_MIPI_DPHY_RX_REG3
{
    /* Define the struct bits */
    struct
    {
        unsigned int    adj_dll_d1_ovrrd               : 8  ; /* [7..0]  */
        unsigned int    adj_dll_d2_ovrrd               : 8  ; /* [15..8]  */
        unsigned int    adj_dll_d3_ovrrd               : 8  ; /* [23..16]  */
        unsigned int    RxDllReadyCnt                  : 2  ; /* [25..24]  */
        unsigned int    reserved0                      : 6  ; /* [31..26]  */
    } bits;    /* Define an unsigned member */

    unsigned int    udata;
} ;

union U_MIPI_DPHY_RX_REG4
{
    /* Define the struct bits */
    struct
    {
        unsigned int    DphyReady                      : 2  ; /* [1..0]  */
        unsigned int    RxClk0ActiveHS                 : 1  ; /* [2]  */
        unsigned int    RxClk1ActiveHS                 : 1  ; /* [3]  */
        unsigned int    RxTriggerEsc_ln0               : 4  ; /* [7..4]  */
        unsigned int    RxSkewFail_ln0                 : 1  ; /* [8]  */
        unsigned int    RxTriggerEsc_ln1               : 4  ; /* [12..9]  */
        unsigned int    RxSkewFail_ln1                 : 1  ; /* [13]  */
        unsigned int    RxTriggerEsc_ln2               : 4  ; /* [17..14]  */
        unsigned int    RxSkewFail_ln2                 : 1  ; /* [18]  */
        unsigned int    RxTriggerEsc_ln3               : 4  ; /* [22..19]  */
        unsigned int    RxSkewFail_ln3                 : 1  ; /* [23]  */
        unsigned int    reserved0                      : 8  ; /* [31..24]  */
    } bits;    /* Define an unsigned member */

    unsigned int    udata;
} ;

union U_MIPI_DPHY_RX_REG5
{
    /* Define the struct bits */
    struct
    {
        unsigned int    ErrSotHS                       : 4  ; /* [3..0]  */
        unsigned int    ErrCtrl                        : 6  ; /* [4..9]  */
        unsigned int    ErrEsc                         : 4  ; /* [13..10]  */
        unsigned int    reserved0                      : 18  ; /* [31..14]  */
    } bits;    /* Define an unsigned member */

    unsigned int    udata;
} ;

#define MIPI_DPHY_RX_REG0      (0x0000)
#define MIPI_DPHY_RX_REG1      (0x0004)
#define MIPI_DPHY_RX_REG2      (0x0008)
#define MIPI_DPHY_RX_REG3      (0x000C)
#define MIPI_DPHY_RX_REG4      (0x0010)
#define MIPI_DPHY_RX_REG5      (0x0014)

#endif /*_MIPI_DPHY_REG_H_*/