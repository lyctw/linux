/**********************************************************************
 * Copyright (C) 2014 Cadence Design Systems, Inc. 
 * All rights reserved worldwide.
 ***********************************************************************
 * cps.h
 * Interface for Cadence Platform Services (CPS), version 3
 *
 * This is the "hardware abstraction layer" upon which all drivers are built.
 * It must be implemented for each platform.
 ***********************************************************************/
#ifndef _CPS_H_
#define _CPS_H_

#include <asm/io.h>
#include "cdn_stdint.h"

/****************************************************************************
 * Types
 ***************************************************************************/

/** A lock handle */
typedef void* CPS_LockHandle;

/****************************************************************************
 * Prototypes
 ***************************************************************************/

/**
 * Check that sufficient locks are available
 * @param[in] lockCount number of locks requested
 * @return 0 on success (locks available)
 * @return ENOENT if insufficient locks are available
 */
static inline uint32_t CPS_ProbeLocks(uint32_t lockCount) {
    return 0;
}

/**
 * Initialize a lock
 * @param[out] lock where to store the allocated, initialized lock
 * @return 0 on success (lock is allocated and initialized)
 * @return ENOENT if insufficient locks are available
 */
static inline uint32_t CPS_InitLock(CPS_LockHandle* lock) {
    return 0;
}

/**
 * Free a lock
 * @param[in] lock the lock
 */
static inline void CPS_FreeLock(CPS_LockHandle lock) {
    return;
}

/**
 * Lock a lock, pending the current thread/task if necessary until the lock is available
 * @param[in] lock the lock
 */
static inline uint32_t CPS_Lock(CPS_LockHandle lock) {
    return 0;
}

/**
 * Unlock a lock, readying the next highest-priority thread/task pended on it if any
 * @param[in] lock the lock
 */
static inline uint32_t CPS_Unlock(CPS_LockHandle lock) {
    return 0;
}

/**
 * Read a (8-bit) word, bypassing the cache
 * @param[in] address the address
 * @return the word at the given address
 */
static inline uint8_t CPS_UncachedRead8(volatile uint8_t* address) {
	return ioread8((void __iomem *)address);
}

/**
 * Write a (8-bit) word to memory, bypassing the cache
 * @param[in] address the address
 * @param[in] value the word to write
 */
static inline void CPS_UncachedWrite8(volatile uint8_t* address, uint8_t value) {
	iowrite8(value, (void __iomem *)address);
}

/**
 * Read a (16-bit) word, bypassing the cache
 * @param[in] address the address
 * @return the word at the given address
 */
static inline uint16_t CPS_UncachedRead16(volatile uint16_t* address) {
	return ioread16((void __iomem *)address);
}

/**
 * Write a (16-bit) word to memory, bypassing the cache
 * @param[in] address the address
 * @param[in] value the word to write
 */
static inline void CPS_UncachedWrite16(volatile uint16_t* address, uint16_t value) {
	iowrite16(value, (void __iomem *)address);
}
/**
 * Read a (32-bit) word, bypassing the cache
 * @param[in] address the address
 * @return the word at the given address
 */
static inline uint32_t CPS_UncachedRead32(volatile uint32_t* address) {
	return ioread32((void __iomem *)address);
}

/**
 * Write a (32-bit) word to memory, bypassing the cache
 * @param[in] address the address
 * @param[in] value the word to write
 */
static inline void CPS_UncachedWrite32(volatile uint32_t* address, uint32_t value) {
	iowrite32(value, (void __iomem *)address);
}

#endif /* multiple inclusion protection */
