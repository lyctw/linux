// SPDX-License-Identifier: GPL-2.0

#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/types.h>
#include <asm/csr.h>
#include <asm/timex.h>
#include <asm/smp.h>
#include <asm/sbi.h>
#include <asm/io.h>

/*
uint64_t riscv_time_val;

uint64_t sbi_get_cycles(void)
{
    SBI_CALL(SBI_GET_CYCLES, virt_to_phys(&riscv_time_val), 0, 0);
    return riscv_time_val;
}
EXPORT_SYMBOL(sbi_get_cycles);
*/


DEFINE_PER_CPU(uint64_t, riscv_time_val);

uint64_t sbi_get_cycles(void)
{
    int ncpu = get_cpu();
    uint64_t *lp = &per_cpu(riscv_time_val, ncpu);
    SBI_CALL(SBI_GET_CYCLES, virt_to_phys(lp), 0, 0);
    put_cpu();
    return *lp;
}
EXPORT_SYMBOL(sbi_get_cycles);

