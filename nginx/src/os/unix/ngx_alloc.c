
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>

#include <asm/atomic.h>

atomic_t g_mem_counter_ngx_alloc_c = ATOMIC_INIT(0);

ngx_uint_t  ngx_pagesize;
ngx_uint_t  ngx_pagesize_shift;
ngx_uint_t  ngx_cacheline_size;

void *
ngx_alloc(size_t size, ngx_log_t *log)
{
    void  *p;

    p = rgos_malloc(size);
    if (p == NULL) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      "rgos_malloc(%uz) failed", size);
    } else {
        atomic_inc(&g_mem_counter_ngx_alloc_c);
    }

    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, log, 0, "rgos_malloc: %p:%uz", p, size);

    return p;
}


void *
ngx_calloc(size_t size, ngx_log_t *log)
{
    void  *p;

    p = ngx_alloc(size, log);
    ngx_dbg_mem_alloc(p);

    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}

void
ngx_free(void *ptr)
{
    ngx_dbg_mem_free(ptr);  /* ZHAOYAO XXX: 即使释放NULL, 这里也能给出debug信息 */

    if (ptr != NULL) {
        atomic_dec(&g_mem_counter_ngx_alloc_c);
        rgos_free(ptr);
    }
}


#if (NGX_HAVE_POSIX_MEMALIGN)

void *
ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
{

    void  *p;
/*
    int    err;

    err = posix_memalign(&p, alignment, size);

    if (err) {
        ngx_log_error(NGX_LOG_EMERG, log, err,
                      "posix_memalign(%uz, %uz) failed", alignment, size);
        p = NULL;
    }

    ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0,
                   "posix_memalign: %p:%uz @%uz", p, size, alignment);

    return p;
*/

    p = ngx_alloc(size, log);
    ngx_dbg_mem_alloc(p);
    return p;
}

#elif (NGX_HAVE_MEMALIGN)

void *
ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
{
    void  *p;

    p = memalign(alignment, size);
    if (p == NULL) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      "memalign(%uz, %uz) failed", alignment, size);
    }

    ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0,
                   "memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#endif
