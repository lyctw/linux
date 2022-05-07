/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _MIPI_CORNER_REG_H_
#define _MIPI_CORNER_REG_H_

union U_MIPI_CORNER_CFG
{
    /* Define the struct bits */
    struct
    {
        unsigned int    bg_apd                          : 1  ; /* [0 ]  */
        unsigned int    bg_selin                        : 1  ; /* [1 ]  */
        unsigned int    pvtcal_start                    : 1  ; /* [2 ]  */
        unsigned int    rst_n                           : 1  ; /* [3]  */
        unsigned int    v2i_apd                         : 1  ; /* [4]  */
        unsigned int    bypass                          : 1  ; /* [5]  */
        unsigned int    pvt_resword_bypass              : 16  ; /* [21..6]  */
        unsigned int    clk_div                         : 8  ; /* [29..22]  */
        unsigned int    reservd0                        : 2  ; /* [31..30]  */
    } bits;    /* Define an unsigned member */

    unsigned int    udata;
};

union U_MIPI_CORNER_STA
{
    /* Define the struct bits */
    struct
    {
        unsigned int    pvt_resword_core                :16  ; /* [15..0 ]  */
        unsigned int    pvtcal_done                     :1  ; /* [16 ]  */
        unsigned int    reservd0                        :15  ; /* [31..17]  */
    } bits;    /* Define an unsigned member */

    unsigned int    udata;
};

#define MIPI_CORNER_CFG        (0x0000)
#define MIPI_CORNER_STA        (0x0004)

#endif /*_MIPI_CORNER_REG_H_*/