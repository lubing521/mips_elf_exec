/*
 * Copyright(C) 2014 Ruijie Network. All rights reserved.
 */
/*
 * ngx_http_getfile_module.c
 * Original Author: zhaoyao@ruijie.com.cn, 2014-02-27
 *
 * Nginx http Getfile module, access to third party or server resources based on Upstream method.
 *
 * History:
 *  v1.0    zhaoyao@ruijie.com.cn   2014-02-27
 *          创建此文件。
 *
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <net/networklayer/inet_lib.h>
#include <ef/ef_buffer/ef_buff.h>
#include <avllib/avlLib.h>
#include "../../http-snooping/http-snooping.h"

#define DEBUG_GETFILE               0   /* Debug switch */
#define NGX_HTTP_GETFILE_TEMP_PATH "mnt/usb0/getfile_temp"
#define NGX_HTTP_WEB_PORT_DEFAULT   80

#define BUFLEN                      64
#define BUFLEN_L                    512
#define IP_ADDR_LEN_MAX             15
#define IP_ADDR_LEN_MIN             7
#define FILE_URL_LEN_MIN            21
#define COMMON_TIMEOUT_VAL          60000   /* 单位ms */
#define BUFFER_NUMBER_VAL           8
#define HASH_MAX_SIZE               100
#define HASH_BUCKET_SIZE            1024
#define PRINT_BUFFER_SIZE           1024

typedef struct {
    ngx_http_upstream_conf_t upstream;
} ngx_http_getfile_conf_t;

typedef struct {
    ngx_http_status_t status;
} ngx_http_getfile_ctx_t;

static char * ngx_http_getfile(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_getfile_handler(ngx_http_request_t *r);
static ngx_int_t getfile_upstream_process_header(ngx_http_request_t *r);
static void *ngx_http_getfile_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_getfile_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t getfile_get_host_from_uri(ngx_http_request_t *r, char host[]);

static ngx_path_init_t  ngx_http_getfile_temp_path = {
    ngx_string(NGX_HTTP_GETFILE_TEMP_PATH), { 1, 2, 0 }
};

static ngx_http_module_t ngx_http_getfile_module_ctx = {
    NULL,
    NULL,

    NULL,
    NULL,

    NULL,
    NULL,

    ngx_http_getfile_create_loc_conf,
    ngx_http_getfile_merge_loc_conf,
};

static ngx_command_t ngx_http_getfile_commands[] = {
    { ngx_string("getfile"),
      NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
      ngx_http_getfile,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("getfile_store_access"),
      NGX_HTTP_LOC_CONF | NGX_CONF_TAKE123,
      ngx_conf_set_access_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_getfile_conf_t, upstream) + offsetof(ngx_http_upstream_conf_t, store_access),
      NULL },

    ngx_null_command,
};

ngx_module_t ngx_http_getfile_module = {
    NGX_MODULE_V1,
    &ngx_http_getfile_module_ctx,
    ngx_http_getfile_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING,
};

static char *ngx_http_getfile(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;

    if (cf == NULL) {
        return NGX_CONF_ERROR;
    }

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_getfile_handler;

    return NGX_CONF_OK;
}

static void *ngx_http_getfile_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_getfile_conf_t *mycf;

    if (cf == NULL) {
        return NULL;
    }

    mycf = (ngx_http_getfile_conf_t *)ngx_pcalloc(cf->pool, sizeof(ngx_http_getfile_conf_t));
    if (mycf == NULL) {
        return NULL;
    }

    ngx_memzero(mycf, sizeof(ngx_http_getfile_conf_t));

    mycf->upstream.connect_timeout = COMMON_TIMEOUT_VAL;
    mycf->upstream.send_timeout = COMMON_TIMEOUT_VAL;
    mycf->upstream.read_timeout = COMMON_TIMEOUT_VAL;
    mycf->upstream.store_access = NGX_CONF_UNSET_UINT;
    
    mycf->upstream.buffering = 1;       /* XXX: flag, using temp file for buffering */
    mycf->upstream.store = 1;           /* XXX: flag, store temp file at local directory */
    mycf->upstream.bufs.num = BUFFER_NUMBER_VAL;
    mycf->upstream.bufs.size = ngx_pagesize;
    mycf->upstream.buffer_size = ngx_pagesize;
    mycf->upstream.busy_buffers_size = 2 * ngx_pagesize;
    mycf->upstream.temp_file_write_size = 2 * ngx_pagesize;
    mycf->upstream.max_temp_file_size = 1024 * 1024 * 1024; /* 1GB */

    mycf->upstream.hide_headers = NGX_CONF_UNSET_PTR;
    mycf->upstream.pass_headers = NGX_CONF_UNSET_PTR;

    return mycf;
}

static ngx_str_t  ngx_http_getfile_hide_headers[] = {
    ngx_string("Date"),
    ngx_string("Server"),
    ngx_string("X-Pad"),
    ngx_string("X-Accel-Expires"),
    ngx_string("X-Accel-Redirect"),
    ngx_string("X-Accel-Limit-Rate"),
    ngx_string("X-Accel-Buffering"),
    ngx_string("X-Accel-Charset"),
    ngx_null_string
};

static char *ngx_http_getfile_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_getfile_conf_t *prev = (ngx_http_getfile_conf_t *)parent;
    ngx_http_getfile_conf_t *conf = (ngx_http_getfile_conf_t *)child;
    ngx_hash_init_t hash;

    if (cf == NULL || parent == NULL || child == NULL) {
        return NGX_CONF_ERROR;
    }

    hash.max_size = HASH_MAX_SIZE;
    hash.bucket_size = HASH_BUCKET_SIZE;
    hash.name = "getfile_headers_hash";
    if (ngx_http_upstream_hide_headers_hash(cf, &conf->upstream,
            &prev->upstream, ngx_http_getfile_hide_headers, &hash) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (ngx_conf_merge_path_value(cf, &conf->upstream.temp_path, NULL, &ngx_http_getfile_temp_path)
        != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    /* 保存文件的权限0600，表示仅拥有者具有读写权限 */
    ngx_conf_merge_uint_value(conf->upstream.store_access, prev->upstream.store_access, 0600);

    return NGX_CONF_OK;
}

static ngx_int_t getfile_get_real_uri_from_uri(ngx_http_request_t *r, char ru[])
{
    u_char *cp;
    size_t i, len;

    if (r == NULL || ru == NULL) {
        return -1;
    }

    for (cp = r->args.data, len = 0; *cp != '/' && len < r->args.len; cp++, len++) {
        (void)0;
    }

    for (i = 0; len < r->args.len && i < BUFLEN_L; cp++, i++, len++) {
        ru[i] = *cp;
    }
    if (i < BUFLEN_L) {
        ru[i] = '\0';
    } else {
        ru[BUFLEN_L - 1] = '\0';
        ngx_log_stderr(NGX_OK, "****** ***ERROR*** %s uri is too large (%d), truncated",
                                __func__, r->args.len);
    }

    return 0;
}

/*
 * http://192.168.46.89/getfile?192.68.196.38/Videos/CrewEarthObservationsVideos/
 *                          bangkokpacific_iss_20140130/bangkokpacific_iss_20140130.mov
 */
static ngx_int_t getfile_upstream_create_request(ngx_http_request_t *r)
{
    static ngx_str_t backendQueryLine =
            ngx_string("GET %s HTTP/1.0\r\nHost: %V\r\nUser-Agent: "
                       "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
                       "Chrome/32.0.1700.107 Safari/537.36\r\nConnection: close\r\n\r\n");
    ngx_int_t queryLineLen;
    ngx_buf_t *b;
    char real_uri[BUFLEN_L];
    ngx_str_t *host;

    if (r == NULL) {
        return NGX_ERROR;
    }

    host = &(r->upstream->resolved->host);
    ngx_memzero(real_uri, BUFLEN_L);
    if (getfile_get_real_uri_from_uri(r, real_uri) != 0) {
        return NGX_ERROR;
    }
    ngx_log_stderr(NGX_OK, "****** %s real_uri %s", __func__, real_uri);

    queryLineLen = backendQueryLine.len + strlen(real_uri) - 2 + host->len - 2; /* 去掉%s无关字符 */
    b = ngx_create_temp_buf(r->pool, queryLineLen);
    if (b == NULL) {
        return NGX_ERROR;
    }
    b->last = b->pos + queryLineLen;

    ngx_snprintf(b->pos, queryLineLen, (char *)backendQueryLine.data, real_uri, host);

    r->upstream->request_bufs = ngx_alloc_chain_link(r->pool);
    if (r->upstream->request_bufs == NULL) {
        return NGX_ERROR;
    }

    r->upstream->request_bufs->buf = b;
    r->upstream->request_bufs->next = NULL;

    r->upstream->request_sent = 0;
    r->upstream->header_sent = 0;

    r->header_hash = 1;

    return NGX_OK;
}

static void getfile_debug_buffer(ngx_buf_t *b)
{
#if DEBUG_GETFILE
    static u_char str[BUFLEN_L];
    int i;

    if (b == NULL) {
        return;
    }

    ngx_memzero(str, BUFLEN_L);

    for (i = 0; i < (b->last - b->pos) && i < (BUFLEN_L - 1); i++) {
        str[i] = b->pos[i];
    }
    str[i] = '\0';
    ngx_log_stderr(NGX_OK, "****** %s: \n"
                           "Buffer size: 0x%p\n"
                           "Buffer data length: 0x%p\n"
                           "%s", __func__, (b->end - b->start), (b->last - b->pos), str);
#endif
}

static ngx_int_t getfile_process_status_line(ngx_http_request_t *r)
{
    size_t len;
    ngx_int_t rc;
    ngx_http_upstream_t *u;
    ngx_http_getfile_ctx_t *ctx;

    if (r == NULL) {
        return NGX_ERROR;
    }

    ctx = ngx_http_get_module_ctx(r, ngx_http_getfile_module);
    if (ctx == NULL) {
        return NGX_ERROR;
    }

    u = r->upstream;
    if (u == NULL) {
        return NGX_ERROR;
    }

    getfile_debug_buffer(&u->buffer);
    rc = ngx_http_parse_status_line(r, &u->buffer, &ctx->status);
    if (rc == NGX_AGAIN) {
        return rc;
    }
    if (rc == NGX_ERROR) {
        ngx_log_stderr(NGX_OK, "%s upstream sent no valid HTTP/1.0 header", __func__);

        r->http_version = NGX_HTTP_VERSION_9;
        u->state->status = NGX_HTTP_OK;

        return NGX_OK;
    }

    if (u->state) {
        u->state->status = ctx->status.code;
    }

    u->headers_in.status_n = ctx->status.code;

    len = ctx->status.end - ctx->status.start;
    u->headers_in.status_line.len = len;

    u->headers_in.status_line.data = ngx_pnalloc(r->pool, len);
    if (u->headers_in.status_line.data == NULL) {
        return NGX_ERROR;
    }

    ngx_memcpy(u->headers_in.status_line.data, ctx->status.start, len);
    ngx_log_stderr(NGX_OK, "****** %s status line: %V", __func__, &u->headers_in.status_line);

    u->process_header = getfile_upstream_process_header;

    return getfile_upstream_process_header(r);
}

static void getfile_debug_upstream_headers(ngx_http_request_t *r)
{
#if DEBUG_GETFILE
    ngx_table_elt_t *h;

    if (r == NULL) {
        return;
    }

    h = r->upstream->headers_in.content_type;
    if (h != NULL) {
        ngx_log_stderr(NGX_OK, "****** %s: %V[%V]", __func__, &h->key, &h->value);
    }
    h = r->upstream->headers_in.content_length;
    if (h != NULL) {
        ngx_log_stderr(NGX_OK, "****** %s: %V[%V]", __func__, &h->key, &h->value);
    }
    h = r->upstream->headers_in.accept_ranges;
    if (h != NULL) {
        ngx_log_stderr(NGX_OK, "****** %s: %V[%V]", __func__, &h->key, &h->value);
    }
    h = r->upstream->headers_in.connection;
    if (h != NULL) {
        ngx_log_stderr(NGX_OK, "****** %s: %V[%V]", __func__, &h->key, &h->value);
    }
    h = r->upstream->headers_in.location;
    if (h != NULL) {
        ngx_log_stderr(NGX_OK, "****** %s: %V[%V]", __func__, &h->key, &h->value);
    }
    h = r->upstream->headers_in.server;
    if (h != NULL) {
        ngx_log_stderr(NGX_OK, "****** %s: %V[%V]", __func__, &h->key, &h->value);
    }
    h = r->upstream->headers_in.date;
    if (h != NULL) {
        ngx_log_stderr(NGX_OK, "****** %s: %V[%V]", __func__, &h->key, &h->value);
    }
#endif
}

static ngx_int_t getfile_upstream_process_header(ngx_http_request_t *r)
{
    ngx_int_t rc;
    ngx_table_elt_t *h;
    ngx_http_upstream_header_t *hh;
    ngx_http_upstream_main_conf_t *umcf;

    if (r == NULL) {
        return NGX_ERROR;
    }

    umcf = ngx_http_get_module_main_conf(r, ngx_http_upstream_module);

    for ( ;; ) {
        rc = ngx_http_parse_header_line(r, &r->upstream->buffer, 1);
        if (rc == NGX_OK) {
            h = ngx_list_push(&r->upstream->headers_in.headers);
            if (h == NULL) {
                return NGX_ERROR;
            }
            h->hash = r->header_hash;

            h->key.len = r->header_name_end - r->header_name_start;
            h->value.len = r->header_end - r->header_start;
            h->key.data = ngx_pnalloc(r->pool, h->key.len + 1 + h->value.len + 1 + h->key.len);
            if (h->key.data == NULL) {
                return NGX_ERROR;
            }

            h->value.data = h->key.data + h->key.len + 1;
            h->lowcase_key = h->key.data + h->key.len + 1 + h->value.len + 1;

            ngx_memcpy(h->key.data, r->header_name_start, h->key.len);
            h->key.data[h->key.len] = '\0';
            ngx_memcpy(h->value.data, r->header_start, h->value.len);
            h->value.data[h->value.len] = '\0';

            if (h->key.len == r->lowcase_index) {
                ngx_memcpy(h->lowcase_key, r->lowcase_header, h->key.len);
            } else {
                ngx_strlow(h->lowcase_key, h->key.data, h->key.len);
            }

            hh = ngx_hash_find(&umcf->headers_in_hash, h->hash, h->lowcase_key, h->key.len);

            if (hh && hh->handler(r, h, hh->offset) != NGX_OK) {
                return NGX_ERROR;
            }

            continue;
        }

        if (rc == NGX_HTTP_PARSE_HEADER_DONE) {
            if (r->upstream->headers_in.server == NULL) {
                h = ngx_list_push(&r->upstream->headers_in.headers);
                if (h == NULL) {
                    return NGX_ERROR;
                }

                h->hash = ngx_hash(ngx_hash(ngx_hash(ngx_hash(ngx_hash('s', 'e'), 'r'), 'v'), 'e'), 'r');

                ngx_str_set(&h->key, "Server");
                ngx_str_null(&h->value);
                h->lowcase_key = (u_char *) "server";
            }

            if (r->upstream->headers_in.date == NULL) {
                h = ngx_list_push(&r->upstream->headers_in.headers);
                if (h == NULL) {
                    return NGX_ERROR;
                }

                h->hash = ngx_hash(ngx_hash(ngx_hash('d', 'a'), 't'), 'e');

                ngx_str_set(&h->key, "Date");
                ngx_str_null(&h->value);
                h->lowcase_key = (u_char *) "date";
            }

            getfile_debug_upstream_headers(r);

            return NGX_OK;
        }

        if (rc == NGX_AGAIN) {
            return NGX_AGAIN;
        }

        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "upstream sent invalid header");

        return NGX_HTTP_UPSTREAM_INVALID_HEADER;
    }
}

static void getfile_upstream_finalize_request(ngx_http_request_t *r, ngx_int_t rc)
{
    ngx_str_t             value;
    ngx_int_t             url_id = -1;
    char                  print_msg[PRINT_BUFFER_SIZE];

    if (r == NULL) {
        return;
    }

#if DEBUG_GETFILE
    ngx_log_stderr(NGX_OK, "****** %s:\n\targs: %V", __func__, &r->args);
#endif

    if ((value.data = (u_char *)ngx_strstr(r->args.data, "urlID")) != NULL) {
        value.data = value.data + 6;    /* 跳过urlID项，共6字节 */
        for (value.len = 0; isdigit(value.data[value.len]); value.len++) {
            (void)0;
        }
        url_id = ngx_atoi(value.data, value.len);
    }
    if (url_id < 0) {
        ngx_log_stderr(NGX_OK, "****** %s ERROR: get url id failed", __func__);
        return;
    }

    memset(print_msg, 0, PRINT_BUFFER_SIZE);
    if (r->args.len >= PRINT_BUFFER_SIZE) {
        memcpy(print_msg, r->args.data, PRINT_BUFFER_SIZE - 1);
    } else {
        memcpy(print_msg, r->args.data, r->args.len);
    }

    if (r->getfile_success) {
		/* XXX: download file success, set state */
        ngx_http_sp_url_state_setby_id(url_id, HTTP_LOCAL_DOWN_STATE_OK);
        ngx_log_stderr(NGX_OK, "****** %s: download %V success...", __func__, &r->args);
    } else {
		/* XXX: download file failed, set state */
        ngx_http_sp_url_state_setby_id(url_id, HTTP_LOCAL_DOWN_STATE_ERROR);
        ngx_log_stderr(NGX_OK, "****** %s: download %V failed...", __func__, &r->args);
    }

    return;
}

static ngx_int_t getfile_get_host_from_uri(ngx_http_request_t *r, char host[])
{
    u_char *cp;
    size_t i;
    ngx_http_upstream_resolved_t *rslv;

    if (r == NULL || host == NULL) {
        return -1;
    }

    rslv = r->upstream->resolved;
    if (rslv == NULL) {
        return -1;
    }

    rslv->host.data = r->args.data;
    rslv->host.len = 0;
    for (cp = r->args.data, i = 0; *cp != '/' && *cp != '\0' && i < BUFLEN - 1; cp++, i++) {
        host[i] = *cp;
        rslv->host.len++;
    }
    host[i] = '\0';
    if (i == BUFLEN - 1) {
        ngx_log_stderr(NGX_OK, "****** ***ERROR*** %s host is too large, truncated", __func__);
    }

    return 0;
}

static ngx_int_t getfile_support_type(ngx_http_request_t *r)
{
    ngx_int_t res;
    u_char *end;

    if (r == NULL) {
        return 0;
    }

//    if (ngx_strstr(r->args.data, "sohu/ts/") != NULL) {
    if ((ngx_strstr(r->args.data, "/sohu/") != NULL) &&
            (ngx_strstr(r->args.data, "/ts/") != NULL)) {
        /*
         * XXX FIXME TODO: 解决sohu视频下载失败问题的临时方法；
         *                 携带"sohu/ts/"即认为是搜狐视频。
         */
        return 1;
    }

    end = (u_char *)ngx_strstr(r->args.data, "urlID");
    if (end == NULL) {
        end = r->args.data + r->args.len;
    } else {
        end = end - 1;
    }
    
    res = strncmp((char *)(end - 3), "flv", 3) && 
          strncmp((char *)(end - 3), "mp4", 3) &&
          strncmp((char *)(end - 3), "gif", 3) &&
          strncmp((char *)(end - 3), "png", 3) &&
          strncmp((char *)(end - 3), "jpg", 3) &&
          strncmp((char *)(end - 3), "zip", 3) &&
          strncmp((char *)(end - 3), "ico", 3) &&
          strncmp((char *)(end - 3), "txt", 3) &&
          strncmp((char *)(end - 3), "rar", 3) &&
          strncmp((char *)(end - 3), "bin", 3) &&
          strncmp((char *)(end - 3), "doc", 3) &&
          strncmp((char *)(end - 4), "docx", 4) &&
          strncmp((char *)(end - 3), "xls", 3) &&
          strncmp((char *)(end - 4), "xlsx", 4) &&
          strncmp((char *)(end - 3), "ppt", 3) &&
          strncmp((char *)(end - 4), "pptx", 4) &&
          strncmp((char *)(end - 4), "html", 4) &&
          strncmp((char *)(end - 3), "mov", 3);
    res = !res;

    if (!res) {
#if DEBUG_GETFILE
        ngx_str_t str;

        str.data = end - 4; /* 打印最后4个字节 */
        str.len = 4;
        ngx_log_stderr(NGX_OK, "****** %s type - %V not support", __func__, &str);
#endif
    }

    return res;
}

#define IP_ADDR_HEX_BUFFER_LEN  4   /* 存放点分十进制IP地址每个hex字符串，需要4字节的缓冲区 */
#define IP_ADDR_HEX_DIGITS_MIN  1   /* hex字符串的位数最多3位，最少1位 */
#define IP_ADDR_HEX_DIGITS_MAX  3
#define IP_ADDR_DOT_COUNT       3   /* 点分十进制IP地址只有3个点 */
#define IP_ADDR_HEX_VAL_MIN     0   /* hex的有效值为[0, 255]区间 */
#define IP_ADDR_HEX_VAL_MAX     255
static ngx_int_t getfile_host_is_ip_addr(const char *host)
{
    ngx_int_t len = 0, dot_cnt = 0, digits = 0, i, num;
    char asc[IP_ADDR_HEX_BUFFER_LEN] = {0};

    if (host == NULL) {
        return 0;
    }

    len = ngx_strlen(host);
    if (len > IP_ADDR_LEN_MAX || len < IP_ADDR_LEN_MIN) {
        return 0;
    }

    ngx_memset(asc, 0, sizeof(asc));
    for (i = 0; i < len; i++) {
        if (host[i] == '.') {
            if (digits > IP_ADDR_HEX_DIGITS_MAX || digits < IP_ADDR_HEX_DIGITS_MIN) {
                return 0;
            }
            asc[digits] = '\0';
            num = atoi(asc);
            if (num < IP_ADDR_HEX_VAL_MIN || num > IP_ADDR_HEX_VAL_MAX) {
                return 0;
            }
            digits = 0;
            ngx_memset(asc, 0, sizeof(asc));
            dot_cnt++;
            if (dot_cnt > IP_ADDR_DOT_COUNT) {
                return 0;
            }
            continue;
        }
        if (!isdigit(host[i])) {
            return 0;
        }
        if (digits >= IP_ADDR_HEX_DIGITS_MAX) {
            return 0;
        }
        asc[digits++] = host[i];
    }
    
    if (dot_cnt != IP_ADDR_DOT_COUNT) {
        return 0;
    }
    if (digits > IP_ADDR_HEX_DIGITS_MAX || digits < IP_ADDR_HEX_DIGITS_MIN) {
        return 0;
    }
    asc[digits] = '\0';
    num = atoi(asc);
    if (num < IP_ADDR_HEX_VAL_MIN || num > IP_ADDR_HEX_VAL_MAX) {
        return 0;
    }

    return 1;
}

static ngx_int_t ngx_http_getfile_handler(ngx_http_request_t *r)
{
    ngx_http_getfile_ctx_t *myctx;
    ngx_http_getfile_conf_t *mycf;
    ngx_http_upstream_t *u;
    static struct sockaddr_in backendSockAddr;
    char host[BUFLEN];

    if (r == NULL) {
        return NGX_ERROR;
    }

    myctx = ngx_http_get_module_ctx(r, ngx_http_getfile_module);
    if (myctx == NULL) {
        myctx = ngx_palloc(r->pool, sizeof(ngx_http_getfile_ctx_t));
        if (myctx == NULL) {
            return NGX_ERROR;
        }
        ngx_memzero(myctx, sizeof(ngx_http_getfile_ctx_t));
        
        ngx_http_set_ctx(r, myctx, ngx_http_getfile_module);
    }
#if DEBUG_GETFILE
    ngx_log_stderr(NGX_OK, "*** %s ***", __func__);
    ngx_log_stderr(NGX_OK, "http uri: \"%V\"", &r->uri);
    ngx_log_stderr(NGX_OK, "http args: \"%V\"", &r->args);
    ngx_log_stderr(NGX_OK, "http exten: \"%V\"", &r->exten);
#endif
    if (r->args.len < FILE_URL_LEN_MIN) {
        ngx_log_stderr(NGX_OK, "****** %s arguments invalid, less than %i",
                                __func__, FILE_URL_LEN_MIN);
        return NGX_ERROR;
    }

    if (!getfile_support_type(r)) {
        ngx_log_stderr(NGX_OK, "****** %s not support file type", __func__);
        return NGX_ERROR;
    }

    if (ngx_http_upstream_create(r) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ngx_http_upstream_create() failed");
        return NGX_ERROR;
    }

    mycf = (ngx_http_getfile_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_getfile_module);
    u = r->upstream;
    u->conf = &mycf->upstream;
    u->buffering = mycf->upstream.buffering;
    u->store = mycf->upstream.store;

    r->header_only = 1; /* XXX: flag, do not send response body to downstream */
    r->getfile = 1;     /* XXX TODO: flag, stored file's name is from uri->args */
    r->getfile_success = 0; /* XXX: when finalizing request, check it to re-download */
    
    u->pipe = ngx_pcalloc(r->pool, sizeof(ngx_event_pipe_t));
    if (u->pipe == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    u->pipe->input_filter = ngx_event_pipe_copy_input_filter;
    u->pipe->input_ctx = r;

    u->resolved = ngx_pcalloc(r->pool, sizeof(ngx_http_upstream_resolved_t));
    if (u->resolved == NULL) {
        ngx_log_stderr(NGX_OK, "%s ngx_pcalloc resolved error.", __func__);
        return NGX_ERROR;
    }

    ngx_memzero(host, BUFLEN);
    /* host can be IP address or domain name */
    if (getfile_get_host_from_uri(r, host) != 0) {
        return NGX_ERROR;
    }

    if (getfile_host_is_ip_addr(host)) {
        /* IP address, using it directly */
        ngx_memzero(&backendSockAddr, sizeof(backendSockAddr));
        /* XXX: it always return >=0 ? */
        if (inet_aton(host, (struct in_addr *)&backendSockAddr.sin_addr) < 0) {
            return NGX_ERROR;
        }
        ngx_log_stderr(NGX_OK, "****** %s: host IP address is %s", __func__, host);
        backendSockAddr.sin_family = AF_INET;
        backendSockAddr.sin_port = htons((in_port_t) NGX_HTTP_WEB_PORT_DEFAULT);
        u->resolved->sockaddr = (struct sockaddr *)&backendSockAddr;
        u->resolved->socklen = sizeof(struct sockaddr_in);
        u->resolved->naddrs = 1;
    } else {
        /* Domain name, using upstream resolve. */
        ngx_log_stderr(NGX_OK, "****** %s: host domain name is %V", __func__, &(u->resolved->host));
        /* XXX TODO: only support NGX_HTTP_WEB_PORT_DEFAULT port now */
        u->resolved->port = (in_port_t)NGX_HTTP_WEB_PORT_DEFAULT;
    }

    u->create_request = getfile_upstream_create_request;
    u->process_header = getfile_process_status_line;
    u->finalize_request = getfile_upstream_finalize_request;

    r->main->count++;
    ngx_http_upstream_init(r);

    return NGX_DONE;
}

