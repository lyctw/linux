#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <asm/andesv5/proc.h>

extern void k510_sharemem_inval_range(unsigned long va_start, unsigned long pa_start, unsigned long size);
extern void k510_sharemem_wb_range(unsigned long va_start, unsigned long pa_start, unsigned long size);

#define SHARE_MEMORY_DEBUG

#ifdef SHARE_MEMORY_DEBUG
#define share_memory_log(format, ...) do {printk("k510-share-memory: " format, ##__VA_ARGS__);} while(0);
#else
#define share_memory_log(format, ...)
#endif

/* memory lib head */
typedef void *PART_ID;

typedef struct
{
    unsigned int numBytesFree;      /* Number of Free Bytes in Partition       */
    unsigned int numBlocksFree;     /* Number of Free Blocks in Partition      */
    unsigned int maxBlockSizeFree;  /* Maximum block size that is free.	      */
    unsigned int numBytesAlloc;     /* Number of Allocated Bytes in Partition  */
    unsigned int numBlocksAlloc;    /* Number of Allocated Blocks in Partition */
    unsigned int poolSize;          /* Number of Bytes in Partition (for R12C) */
}  MEM_PART_STATS;

/* memory lib c code */
#undef MEMLIB_DEBUG
#define PART_ID_MAGIC_NUM 0xAF50B9E7

/* block header */
struct header 
{
    struct
    {
        struct header *ptr;  /* next block if on free list */
        unsigned int size;  /* size of this block in unit of sizeof(Header) */
    } s;
};

typedef struct header Header;

typedef struct os_mem_part
{
    int magicNum;                   /* for verify it is a valid partId */
    char *pPool;                    /* the aligned start address */
    unsigned int poolSize;          /* pool size in Byte */
    unsigned int numBytesFree;      /* Number of free Bytes in this memPart */
    unsigned int numBytesAlloc;     /* Number of Allocated Bytes in this memPart */
    unsigned int numBlocksAlloc;    /* Number of Allocated Blocks in this memPart */
    Header *freep;                  /* start of free list */
} os_mem_part;

typedef os_mem_part *_PART_ID;

PART_ID memSysPartId;               /* memSysPartId will be created by application */

/* return True if the block start with pBlockStart is a free block. Otherwise return False. */
/* pBlockStart must be the start address of a block, otherwise the behavior is undefined. */
static bool isMemBlockFree(char *pBlockStart)
{
    Header *pHeader;
    if (pBlockStart == NULL)
    {
        return false;
    }
    pHeader = (Header *)pBlockStart - 1;

    if(NULL != pHeader->s.ptr) 
        return true;
    else
        return false;
}

/* create memory pool */
static PART_ID memPartCreate (char *pPool, unsigned poolSize)
{
    int alignOffset = 0;
    _PART_ID memPartId = NULL;

    /* kalloc memory pool struct */
    memPartId = (_PART_ID)kzalloc(sizeof(os_mem_part),GFP_KERNEL);
    if (NULL == memPartId)
    {
        share_memory_log("[ERROR] memPartCreate memPartCreate kzalloc error!\r\n");
        return NULL;
    }

    /* initialize the descriptor */
    alignOffset = ((int)(long)pPool + (int)sizeof(Header) - 1) / (int)sizeof(Header) * (int)sizeof(Header) - (int)(long)pPool; /* calculate the offset first. Fit for 32-bit or 64-bit system. */
    memPartId->magicNum = PART_ID_MAGIC_NUM;
    memPartId->pPool = pPool + alignOffset;
    memPartId->poolSize = poolSize - alignOffset;
    memPartId->numBytesFree = memPartId->poolSize;
    memPartId->numBytesAlloc = 0;
    memPartId->numBlocksAlloc = 0;
    memPartId->freep = (Header *)(memPartId->pPool);
    memPartId->freep->s.ptr = memPartId->freep;
    memPartId->freep->s.size = (memPartId->poolSize) / sizeof(Header);

#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] memPartCreate _pPool=0x%lx, _poolSize=%d\r\n", (long)pPool, poolSize);
    share_memory_log("[INFO] memPartCreate sizeof(Header)=%d, alignOffset=%d, pPool=0x%lx, poolSize=%d, pPoolEnd=0x%lx\r\n", (int)sizeof(Header), alignOffset, (long)memPartId->pPool, (int)memPartId->poolSize, (long)((long)memPartId->pPool + memPartId->poolSize));
#endif

    return (PART_ID)memPartId;
}

static void memPartDelete(PART_ID partId)
{
    if(NULL == partId)
        return;
    if(PART_ID_MAGIC_NUM != ((_PART_ID)partId)->magicNum)
        return;
    ((_PART_ID)partId)->magicNum = 0;
    kfree(partId);

    return;
}

static void *memPartAlloc (PART_ID partId, unsigned nBytes)
{
    Header *p           = NULL;
    Header *prevp       = NULL;
#ifdef MEMLIB_DEBUG
    Header *pTmp        = NULL;
#endif
    unsigned int nunits = 0;
    _PART_ID mpid       = NULL;
   
    mpid = (_PART_ID)partId;

    if(NULL == mpid)
    {
        share_memory_log("[ERROR] memPartAlloc partId is NULL!\r\n");
        return NULL;
    }

    if(mpid->magicNum != PART_ID_MAGIC_NUM)
    {
        share_memory_log("[ERROR] memPartAlloc magic is Error!\r\n");
        return NULL;
    }

    /* The partition must already be created with memPartCreate() */
    if(NULL == mpid->freep)
    {
        share_memory_log("[ERROR] memPartAlloc mpid->freep is NULL!\r\n");
        return NULL;
    }

    if (mpid->numBytesFree < nBytes)
    {
        share_memory_log("[ERROR] memPartAlloc mpid->numBytesFree less than nBytes!\r\n");
        return NULL;
    }

    prevp = mpid->freep;
    nunits = (nBytes + sizeof(Header) - 1) / sizeof(Header) + 1; /* including Header space */

    for (p = prevp->s.ptr; ; prevp = p, p = p->s.ptr)
    {
        if (p->s.size >= nunits)
        {   /* big enough */
            if (p->s.size == nunits)
            {
                /* exactly match */
                prevp->s.ptr = p->s.ptr;
            }
            else 
            {   /* allocate tail end */
                p->s.size -= nunits;
                p += p->s.size;
                p->s.size = nunits;
            }
            mpid->freep = prevp;
            mpid->numBytesFree -= nunits * sizeof(Header);
            mpid->numBytesAlloc += nunits * sizeof(Header);
            mpid->numBlocksAlloc += 1;
#ifdef MEMLIB_DEBUG
            share_memory_log("[INFO] memPartAlloc, phd=0x%lx, pdata=0x%lx, numBytesFree=%d\r\n", (long)(p), (long)(p + 1), mpid->numBytesFree);
            share_memory_log("\r\n[INFO] free list after alloc:\r\n");
            for (pTmp = mpid->freep->s.ptr; ; prevp = pTmp, pTmp = pTmp->s.ptr)
            {
                share_memory_log("[INFO] p=0x%lx, p->s.ptr=0x%lx, p->s.size=%d\r\n", (long)pTmp, (long)pTmp->s.ptr, pTmp->s.size);
                if (pTmp == mpid->freep)
                    break;
            }
#endif
            p->s.ptr = NULL; /* use this to mark this block as "not free" */
            return (void *)(p + 1);
        }

        if (p == mpid->freep)
        {   /* wrapped around free list means no space left */
            share_memory_log("[ERROR] memPartAlloc, nospace, numBytesFree=%d\r\n", mpid->numBytesFree);
            return NULL;
        }
    }

    return NULL;
}

static void *memPartAlignedAlloc (PART_ID partId, unsigned nBytes, unsigned alignment)
{
    Header *p = NULL;
    Header *prevp = NULL;
    Header *pTmp = NULL;
    unsigned int nunits = 0;
    unsigned int padding = 0;
    unsigned int offset = 0;
    _PART_ID mpid = NULL;

    mpid = (_PART_ID)partId;

    if(NULL == mpid)
    {
        share_memory_log("[ERROR] memPartAlignedAlloc partId is NULL!\r\n");
        return NULL;
    }
    if(mpid->magicNum != PART_ID_MAGIC_NUM)
    {
        share_memory_log("[ERROR] memPartAlignedAlloc magic is Error!\r\n");
        return NULL;
    }

    /* The partition must already be created with memPartCreate() */
    if(NULL == mpid->freep)
    {
        share_memory_log("[ERROR] memPartAlignedAlloc mpid->freep is NULL!\r\n");
        return NULL;
    }

    if (mpid->numBytesFree < nBytes)
    {
        share_memory_log("[ERROR] memPartAlignedAlloc mpid->numBytesFree less than nBytes!\r\n");
        return NULL;
    }

    prevp = mpid->freep;
    /* TODO: check alignment must be power of 2 */
    if (alignment > sizeof(Header))
    {
        if (alignment % sizeof(Header) != 0)
        {
            share_memory_log("[ERROR] memPartAlignedAlloc %d %% %d != 0 error!\r\n",alignment,(uint32_t)sizeof(Header));
            return NULL;
        }
    }
    
    (alignment <= sizeof(Header)) ? (padding = 0) : (padding = alignment - sizeof(Header));                 /* extra margin for alignment */
    nunits = (nBytes + padding + sizeof(Header) - 1) / sizeof(Header) + 1;                                  /* including Header space */
    
    for (p = prevp->s.ptr; ; prevp = p, p = p->s.ptr)
    {
        if (p->s.size >= nunits)
        {
            /* big enough */
            if (p->s.size == nunits)
            {
                /* exactly match */
                offset = ((alignment - ((unsigned int)(long)(p + 1) % alignment)) % alignment) / sizeof(Header);  /* offset in sizeof(Header) unit */
                if (offset != 0)
                {
                    p->s.ptr = prevp->s.ptr;
                    prevp->s.ptr = p;
                    p->s.size = offset;
                    p += offset;
                    p->s.size = nunits - offset;
                }
                else
                {
                    prevp->s.ptr = p->s.ptr;
                }
            }
            else
            {   /* allocate tail end */
                pTmp = p;
                p->s.size -= nunits;
                p += p->s.size;
                p->s.size = nunits;
                
                offset = ((alignment - ((unsigned int)(long)(p + 1) % alignment)) % alignment) / sizeof(Header);  /* offset in sizeof(Header) unit */
                
                if (offset != 0)
                {
                    pTmp->s.size += offset;
                    p += offset;
                    p->s.size = nunits - offset;
                }
            }
            
            mpid->freep = prevp;
            mpid->numBytesFree -= (nunits - offset) * sizeof(Header);
            mpid->numBytesAlloc += (nunits - offset) * sizeof(Header);
            mpid->numBlocksAlloc += 1;
#ifdef MEMLIB_DEBUG
            share_memory_log("\r\n====\r\n[INFO] memPartAlignedAlloc, phd=0x%lx, pdata=0x%lx, psize=%d, pdataModAlign=%d, numBytesFree=%d\r\n", 
            (long)(p), (long)(p + 1), p->s.size, ((int)(long)(p + 1))%alignment, mpid->numBytesFree);  
            share_memory_log("\r\n[INFO] free list after alloc:\r\n");
            for (pTmp = mpid->freep->s.ptr; ; prevp = pTmp, pTmp = pTmp->s.ptr)
            {
                share_memory_log("[INFO] p=0x%lx, p->s.ptr=0x%lx, p->s.size=%d\r\n", (long)pTmp, (long)pTmp->s.ptr, pTmp->s.size);
                if (pTmp == mpid->freep)
                    break;
            }
#endif
            p->s.ptr = NULL; // use this to mark this block as "not free"
            return (void *)(p + 1);
        }

        if (p == mpid->freep)
        {   // wrapped around free list means no space left
#ifdef MEMLIB_DEBUG
            share_memory_log("\r\n====\r\n[ERROR] memPartAlignedAlloc, nospace, numBytesFree=%d\r\n", mpid->numBytesFree);
#endif
            return NULL;
        }
    }

    return NULL;
}


/* If pBlock does not point to a block of memory allocated with memPartAlloc or memPartAlignedAlloc, it causes undefined behavior. */
static void memPartFree (PART_ID partId, char *pBlock)
{
    Header *bp;
    Header *p;
    unsigned int blockSize = 0; /* in Byte */
    _PART_ID mpid = NULL;

    mpid = (_PART_ID)partId;

    if(NULL == mpid)
    {
#ifdef MEMLIB_DEBUG
        share_memory_log("[ERROR]: memPartFree partId is NULL!\r\n");
#endif
        return;
    }

    if(mpid->magicNum != PART_ID_MAGIC_NUM)
    {
#ifdef MEMLIB_DEBUG
        share_memory_log("[ERROR]: memPartFree magic is Error!\r\n");
#endif
        return;
    }

    if (pBlock < mpid->pPool)
    {
#ifdef MEMLIB_DEBUG
        share_memory_log("[ERROR]: memPartFree pBlock address is less than partId pool address!\r\n");
#endif
        return;
    }

    if (pBlock >= mpid->pPool + mpid->poolSize)
    {
#ifdef MEMLIB_DEBUG
        share_memory_log("[ERROR]: memPartFree pBlock address is great than partId pool space!\r\n");
#endif
        return;
    }

    bp = (Header *)pBlock - 1; /* point to block header */
    if (bp->s.size * sizeof(Header) > mpid->poolSize)
    {
#ifdef MEMLIB_DEBUG
        share_memory_log("[ERROR]: memPartFree pBlock size is great than partId pool space!\r\n");
#endif
        return;
    }
    
    if (isMemBlockFree(pBlock))
    {
#ifdef MEMLIB_DEBUG
        share_memory_log("[ERROR]: memPartFree pBlock has been freed!\r\n");
#endif
        return;
    }

    blockSize = bp->s.size * sizeof(Header);

    if (mpid->freep->s.ptr == NULL)
    {
        /* now there is no space in this partition */
        /* bp will be the first free block */
        mpid->freep = bp;
        mpid->freep->s.ptr = bp;
        mpid->freep->s.size = bp->s.size;
    }
    else
    {
        for (p = mpid->freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
        {
            if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
            {   /* freed block at start or end */
                break;
            }
        }

        if (bp + bp->s.size == p->s.ptr) 
        {   /* join to upper address block */
            bp->s.size += p->s.ptr->s.size;
            if (p->s.ptr->s.ptr == p->s.ptr)
            {
                /* special case. p is the only one free block before free bp. */
                p = bp;
            }
            else
            {
                bp->s.ptr = p->s.ptr->s.ptr;
            }
        }
        else
        {
            bp->s.ptr = p->s.ptr;
        }
        
        if (p + p->s.size == bp) 
        {   /* join to lower address block */
            p->s.size += bp->s.size;
            p->s.ptr = bp->s.ptr;
        }
        else
        {
            p->s.ptr = bp;
        }
        
        mpid->freep = p;
    }

    mpid->numBytesFree += blockSize;
    mpid->numBytesAlloc -= blockSize;
    mpid->numBlocksAlloc -= 1;
    
#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] memPartFree, numBytesFree=%d\r\n", mpid->numBytesFree);
#endif
    return;
}

static void memPartInfoGet (PART_ID partId)
{
    Header *p = NULL;
    Header *prevp = NULL;
    Header *pTmp = NULL;
    _PART_ID mpid = (_PART_ID)partId;

    MEM_PART_STATS partStats;
    MEM_PART_STATS *ppartStats;

    ppartStats = &partStats;

    if(NULL == mpid)
    {
        share_memory_log("[ERROR]: memPartFree partId is NULL!\r\n");
        return;
    }

    if(mpid->magicNum != PART_ID_MAGIC_NUM)
    {
        share_memory_log("[ERROR]: memPartFree magic is Error!\r\n");
        return;
    }

    ppartStats->maxBlockSizeFree = 0;
    ppartStats->numBlocksFree = 0;

    for (p = mpid->freep->s.ptr; p != NULL; p = p->s.ptr)
    {
        ppartStats->numBlocksFree += 1;
        
        if (p->s.size > ppartStats->maxBlockSizeFree)
        {
            ppartStats->maxBlockSizeFree = p->s.size;
        }
        
        if (p == mpid->freep)
        {
            break;
        }
    }
    
    ppartStats->numBytesFree = mpid->numBytesFree;
    ppartStats->numBytesAlloc = mpid->numBytesAlloc;
    ppartStats->numBlocksAlloc = mpid->numBlocksAlloc;
    ppartStats->maxBlockSizeFree *= sizeof(Header);
    ppartStats->poolSize = mpid->poolSize;

    share_memory_log("memPartInfoGet, numBytesFree=%d\r\n",    ppartStats->numBytesFree);
    share_memory_log("memPartInfoGet, maxBlockSizeFree=%d\r\n",   ppartStats->maxBlockSizeFree);
    share_memory_log("memPartInfoGet, numBlocksFree=%d\r\n",      ppartStats->numBlocksFree);
    share_memory_log("memPartInfoGet, numBytesAlloc=%d\r\n",      ppartStats->numBytesAlloc);
    share_memory_log("memPartInfoGet, numBlocksAlloc=%d\r\n",     ppartStats->numBlocksAlloc);
    share_memory_log("memPartInfoGet, poolSize=%d\r\n\r\n",         ppartStats->poolSize);

    share_memory_log("\r\nmemPartInfoGet free list:\r\n");
    for (pTmp = mpid->freep->s.ptr; ; prevp = pTmp, pTmp = pTmp->s.ptr)
    {
        share_memory_log("memPartInfoGet p=0x%lx, p->s.ptr=0x%lx, p->s.size=%d\r\n", (long)pTmp, (long)pTmp->s.ptr, pTmp->s.size);
        if (pTmp == mpid->freep)
            break;
    }

    return;
}

#define SHARE_MEMORY_ALLOC          _IOWR('m', 1, unsigned long)
#define SHARE_MEMORY_ALIGN_ALLOC    _IOWR('m', 2, unsigned long)
#define SHARE_MEMORY_FREE           _IOWR('m', 3, unsigned long)
#define SHARE_MEMORY_SHOW           _IOWR('m', 4, unsigned long)
#define SHARE_MEMORY_INVAL_RANGE        _IOWR('m', 5, unsigned long)
#define SHARE_MEMORY_WB_RANGE           _IOWR('m', 6, unsigned long)

struct share_memory_plat {
    uint32_t        memory_phy_addr;    /* physical address */
    void __iomem    *memory_vir_addr;   /* virtual address */
    uint32_t        memory_sizes;
    PART_ID         part_id;
    int             major;
    int             minor;
    struct class    *class;
    struct device   *device;
    struct cdev     cdev;
};

static struct share_memory_plat *plat;

static DEFINE_MUTEX(share_memory_mutex);

struct share_memory_alloc_args {
    uint32_t size;
    uint32_t phyAddr;
};

struct memory_cache_sync_request {
    size_t size;
    uint64_t vaddr;
    uint64_t paddr;
};

static long share_memory_alloc(unsigned long arg)
{
    void *pBlockVirtual = NULL;
    uint32_t BlockPhysical = 0;
    struct share_memory_alloc_args alloc_args = {0,0};

    if(copy_from_user((void *)&alloc_args, (void *)arg, sizeof(struct share_memory_alloc_args))) {
        return -EFAULT;
    }

#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] share_memory_alloc share_memory_alloc size %d\r\n", (uint32_t)alloc_args.size);
#endif
    pBlockVirtual = memPartAlloc(plat->part_id, (uint32_t)alloc_args.size);

    /*virtual address to physical address*/
    BlockPhysical = plat->memory_phy_addr + (uint32_t)((long)pBlockVirtual - (long)plat->memory_vir_addr);
#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] share_memory_alloc virtual address:0x%lx, physical address:0x%08x\r\n",(long)pBlockVirtual,BlockPhysical);
#endif
    alloc_args.phyAddr = BlockPhysical;
    if(copy_to_user((void *)arg, (void *)&alloc_args, sizeof(struct share_memory_alloc_args)))
         return -EFAULT;

    if(0 == BlockPhysical) {
        return -EFAULT;
    }
    else {
        return 0;
    }
}

struct share_memory_alloc_align_args {
    uint32_t size;
    uint32_t alignment;
    uint32_t phyAddr;
};

static long share_memory_align_alloc(unsigned long arg)
{
    void *pBlockVirtual = NULL;
    uint32_t BlockPhysical = 0;
    struct share_memory_alloc_align_args alloc_args = {0,0,0};

    if(copy_from_user((void *)&alloc_args, (void *)arg, sizeof(struct share_memory_alloc_align_args)))
        return -EFAULT;

#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] share_memory_align_alloc size %d, alignment %d\r\n", alloc_args.size, alloc_args.alignment);
#endif

    pBlockVirtual = memPartAlignedAlloc(plat->part_id, (uint32_t)alloc_args.size, alloc_args.alignment);

    /*virtual address to physical address*/
    BlockPhysical = plat->memory_phy_addr + (uint32_t)((long)pBlockVirtual - (long)plat->memory_vir_addr);

#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] share_memory_align_alloc virtual address:0x%lx, physical address:0x%08x\r\n",(long)pBlockVirtual,BlockPhysical);
#endif

    alloc_args.phyAddr = BlockPhysical;
    if(copy_to_user((void *)arg, (void *)&alloc_args, sizeof(struct share_memory_alloc_align_args)))
         return -EFAULT;

    if(0 == BlockPhysical) {
        return -EFAULT;
    }
    else {
        return 0;
    }
}

static long share_memory_free(unsigned long arg)
{
    uint32_t BlockPhysical;
    char *pBlockVirtual;
    if(copy_from_user((void *)&BlockPhysical,(void *)arg, sizeof(uint32_t)))
        return -EFAULT;

    /*physical address to virtual address*/
    pBlockVirtual = (char *)(plat->memory_vir_addr + ((uint32_t)BlockPhysical - plat->memory_phy_addr));

#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] share_memory_free: physical address:0x%08x virtual address 0x%lx\r\n",BlockPhysical,(long)pBlockVirtual);
#endif

    memPartFree(plat->part_id, pBlockVirtual);
    return 0;
}

static long share_memory_show(unsigned long arg)
{
    memPartInfoGet(plat->part_id);
    return 0;
}

static long share_memory_inval_range(unsigned long arg)
{
    struct memory_cache_sync_request inval_range_args = {0, 0, 0};

    if(copy_from_user((void *)&inval_range_args, (void *)arg, sizeof(struct memory_cache_sync_request))) {
        return -EFAULT;
    }

    k510_sharemem_inval_range(inval_range_args.vaddr, inval_range_args.paddr, inval_range_args.size);

    return 0;

}

static long share_memory_wb_range(unsigned long arg)
{
    struct memory_cache_sync_request wb_range_args = {0, 0, 0};

    if(copy_from_user((void *)&wb_range_args, (void *)arg, sizeof(struct memory_cache_sync_request))) {
        return -EFAULT;
    }

    k510_sharemem_wb_range(wb_range_args.vaddr, wb_range_args.paddr, wb_range_args.size);

    return 0;
}

static long share_memory_ioctl(struct file *filp, unsigned int cmd,
                             unsigned long arg)
{
    switch (cmd) {
        case SHARE_MEMORY_ALLOC:
            return share_memory_alloc(arg);
        case SHARE_MEMORY_ALIGN_ALLOC:
            return share_memory_align_alloc(arg);
        case SHARE_MEMORY_FREE:
            return share_memory_free(arg);
        case SHARE_MEMORY_SHOW:
            return share_memory_show(arg);
	case SHARE_MEMORY_INVAL_RANGE:
	    return share_memory_inval_range(arg);
	case SHARE_MEMORY_WB_RANGE:
	    return share_memory_wb_range(arg);
        default:
            share_memory_log("[ERROR] share_memory_ioctl: Unknown ioctl: 0x%.8X\r\n", cmd);
            return -EINVAL;
    }
}

static long share_memory_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret;

    mutex_lock(&share_memory_mutex);
    ret = share_memory_ioctl(file, cmd, arg);
    mutex_unlock(&share_memory_mutex);

    return ret;
}

static int share_memory_open(struct inode *inode, struct file *filp)
{
#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] share_memory_open success!\r\n");
#endif
    return 0;
}

static int share_memory_release(struct inode *inode, struct file *filp)
{
#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] share_memory_release success!\r\n");
#endif
    return 0;
}

const struct file_operations share_memory_fops = {
    .owner          = THIS_MODULE,
    .open           = share_memory_open,
    .release        = share_memory_release,
    .unlocked_ioctl = share_memory_unlocked_ioctl,
};

static int share_memory_probe(struct platform_device *pdev)
{
    struct resource *res;
    int err = 0;
    dev_t dev = 0;
    int devno;
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        share_memory_log("[ERROR] share_memory_probe get resource failed \r\n");
        err = -ENODEV;
        goto error;
    }

    plat = kzalloc(sizeof(struct share_memory_plat), GFP_KERNEL);
    if (!plat) {
        share_memory_log("[ERROR] share_memory_probe  kzalloc failed \r\n");
        err = -ENOMEM;
        goto error;
    }

    plat->major = 0;
    plat->minor = 0;

    err = alloc_chrdev_region(&dev, 0, 1, "k510-share-memory");
    if (err) {
        share_memory_log("[ERROR] share_memory_probe  can't get major %d\r\n", plat->major);
        goto cleanup_kmem;
    }
    plat->major = MAJOR(dev);

    plat->class = class_create(THIS_MODULE, "k510_share_memory_class");
    if (IS_ERR(plat->class)) {
        err = PTR_ERR(plat->class);
        goto cleanup_cdev;
    }
    // create_module_class();
    devno = MKDEV(plat->major, plat->minor);

    cdev_init(&plat->cdev, &share_memory_fops);
    plat->cdev.owner = THIS_MODULE;
    err = cdev_add(&plat->cdev, devno, 1);
    if (err) {
        share_memory_log("[ERROR] share_memory_probe Error %d adding gnne device number %d \r\n", err, plat->minor);
        goto cleanup_class;
    }

    plat->device = device_create(plat->class, NULL, devno, NULL, "k510-share-memory");
    if (IS_ERR(plat->device)) {
        share_memory_log("[ERROR] share_memory_probe device NOT created\r\n");
        err = PTR_ERR(plat->device);
        goto cleanup_cdev;
    }

    plat->memory_sizes          = (uint32_t)res->end - (uint32_t)res->start + 1;
    plat->memory_phy_addr       = (uint32_t)res->start;
    plat->memory_vir_addr       = ioremap(res->start,plat->memory_sizes);

    if(NULL == plat->memory_vir_addr) {
        share_memory_log("[ERROR] share_memory_probe ioremap error!\r\n");
        goto cleanup_device;
    }
#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] share_memory_probe phy address 0x%08x virtual address 0x%lx size 0x%08x\r\n",plat->memory_phy_addr,(long)plat->memory_vir_addr, plat->memory_sizes);
#endif

    plat->part_id = memPartCreate((char *)plat->memory_vir_addr, plat->memory_sizes);
    if(NULL == plat->part_id) {
        share_memory_log("[ERROR] hare_memory_probe memPartCreate return failed!\r\n");
        goto cleanup_ioremap;
    }

#ifdef MEMLIB_DEBUG
    share_memory_log("share_memory_probe success!\r\n");
#endif

    return 0;

cleanup_ioremap:
    iounmap(plat->memory_vir_addr);
cleanup_device:
    device_destroy(plat->class, dev);
cleanup_class:
    class_destroy(plat->class);
cleanup_cdev:
    cdev_del(&plat->cdev);
cleanup_kmem:
    kfree(plat);
error:
	return err;
}

static int share_memory_remove(struct platform_device *pdev)
{
    dev_t dev = MKDEV(plat->major, plat->minor);

    device_destroy(plat->class, dev);
    cdev_del(&plat->cdev);
    class_destroy(plat->class);
    memPartDelete(plat->part_id);   /* Free memPart */
    kfree(plat);

    return 0;
}

static const struct of_device_id k510_share_memory_ids[] = {
    { .compatible = "k510-share-memory" },
    {}
};

static struct platform_driver k510_share_memory_driver = {
    .probe          = share_memory_probe,
    .remove         = share_memory_remove,
    .driver         = {
        .name           = "k510-share-memory",
        .of_match_table = of_match_ptr(k510_share_memory_ids),
    },
};

int share_memory_module_init(void)
{
    int ret;
    ret = platform_driver_register(&k510_share_memory_driver);
    return ret;
}

void share_memory_module_deinit(void)
{
    platform_driver_unregister(&k510_share_memory_driver);
}

module_init(share_memory_module_init);
module_exit(share_memory_module_deinit);
MODULE_LICENSE("GPL v2");
