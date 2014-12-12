#ifndef __HOT_CACHE_STD_STDLIB_H__
#define __HOT_CACHE_STD_STDLIB_H__

#include <asm/atomic.h>
#include <sys/slab.h>

extern atomic_t g_memory_alloc_counter;

static inline void *rgos_malloc(size_t size)
{
    void *ptr;

    ptr = kmalloc(size, 0);
    if (ptr != NULL) {
        atomic_inc(&g_memory_alloc_counter);
    }

    return ptr;
}

static inline void *rgos_calloc(size_t nmemb, size_t size)
{
    size_t total_size = nmemb * size;
    void *ptr = kmalloc(total_size, 0);

    printk_rt("%s<%d>\r\n", __func__, __LINE__);

    if (ptr != NULL) {
        atomic_inc(&g_memory_alloc_counter);
        memset(ptr, 0, total_size);
    }

    return ptr;
}

static inline void rgos_free(void *ptr)
{
    atomic_dec(&g_memory_alloc_counter);
    kfree(ptr);
}

static inline int rgos_remove(char *path)
{
    return unlink(path);
}

#endif /* __HOT_CACHE_STD_STDLIB_H__ */

