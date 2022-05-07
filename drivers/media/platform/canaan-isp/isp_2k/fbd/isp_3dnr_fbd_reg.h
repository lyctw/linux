/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _ISP_FBD_REG_DEF_H_
#define _ISP_FBD_REG_DEF_H_

union U_ISP_FBD_CTL
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_en                        : 1   ; /* [0 ]      */
        unsigned int    reserved0                      : 3   ; /* [3 ..1 ]  */
        unsigned int    fbd_format_cfg                : 1   ; /* [4 ]      */
        unsigned int    reserved1                      : 27  ; /* [31..5 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_INPUT_SIZE
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_input_height              : 13  ; /* [12..0 ]  */
        unsigned int    reserved0                      : 3   ; /* [15..13]  */
        unsigned int    fbd_input_width               : 13  ; /* [28..16]  */
        unsigned int    reserved1                      : 3   ; /* [31..29]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_Y_DATA_BUF_BASE0
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_y_data_buf_base0          : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_Y_DATA_BUF_BASE1
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_y_data_buf_base1          : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_Y_DATA_LINE_STRIDE
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_y_data_stride             : 15  ; /* [14..0 ]  */
        unsigned int    reserved0                      : 17  ; /* [31..15]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_Y_HEAD_BUF_BASE0
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_y_head_buf_base0          : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_Y_HEAD_BUF_BASE1
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_y_head_buf_base1          : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_Y_HEAD_LINE_STRIDE
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_y_head_stride             : 15  ; /* [14..0 ]  */
        unsigned int    reserved0                      : 17  ; /* [31..15]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_UV_DATA_BUF_BASE0
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_uv_data_buf_base0         : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_UV_DATA_BUF_BASE1
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_uv_data_buf_base1         : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_UV_DATA_LINE_STRIDE
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_uv_data_stride            : 15  ; /* [14..0 ]  */
        unsigned int    reserved0                      : 17  ; /* [31..15]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_UV_HEAD_BUF_BASE0
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_uv_head_buf_base0         : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_UV_HEAD_BUF_BASE1
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_uv_head_buf_base1         : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_UV_HEAD_LINE_STRIDE
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_uv_head_stride            : 15  ; /* [14..0 ]  */
        unsigned int    reserved0                      : 17  ; /* [31..15]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_YL_DATA_BUF_BASE0
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_yl_data_buf_base0         : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_YL_DATA_BUF_BASE1
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_yl_data_buf_base1         : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_YL_DATA_LINE_STRIDE
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_yl_data_stride            : 15  ; /* [14..0 ]  */
        unsigned int    reserved0                      : 17  ; /* [31..15]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_YL_HEAD_BUF_BASE0
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_yl_head_buf_base0         : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_YL_HEAD_BUF_BASE1
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_yl_head_buf_base1         : 32  ; /* [31..0 ]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

union U_ISP_FBD_YL_HEAD_LINE_STRIDE
{
    /* Define the struct bits */
    struct
    {
        unsigned int    fbd_yl_head_stride            : 15  ; /* [14..0 ]  */
        unsigned int    reserved0                      : 17  ; /* [31..15]  */
    } bits;    /* Define an unsigned member */

    unsigned int    u32;
} ;

//
#define  ISP_FBD_CTL     				(0x00)
#define  ISP_FBD_INPUT_SIZE     		(0x04)

#define  ISP_FBD_Y_DATA_BUF_BASE0     	(0x20)
#define  ISP_FBD_Y_DATA_BUF_BASE1     	(0x24)
#define  ISP_FBD_Y_DATA_LINE_STRIDE    (0x28)
#define  ISP_FBD_Y_HEAD_BUF_BASE0     	(0x2C)
#define  ISP_FBD_Y_HEAD_BUF_BASE1     	(0x30)
#define  ISP_FBD_Y_HEAD_LINE_STRIDE    (0x34)
#define  ISP_FBD_UV_DATA_BUF_BASE0     (0x38)
#define  ISP_FBD_UV_DATA_BUF_BASE1     (0x3C)
#define  ISP_FBD_UV_DATA_LINE_STRIDE   (0x40)
#define  ISP_FBD_UV_HEAD_BUF_BASE0     (0x44)
#define  ISP_FBD_UV_HEAD_BUF_BASE1     (0x48)
#define  ISP_FBD_UV_HEAD_LINE_STRIDE   (0x4C)
#define  ISP_FBD_YL_DATA_BUF_BASE0     (0x50)
#define  ISP_FBD_YL_DATA_BUF_BASE1     (0x54)
#define  ISP_FBD_YL_DATA_LINE_STRIDE   (0x58)
#define  ISP_FBD_YL_HEAD_BUF_BASE0     (0x5C)
#define  ISP_FBD_YL_HEAD_BUF_BASE1     (0x60)
#define  ISP_FBD_YL_HEAD_LINE_STRIDE   (0x64)

#endif  /*_ISP_FBD_REG_DEF_H_*/

