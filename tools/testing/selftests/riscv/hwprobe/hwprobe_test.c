/*
 * Doc: https://docs.kernel.org/riscv/hwprobe.html
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <asm/hwprobe.h>

/*
 * Rather than relying on having a new enough libc to define this, just do it
 * ourselves.  This way we don't need to be coupled to a new-enough libc to
 * contain the call.
 */
long riscv_hwprobe(struct riscv_hwprobe *pairs, size_t pair_count,
		   size_t cpu_count, unsigned long *cpus, unsigned int flags);

int main() {
    struct riscv_hwprobe pairs[] = {
        {RISCV_HWPROBE_KEY_MVENDORID, 0},
        {RISCV_HWPROBE_KEY_MARCHID, 0},
        {RISCV_HWPROBE_KEY_MIMPID, 0},
        {RISCV_HWPROBE_KEY_BASE_BEHAVIOR, 0},
        {RISCV_HWPROBE_KEY_IMA_EXT_0, 0},
        {RISCV_HWPROBE_KEY_CPUPERF_0, 0},
    };

    size_t pair_count = sizeof(pairs) / sizeof(struct riscv_hwprobe);
    size_t cpu_count = 0; // 0 for all online CPUs
    //cpu_set_t cpus;

    //CPU_ZERO(&cpus);

    long ret = riscv_hwprobe(pairs, pair_count, cpu_count, NULL, 0);
    if (ret < 0) {
        perror("riscv_hwprobe");
        exit(1);
    }

    printf("Vendor ID: %llx\n", pairs[0].value);
    printf("Architecture ID: %llx\n", pairs[1].value);
    printf("Implementation ID: %llx\n", pairs[2].value);

    unsigned long base_behavior = pairs[3].value;
    if (base_behavior & RISCV_HWPROBE_BASE_BEHAVIOR_IMA) {
        printf("Supports rv32ima or rv64ima\n");
    }

    unsigned long ima_ext_0 = pairs[4].value;
    if (ima_ext_0 & RISCV_HWPROBE_IMA_FD) {
        printf("Supports F and D extensions\n");
    }
    if (ima_ext_0 & RISCV_HWPROBE_IMA_C) {
        printf("Supports C extension\n");
    }
    if (ima_ext_0 & RISCV_HWPROBE_IMA_V) {
        printf("Supports V extension\n");
    }
    if (ima_ext_0 & RISCV_HWPROBE_EXT_ZBA) {
        printf("Supports Zba extension\n");
    }
    if (ima_ext_0 & RISCV_HWPROBE_EXT_ZBB) {
        printf("Supports Zbb extension\n");
    }
    if (ima_ext_0 & RISCV_HWPROBE_EXT_ZBS) {
        printf("Supports Zbs extension\n");
    }

    unsigned long cpuperf_0 = pairs[5].value;
    if (cpuperf_0 & RISCV_HWPROBE_MISALIGNED_UNKNOWN) {
        printf("Misaligned accesses performance: unknown\n");
    }
    if (cpuperf_0 & RISCV_HWPROBE_MISALIGNED_EMULATED) {
        printf("Misaligned accesses are emulated\n");
    }
    if (cpuperf_0 & RISCV_HWPROBE_MISALIGNED_SLOW) {
        printf("Misaligned accesses are slower\n");
    }
    if (cpuperf_0 & RISCV_HWPROBE_MISALIGNED_FAST) {
        printf("Misaligned accesses are faster\n");
    }
    if (cpuperf_0 & RISCV_HWPROBE_MISALIGNED_UNSUPPORTED) {
        printf("Misaligned accesses are not supported\n");
    }

    return 0;
}
