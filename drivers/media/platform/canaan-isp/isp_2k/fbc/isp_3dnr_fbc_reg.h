/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _ISP_FBC_REG_DEF_H_
#define _ISP_FBC_REG_DEF_H_

union U_ISP_FBC_INPUT_SIZE
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_input_height              : 13  ; /* [12..0 ]  */
        unsigned int    reserved0                      : 3   ; /* [15..13]  */
        unsigned int    fbc_input_width               : 13  ; /* [28..16]  */
        unsigned int    reserved1                      : 3   ; /* [31..29]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_OUT_FORMAT
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_out_format_cfg            : 1   ; /* [0 ]      */
        unsigned int    reserved0                      : 31  ; /* [31..1 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_Y_DATA_BUF_BASE0
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_y_data_buf_base0          : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_Y_DATA_BUF_BASE1
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_y_data_buf_base1          : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_Y_DATA_LINE_STRIDE
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_y_data_stride             : 15  ; /* [14..0 ]  */
        unsigned int    reserved0                      : 17  ; /* [31..15]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_Y_DATA_WR_BLEN
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_y_data_wr_blen            : 5   ; /* [4 ..0 ]  */
        unsigned int    reserved0                      : 27  ; /* [31..5 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_Y_HEAD_BUF_BASE0
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_y_head_buf_base0          : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_Y_HEAD_BUF_BASE1
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_y_head_buf_base1          : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_Y_HEAD_LINE_STRIDE
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_y_head_stride             : 15  ; /* [14..0 ]  */
        unsigned int    reserved0                      : 17  ; /* [31..15]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_Y_HEAD_WR_BLEN
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_y_head_wr_blen            : 5   ; /* [4 ..0 ]  */
        unsigned int    reserved0                      : 27  ; /* [31..5 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_UV_DATA_BUF_BASE0
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_uv_data_buf_base0         : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_UV_DATA_BUF_BASE1
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_uv_data_buf_base1         : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_UV_DATA_LINE_STRIDE
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_uv_data_stride            : 15  ; /* [14..0 ]  */
        unsigned int    reserved0                      : 17  ; /* [31..15]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_UV_DATA_WR_BLEN
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_uv_data_wr_blen           : 5   ; /* [4 ..0 ]  */
        unsigned int    reserved0                      : 27  ; /* [31..5 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_UV_HEAD_BUF_BASE0
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_uv_head_buf_base0         : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_UV_HEAD_BUF_BASE1
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_uv_head_buf_base1         : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_UV_HEAD_LINE_STRIDE
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_uv_head_stride            : 15  ; /* [14..0 ]  */
        unsigned int    reserved0                      : 17  ; /* [31..15]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_UV_HEAD_WR_BLEN
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_uv_head_wr_blen           : 5   ; /* [4 ..0 ]  */
        unsigned int    reserved0                      : 27  ; /* [31..5 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_YL_DATA_BUF_BASE0
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_yl_data_buf_base0         : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_YL_DATA_BUF_BASE1
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_yl_data_buf_base1         : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_YL_DATA_LINE_STRIDE
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_yl_data_stride            : 15  ; /* [14..0 ]  */
        unsigned int    reserved0                      : 17  ; /* [31..15]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_YL_DATA_WR_BLEN
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_yl_data_wr_blen           : 5   ; /* [4 ..0 ]  */
        unsigned int    reserved0                      : 27  ; /* [31..5 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_YL_HEAD_BUF_BASE0
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_yl_head_buf_base0         : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_YL_HEAD_BUF_BASE1
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_yl_head_buf_base1         : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_YL_HEAD_LINE_STRIDE
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_yl_head_stride            : 15  ; /* [14..0 ]  */
        unsigned int    reserved0                      : 17  ; /* [31..15]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBC_YL_HEAD_WR_BLEN
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbc_yl_head_wr_blen           : 5   ; /* [4 ..0 ]  */
        unsigned int    reserved0                      : 27  ; /* [31..5 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

//
#define  ISP_FBC_INPUT_SIZE     		(0x0000)
#define  ISP_FBC_OUT_FORMAT     		(0x0004)

#define  ISP_FBC_Y_DATA_BUF_BASE0    	(0x0010)
#define  ISP_FBC_Y_DATA_BUF_BASE1    	(0x0014)
#define  ISP_FBC_Y_DATA_LINE_STRIDE    (0x0018)
#define  ISP_FBC_Y_DATA_WR_BLEN     	(0x001C)

#define  ISP_FBC_Y_HEAD_BUF_BASE0     	(0x0020)
#define  ISP_FBC_Y_HEAD_BUF_BASE1     	(0x0024)
#define  ISP_FBC_Y_HEAD_LINE_STRIDE    (0x0028)
#define  ISP_FBC_Y_HEAD_WR_BLEN     	(0x002C)

#define  ISP_FBC_UV_DATA_BUF_BASE0     (0x0030)
#define  ISP_FBC_UV_DATA_BUF_BASE1     (0x0034)
#define  ISP_FBC_UV_DATA_LINE_STRIDE   (0x0038)
#define  ISP_FBC_UV_DATA_WR_BLEN       (0x003C)

#define  ISP_FBC_UV_HEAD_BUF_BASE0     (0x0040)
#define  ISP_FBC_UV_HEAD_BUF_BASE1     (0x0044)
#define  ISP_FBC_UV_HEAD_LINE_STRIDE   (0x0048)
#define  ISP_FBC_UV_HEAD_WR_BLEN     	(0x004C)

#define  ISP_FBC_YL_DATA_BUF_BASE0     (0x0050)
#define  ISP_FBC_YL_DATA_BUF_BASE1     (0x0054)
#define  ISP_FBC_YL_DATA_LINE_STRIDE   (0x0058)
#define  ISP_FBC_YL_DATA_WR_BLEN       (0x005C)

#define  ISP_FBC_YL_HEAD_BUF_BASE0     (0x0060)
#define  ISP_FBC_YL_HEAD_BUF_BASE1     (0x0064)
#define  ISP_FBC_YL_HEAD_LINE_STRIDE   (0x0068)
#define  ISP_FBC_YL_HEAD_WR_BLEN       (0x006C)

#endif /*_ISP_FBC_REG_DEF_H_*/

