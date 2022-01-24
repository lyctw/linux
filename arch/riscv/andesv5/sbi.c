/*
 *  Copyright (C) 2020 Andes Technology Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <asm/andesv5/csr.h>
#include <asm/andesv5/proc.h>
#include <asm/sbi.h>

long sbi_get_marchid(void)
{
	struct sbiret ret;
	ret = sbi_ecall(SBI_EXT_BASE, SBI_EXT_BASE_GET_MARCHID, 0, 0, 0, 0, 0, 0);
	return ret.value;
}
EXPORT_SYMBOL(sbi_get_marchid);
