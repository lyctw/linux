/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _KENDRYTE_HASH_H_
#define _KENDRYTE_HASH_H_

#define ENABLE_SHA (0x1)
#define SHA256_BIG_ENDIAN (0x1)
#define SHA256_DMA_EN (0X1)

#define SHA256_HASH_LEN 32
#define SHA256_HASH_WORDS 8
#define SHA256_BLOCK_LEN 64L

typedef struct _sha_num_reg
{
    /* sha data push threshold */
    uint32_t sha_data_th : 16;
    /* sha data push count */
    uint32_t sha_data_cnt : 16;
} __attribute__((packed, aligned(4))) sha_num_reg_t;

typedef struct _sha_function_reg_0
{
    /* write:SHA256 enable register. read:Calculation completed flag  */
    uint32_t sha_en : 1;
    uint32_t sha_otp_en : 1;
    uint32_t reserved00 : 6;
    /* SHA256 calculation overflow flag */
    uint32_t sha_overflow : 1;
    uint32_t reserved01 : 7;
    /* Endian setting; b'0:little endian b'1:big endian */
    uint32_t sha_endian : 1;
    uint32_t reserved02 : 15;
} __attribute__((packed, aligned(4))) sha_function_reg_0_t;

typedef struct _sha_function_reg_1
{
    /* Sha and DMA handshake signals enable.b'1:enable;b'0:disable */
    uint32_t dma_en : 1;
    uint32_t reserved10 : 7;
    /* b'1:sha256 fifo is full; b'0:not full */
    uint32_t fifo_in_full : 1;
    uint32_t reserved11 : 23;
} __attribute__((packed, aligned(4))) sha_function_reg_1_t;

#define SHA_RESULT          0X00
#define SHA_DATA_IN0        0X20
#define SHA_NUM_REG         0X28
#define SHA_FUNCTION_REG_0  0X2C
#define SHA_FUNCTION_REG_1  0X34
#define INIT_HASH           0X38

typedef struct _sha256
{
    /* Calculated sha256 return value. */
    uint32_t sha_result[8];
    /* SHA256 input data from this register. */
    uint32_t sha_data_in0;
    uint32_t reverse0;
    sha_num_reg_t sha_num_reg;
    sha_function_reg_0_t sha_function_reg_0;
    uint32_t double_sha;    
    sha_function_reg_1_t sha_function_reg_1;
    uint32_t init_hash[8];
} __attribute__((packed, aligned(4))) sha256_t;

typedef struct _sha256_context
{
    size_t total_len;
    size_t buffer_len;
    union
    {
        uint32_t words[16];
        uint8_t bytes[64];
    } buffer;
} sha256_context_t;

typedef struct sha256_cfg
{
    uint32_t* init_hash;
    bool opt_hash_en;
    bool double_sha_en;
    bool dma_en;
} sha256_cfg_t;

struct kendryte_sha256_info {
	struct device			*dev;
	struct clk			*clk;
	struct reset_control		*rst;
	void __iomem			*regs;
    struct mutex sha256_mutex;
	int 				err;
    struct dma_chan		*dma_lch;
};

struct kendryte_sha256_ctx {
	struct kendryte_sha256_info		*dev;
    sha256_context_t context;
    sha256_cfg_t cfg;
    u8 digest[32];
};


#endif