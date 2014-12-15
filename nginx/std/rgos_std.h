#ifndef __RGOS_STD_H__
#define __RGOS_STD_H__

#include <sys/print.h>

#define rgos_dbg(fmt, arg...) \
    do { \
        printk("*DEBUG* %-32s[%04d]: " fmt "\r\n", __func__, __LINE__, ##arg); \
    } while (0)

#define rgos_err(fmt, arg...) \
    do { \
        printk_rt("*ERROR* %-32s[%04d]: " fmt "\r\n", __func__, __LINE__, ##arg); \
    } while (0)

#endif /* __RGOS_STD_H__ */

