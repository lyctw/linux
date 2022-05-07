/* Copyright (C) 2003-2006, Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _KENDRYTE_AES_H_
#define _KENDRYTE_AES_H_
#include <crypto/aes.h>
#include <crypto/algapi.h>

typedef struct aes_mode_ctl
{
    /* [2:0]:000:cfb; 001:cbc,010:gcm,011:cfb,100:ofb*/
    uint32_t cipher_mode : 3;
    /* [4:3]:00:aes-128; 01:aes-192; 10:aes-256;11:reserved*/
    uint32_t kmode : 2;
    /* [10:5]:aes_endian */
    uint32_t aes_endian : 6;
    /* [13:11]:aes_stream_mode */
    uint32_t aes_stream_mode : 3;
    uint32_t reserved : 18;
} __attribute__((packed, aligned(4))) aes_mode_ctl_t;

#define AES_KEY         0x00
#define ENCRYPT_SEL     0x10
#define MODE_CTL        0x14
#define AES_IV          0x18
#define AES_ENDIAN      0x28
#define AES_FINISH      0x2C
#define DMA_SEL         0x30
#define GB_AAD_NUM      0x34
#define GB_PC_NUM       0x3C
#define AES_TEXT_DATA   0x40
#define AES_AAD_DATA    0x44
#define TAG_CHK         0x48
#define DATA_IN_FLAG    0x4C
#define GCM_IN_TAG      0x50
#define AES_OUT_DATA    0x60
#define GB_AES_EN       0x64
#define DATA_OUT_FLAG   0x68
#define TAG_IN_FLAG     0X6C
#define TAG_CLEAR       0X70
#define GCM_OUT_TAG     0x74
#define AES_KEY_EXT     0x84

typedef struct aes
{
    /* (0x00) customer key.1st~4th byte key */
    uint32_t aes_key[4];
    /* (0x10) 0: encryption; 1: decryption */
    uint32_t encrypt_sel;
    /* (0x14) aes mode reg */
    aes_mode_ctl_t mode_ctl;
    /* (0x18) Initialisation Vector. GCM support 96bit. CBC support 128bit */
    uint32_t aes_iv[4];
    /* (0x28) input data endian;1:little endian; 0:big endian */
    uint32_t aes_endian;
    /* (0x2c) calculate status. 1:finish; 0:not finish */
    uint32_t aes_finish;
    /* (0x30) aes out data to dma 0:cpu 1:dma */
    uint32_t dma_sel;
    /* (0x34) gcm Additional authenticated data number */
    uint32_t gb_aad_num;
    uint32_t reserved;
    /* (0x3c) aes plantext/ciphter text input data number */
    uint32_t gb_pc_num;
    /* (0x40) aes plantext/ciphter text input data */
    uint32_t aes_text_data;
    /* (0x44) Additional authenticated data */
    uint32_t aes_aad_data;
    /**
     * (0x48) [1:0],b'00:check not finish; b'01:check fail; b'10:check success;
     * b'11:reversed
     */
    uint32_t tag_chk;
    /* (0x4c) data can input flag. 1: data can input; 0 : data cannot input */
    uint32_t data_in_flag;
    /* (0x50) gcm input tag for compare with the calculate tag */
    uint32_t gcm_in_tag[4];
    /* (0x60) aes plantext/ciphter text output data */
    uint32_t aes_out_data;
    /* (0x64) aes module enable */
    uint32_t gb_aes_en;
    /* (0x68) data can output flag 1: data ready 0: data not ready */
    uint32_t data_out_flag;
    /* (0x6c) allow tag input when use gcm */
    uint32_t tag_in_flag;
    /* (0x70) clear tag_chk */
    uint32_t tag_clear;
    uint32_t gcm_out_tag[4];
    /* (0x84) customer key for aes-192 aes-256.5th~8th byte key */
    uint32_t aes_key_ext[4];
} aes_t;

typedef enum aes_cipher_mode
{
    AES_ECB = 0,
    AES_CBC = 1,
    AES_GCM = 2,
    AES_CFB = 3,
    AES_OFB = 4,
    AES_CIPHER_MAX,
} aes_cipher_mode_t;

typedef enum aes_kmode
{
    AES_128 = 16,
    AES_192 = 24,
    AES_256 = 32,
} aes_kmode_t;

typedef enum aes_iv_len
{
    IV_LEN_96 = 12,
    IV_LEN_128 = 16,
} aes_iv_len_t;

typedef enum aes_encrypt_sel
{
    AES_HARD_ENCRYPTION = 0,
    AES_HARD_DECRYPTION = 1,
} aes_encrypt_sel_t;

typedef struct cbc_context
{
    uint8_t* input_key;
    uint8_t* iv;  // 128-bits
} cbc_context_t;

typedef struct gcm_context
{
    uint8_t* input_key;
    uint8_t* iv; // 96-bits
    uint8_t* gcm_aad;
    size_t gcm_aad_len;
} gcm_context_t;

typedef struct cfb_context
{
    uint8_t* input_key;
    uint8_t* iv;  // 128-bits
} cfb_context_t;

typedef struct ofb_context
{
    uint8_t* input_key;
    uint8_t* iv;  // 128-bits
} ofb_context_t;

struct kendryte_aes_info {
	struct device			*dev;
	struct clk			*clk;
	struct reset_control		*rst;
	void __iomem			*regs;
    struct mutex aes_mutex;
	int 				err;
    struct dma_chan		*dma_lch;
};

struct kendryte_aes_ctx {
	struct kendryte_aes_info		*dev;
    u8 *src;
	u8 *dst;

	u32 dir;
	u32 flags;
	int len;
    int total;
	unsigned int			key_len;
	u32				mode;
	u8 key[32];
	u8 *iv;
    u8 iv_len;
    u8 aad[32];
    u8 aad_len;
    u8 tag[16];
    u8 tag_len;
    u8 buffer[80];
};

#define GCM_AES_IV_SIZE 12
#endif
