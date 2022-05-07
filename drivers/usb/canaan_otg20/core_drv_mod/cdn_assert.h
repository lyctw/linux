#ifndef _INCLUDE_CDN_ASSERT_H_
#define _INCLUDE_CDN_ASSERT_H_

#define assert(expr) \
    do { \
        if (unlikely(!(expr))) { \
            printk(KERN_ERR "Assertion failed! %s,%s,%s,line=%d\n", \
              #expr, __FILE__, __func__, __LINE__); \
    } \
} while (0)

#endif //_INCLUDE_CDN_ASSERT_H_
