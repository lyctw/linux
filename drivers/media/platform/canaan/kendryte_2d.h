/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __KENDRYTE_2D_H__
#define __KENDRYTE_2D_H__


struct twod_data {
    
    unsigned int addr[4];
    unsigned int flag[4];
    int val;
};


extern unsigned long timer1 ;
extern unsigned long timer2 ;

void kendrty_set_rot_frame_run(int status);
void  kendrty_set_rot_frame_src_addr(unsigned int y_base, unsigned int u_base, unsigned int v_base);
void  kendrty_set_rot_frame_des_addr(unsigned int y_base, unsigned int u_base, unsigned int v_base);
int kendrty_get_frame_addr(struct twod_data dat);
void kendryte_set_fram_addr(unsigned int y_base, int cut);

#endif

