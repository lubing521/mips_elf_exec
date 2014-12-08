
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


static char *ngx_http_flv(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_command_t  ngx_http_flv_commands[] = {

    { ngx_string("flv"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      ngx_http_flv,
      0,
      0,
      NULL },

      ngx_null_command
};


static u_char  ngx_flv_header[] = "FLV\x1\x5\0\0\0\x9\0\0\0\0";


static ngx_http_module_t  ngx_http_flv_module_ctx = {
    NULL,                          /* preconfiguration */
    NULL,                          /* postconfiguration */

    NULL,                          /* create main configuration */
    NULL,                          /* init main configuration */

    NULL,                          /* create server configuration */
    NULL,                          /* merge server configuration */

    NULL,                          /* create location configuration */
    NULL                           /* merge location configuration */
};


ngx_module_t  ngx_http_flv_module = {
    NGX_MODULE_V1,
    &ngx_http_flv_module_ctx,      /* module context */
    ngx_http_flv_commands,         /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};


#define FLV_METADATA_READ_LEN           (1 << 14) /* 16KB*/
#define ngx_http_flv_is_script_tag(val) ((val) == 0x12)
#define ngx_http_flv_is_video_tag(val)  ((val) == 0x09)
#define ngx_http_flv_is_audio_tag(val)  ((val) == 0x08)


/* 查找flv视频中，tag的位置 
 * tag_index, tag在buf中的位置
 * tag_size, tag的大小, 含11字节的头部长度
 * 返回NGX_ERROR，查找失败 */
static ngx_int_t
ngx_http_flv_kf_find_tag_pos(u_char *buf, ssize_t buf_size, ssize_t *tag_size, ngx_uint_t *tag_index)
{
	ngx_uint_t i;
    ngx_uint_t tag_start_pos, tag_end_pos;
    ngx_int_t ret = NGX_ERROR;

    if (buf == NULL) {
        return NGX_ERROR;
    }

	i = 0;
	while (i < (ngx_uint_t)buf_size) {
		if (ngx_http_flv_is_script_tag(buf[i]) ||
            ngx_http_flv_is_audio_tag(buf[i])  ||
            ngx_http_flv_is_video_tag(buf[i])) {
			if (buf[i + 8] == 0 && 
                buf[i + 9] == 0 && 
                buf[i + 10] == 0) {//判断流ID是否为0
                tag_start_pos = (buf[i + 1] << 16) + (buf[i + 2] << 8) + (buf[i + 3]);

				if ((i + tag_start_pos) <= (ngx_uint_t)buf_size && tag_start_pos > 0 ) {
                    tag_end_pos = ((buf[i+tag_start_pos+1+11]) << 16) +
                                  ((buf[i+tag_start_pos+2+11]) << 8) +
                                  ((buf[i+tag_start_pos+3+11])) - 11;

                    if (tag_start_pos == tag_end_pos) {
                        *tag_index = i; //找到了视频tag的位置
                        *tag_size = tag_start_pos + 11; /* 该tag的大小 */
                        ret = NGX_OK;
                        break;
                    }
                }
			}
		}

		i++;
	}
	
	return ret;
}

static ngx_int_t
ngx_http_flv_kf_compute_offset(u_char *buf, ssize_t buf_size, off_t start, off_t *offset, off_t *pre_kf2_size2)
{
	ngx_uint_t tag_index, keyframe_total, i, start_kf_num;
    ssize_t tag_size;
    u_char *kf_ptr = NULL, *p, *fpos_start;
    //u_char *tinfo_start;
    /* zhaoyao: 计算用的临时值 */
    u_char switch_buf32[4], switch_buf64[8];
    uint64_t temp64;
    double dd;

    if (ngx_http_flv_kf_find_tag_pos(buf, buf_size, &tag_size, &tag_index) != NGX_OK) {
        ngx_log_stderr(NGX_OK, "%s: ngx_http_flv_kf_find_tag_pos failed, SHOULD expand read buffer size\n", __func__);
        return NGX_ERROR;
    }

    if (!ngx_http_flv_is_script_tag(buf[tag_index])) {
        ngx_log_stderr(NGX_OK, "%s: %xi is not script tag\n", __func__, buf[tag_index]);
        return NGX_ERROR;
    }

    for (i = tag_index + 1; (ssize_t)i < buf_size; i++) {
        if (buf[i] == 'k') {
            kf_ptr = (u_char *)ngx_strstr(buf + i, "keyframes");
            if (kf_ptr != NULL) {
                break;
            }
        }
    }
    if (kf_ptr == NULL) {
        ngx_log_stderr(NGX_OK, "%s: not find \"keyframes\"\n", __func__);
        return NGX_ERROR;
    }

    p = kf_ptr + 26;
    /* 03 00 0D + filepositions + 0A + 4bytes的长度 */
    ngx_memzero(switch_buf32, sizeof(switch_buf32));
    /* 读取出来的数据是大端序 */
    ngx_memcpy(switch_buf32, p, 4);
    /* 读取keyframe 的个数 */
    keyframe_total = *((ngx_uint_t *)(&switch_buf32));
    /* zhaoyao XXX TODO FIXME: 默认MIPS是大端 */
//    keyframe_total = be32toh(keyframe_total);

    p = p + 4;
    fpos_start = p;
    /* filepositions信息 */
    p = fpos_start + (1 + 8) * keyframe_total;
    /* times信息 */
    p = p + 12;
//    tinfo_start = p;

    for (i = 0; i < keyframe_total; i++) {
        p++;   /* 类型1bytes */
        ngx_memzero(switch_buf64, sizeof(switch_buf64));
        /* 8bytes */
        ngx_memcpy(switch_buf64, p, 8);
        p = p + 8;

        /* zhaoyao XXX TODO FIXME: 默认MIPS是大端 */
        //temp64 = be64toh(*(uint64_t *)&switch_buf64);
        temp64 = *(uint64_t *)&switch_buf64;
        dd = *(double *)&temp64;
        if (start < (off_t)dd) {
            break;
        }
    }
    
    if (i == 0) {
        ngx_log_stderr(NGX_OK, "%s: start is ahead of first keyframe\n", __func__);
        start_kf_num = 0;
    } else {
        start_kf_num = i - 1;
    }

    p = fpos_start + (1 + 8) * start_kf_num;
    p++;
    ngx_memzero(switch_buf64, sizeof(switch_buf64));
    ngx_memcpy(switch_buf64, p, 8);
    /* zhaoyao XXX TODO FIXME: 默认MIPS是大端 */
    //temp64 = be64toh(*(uint64_t *)&switch_buf64);
    temp64 = *(uint64_t *)&switch_buf64;
    dd = *(double *)&temp64;
    *offset = (off_t)dd;

    p = fpos_start + (1 + 8) * 1;
    p++;
    ngx_memzero(switch_buf64, sizeof(switch_buf64));
    ngx_memcpy(switch_buf64, p, 8);
    /* zhaoyao XXX TODO FIXME: 默认MIPS是大端 */
    //temp64 = be64toh(*(uint64_t *)&switch_buf64);
    temp64 = *(uint64_t *)&switch_buf64;
    dd = *(double *)&temp64;
    *pre_kf2_size2 = (off_t)dd;

	return NGX_OK;
}

static ngx_int_t
ngx_http_flv_time_to_offset(ngx_file_t *file, off_t start, off_t *offset, off_t *pre_kf2_size2)
{
    ssize_t                    n;
    u_char                     meta_data[FLV_METADATA_READ_LEN];

    ngx_memzero(meta_data, FLV_METADATA_READ_LEN);
    n = ngx_read_file(file, meta_data, sizeof(meta_data), 0);
    if (n == NGX_ERROR) {
        return NGX_ERROR;
    }

    if (ngx_http_flv_kf_compute_offset(meta_data, n, start, offset, pre_kf2_size2) != NGX_OK) {
        ngx_log_stderr(NGX_OK, "%s ERROR: start = %O\n", __func__, start);
        return NGX_ERROR;
    } else {
        return NGX_OK;
    }
}


static ngx_int_t
ngx_http_flv_handler(ngx_http_request_t *r)
{
    u_char                    *last;
    off_t                      start, len;
    size_t                     root;
    ngx_int_t                  rc;
    ngx_uint_t                 level, i;
    ngx_str_t                  path, value;
    ngx_log_t                 *log;
    ngx_buf_t                 *b;
    ngx_chain_t                out[2];
    ngx_open_file_info_t       of;
    ngx_http_core_loc_conf_t  *clcf;

    ngx_file_t                *file;
    off_t                      offset, pre_kf2_size;
    ngx_int_t                  ret;

    if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    if (r->uri.data[r->uri.len - 1] == '/') {
        return NGX_DECLINED;
    }

    rc = ngx_http_discard_request_body(r);

    if (rc != NGX_OK) {
        return rc;
    }

    last = ngx_http_map_uri_to_path(r, &path, &root, 0);
    if (last == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    log = r->connection->log;

    path.len = last - path.data;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0,
                   "http flv filename: \"%V\"", &path);

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    ngx_memzero(&of, sizeof(ngx_open_file_info_t));

    of.read_ahead = clcf->read_ahead;
    of.directio = clcf->directio;
    of.valid = clcf->open_file_cache_valid;
    of.min_uses = clcf->open_file_cache_min_uses;
    of.errors = clcf->open_file_cache_errors;
    of.events = clcf->open_file_cache_events;

    if (ngx_http_set_disable_symlinks(r, clcf, &path, &of) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (ngx_open_cached_file(clcf->open_file_cache, &path, &of, r->pool)
        != NGX_OK)
    {
        switch (of.err) {

        case 0:
            return NGX_HTTP_INTERNAL_SERVER_ERROR;

        case NGX_ENOENT:
        case NGX_ENOTDIR:
        case NGX_ENAMETOOLONG:

            level = NGX_LOG_ERR;
            rc = NGX_HTTP_NOT_FOUND;
            break;

        case NGX_EACCES:
#if (NGX_HAVE_OPENAT)
        case NGX_EMLINK:
        case NGX_ELOOP:
#endif

            level = NGX_LOG_ERR;
            rc = NGX_HTTP_FORBIDDEN;
            break;

        default:

            level = NGX_LOG_CRIT;
            rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
            break;
        }

        if (rc != NGX_HTTP_NOT_FOUND || clcf->log_not_found) {
            ngx_log_error(level, log, of.err,
                          "%s \"%s\" failed", of.failed, path.data);
        }

        return rc;
    }

    if (!of.is_file) {

        if (ngx_close_file(of.fd) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                          ngx_close_file_n " \"%s\" failed", path.data);
        }

        return NGX_DECLINED;
    }

    r->root_tested = !r->error_page;

    start = 0;
    len = of.size;
    i = 1;

    if (r->args.len) {

        if (ngx_http_arg(r, (u_char *) "start", 5, &value) == NGX_OK) {

            start = ngx_atoof(value.data, value.len);

            if (start == NGX_ERROR || start >= len) {
                start = 0;
            }

            if (start) {
                file = ngx_pcalloc(r->pool, sizeof(ngx_file_t));
                if (file == NULL) {
                    return NGX_HTTP_INTERNAL_SERVER_ERROR;
                }
                file->fd = of.fd;
                file->name = path;
                file->log = r->connection->log;
                file->directio = of.is_directio;
                ret = ngx_http_flv_time_to_offset(file, start, &offset, &pre_kf2_size);
                ngx_pfree(r->pool, file);

                if (ret == NGX_ERROR) {
                    ngx_log_stderr(NGX_OK, "*** %s ***: ngx_http_flv_time_to_offset error, using default operation\n", __func__);
                    /* zhaoyao XXX: 采用默认的以字节为单位 */
                    len = sizeof(ngx_flv_header) - 1 + len - start;
                    pre_kf2_size = 0;
                } else {
                    /*
                     * zhaoyao XXX: all we treat is YOUKU .flv video, 它以时间做单位.
                     */
                    len = of.size - offset + pre_kf2_size;
                    ngx_log_stderr(NGX_OK, "*** %s: start(%O), len(%O), offset(%O), file(%O), pre_kf2_size(%O)\n",
                                                __func__, start, len, offset, of.size, pre_kf2_size);
                    start = offset;
                }

                i = 0;
            }
        }
    }

    log->action = "sending flv to client";

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = len;
    r->headers_out.last_modified_time = of.mtime;

    if (ngx_http_set_etag(r) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (ngx_http_set_content_type(r) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (i == 0) {
        b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
        if (b == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        if (pre_kf2_size != 0) {
            b->file = ngx_pcalloc(r->pool, sizeof(ngx_file_t));
            if (b->file == NULL) {
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }
            b->file_pos = 0;
            b->file_last = pre_kf2_size;
            b->in_file = b->file_last ? 1 : 0;
            b->last_buf = (r == r->main) ? 1 : 0;
            b->file->fd = of.fd;
            b->file->name = path;
            b->file->log = log;
            b->file->directio = of.is_directio;
        } else {
            b->pos = ngx_flv_header;
            b->last = ngx_flv_header + sizeof(ngx_flv_header) - 1;
            b->memory = 1;
        }


        out[0].buf = b;
        out[0].next = &out[1];
    }


    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    b->file = ngx_pcalloc(r->pool, sizeof(ngx_file_t));
    if (b->file == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

#if 0
    if (!ns_i) {
        r->allow_ranges = 1;    /* zhaoyao XXX: Accept-Ranges: bytes */
    } else {
        r->allow_ranges = 0;
    }
#endif
    r->allow_ranges = 1;
	r->keepalive = 0;
    rc = ngx_http_send_header(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    b->file_pos = start;
    b->file_last = of.size;

    b->in_file = b->file_last ? 1: 0;
    b->last_buf = (r == r->main) ? 1 : 0;
    b->last_in_chain = 1;

    b->file->fd = of.fd;
    b->file->name = path;
    b->file->log = log;
    b->file->directio = of.is_directio;

    out[1].buf = b;
    out[1].next = NULL;

    return ngx_http_output_filter(r, &out[i]);
}


static char *
ngx_http_flv(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_flv_handler;

    return NGX_CONF_OK;
}
