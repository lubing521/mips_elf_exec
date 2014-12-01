#include <stdint.h>

typedef uint32_t addr_t; /* 目标设备的地址指针类型 */

#define BUG(fmt, arg...) \
    do { \
        fprintf(stderr, "*BUG* %s[%d]: " fmt "\n", __func__, __LINE__, ##arg); \
        exit(-1); \
    } while (0)

#define ERR(fmt, arg...) \
    do { \
        fprintf(stderr, "*ERR* %s[%d]: " fmt "\n", __func__, __LINE__, ##arg); \
    } while (0)

#define DBG(fmt, arg...) \
        do { \
            fprintf(stdout, "*DBG* : " fmt "\n", ##arg); \
        } while (0)

