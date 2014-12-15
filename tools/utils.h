#include <stdint.h>

#if defined(__CYGWIN__)
#else
typedef uint32_t addr_t; /* Ŀ���豸�ĵ�ַָ������ */
#endif

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

