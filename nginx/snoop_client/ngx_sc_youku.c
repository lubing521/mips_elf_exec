/*
 * Copyright(C) 2014 Ruijie Network. All rights reserved.
 */
/*
 * ngx_sc_youku.c
 * Original Author: zhaoyao@ruijie.com.cn, 2014-04-28
 *
 * Snooping Client's Youku-related routines.
 *
 * History:
 *  v1.0    zhaoyao@ruijie.com.cn   2014-04-28
 *          创建此文件。
 *
 */

#include <sys/support.h>
#include "ngx_snoop_client.h"
#include "yk_lib.h"

static char sc_yk_origin_url_pattern[] = "v.youku.com/v_show/id_%s.html";

/**
 * NAME: sc_url_is_yk
 *
 * DESCRIPTION:
 *      判断是否为优酷url。
 *
 * @url:     -IN 资源url。
 *
 * RETURN: 1表示是，0表示不是。
 */
int sc_url_is_yk(char *url)
{
    if (strstr(url, "youku") != NULL) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * NAME: sc_yk_gen_origin_url
 *
 * DESCRIPTION:
 *      将请求的优酷url转换为固定模式的url。
 *
 * @req_url:    -IN  请求的url。
 * @origin_url: -OUT 转换得到的固定模式url。
 *
 * RETURN: -1表示失败，0表示成功。
 */
int sc_yk_gen_origin_url(char *req_url, char *origin_url)
{
    char buf[YK_VID_MAX_LEN];
    char *tag[] = { "v_show/id_",
                    "&video_id=",
                    "&id=",
                    "&vid=",
                    "&local_vid=",
                    NULL };
    char *start = NULL;
    int i, len = 0;

    if (req_url == NULL || origin_url == NULL) {
        return -1;
    }

    for (i = 0; tag[i] != NULL; i++) {
        start = strstr(req_url, tag[i]);
        if (start == NULL) {
again:
            continue;
        }
        start = start + strlen(tag[i]);
        for (len = 0; isalnum(start[len]); len++) {
            (void)0;
        }
        goto generate;
    }

    return -1;

generate:
    if (len != YK_VID_VALID_LEN) {
        goto again;
    }
    bzero(buf, sizeof(buf));
    strncpy(buf, start, YK_VID_VALID_LEN);
    if (strlen(sc_yk_origin_url_pattern) + YK_VID_VALID_LEN - 2 >= HTTP_SP_URL_LEN_MAX) { /* 去掉"%s" */
        return -1;
    }
    sprintf(origin_url, sc_yk_origin_url_pattern, buf);

    return 0;
}

/* 当有多种清晰度的视频时，选定下载的一种视频 */
static int sc_get_yk_download_video_type(yk_stream_info_t *streams[])
{
    int i, ret = -1;

    if (streams == NULL) {
        return -1;
    }

    for (i = 0; i < YK_STREAM_TYPE_TOTAL && streams[i] != NULL; i++) {
        if (strncmp(streams[i]->type, VIDEO_FLV_SUFFIX, VIDEO_FLV_SUFFIX_LEN) == 0) {
            /* 缺省下载flv格式的视频文件 */
            ret = i;
        }
        if (strncmp(streams[i]->type, VIDEO_MP4_SUFFIX, VIDEO_MP4_SUFFIX_LEN) == 0) {
            /* 优先下载mp4格式的视频文件 */
            ret = i;
            goto out;
        }
    }

out:
    return ret;
}

/**
 * NAME: sc_youku_download
 *
 * DESCRIPTION:
 *      下载优酷视频。
 *
 * @real_url:    -IN 优酷视频最终(真实)的url。
 * @url_id:      -IN 视频的url ID。
 *
 * RETURN: -1表示失败，0表示成功。
 */
int sc_youku_download(char *real_url, ngx_int_t url_id)
{
    int ret;

    if (real_url == NULL || url_id < 0) {
        return -1;
    }

    /* 不需解析，直接通知Nginx下载即可 */
    ret = sc_ngx_download(real_url, url_id);
    
    if (ret != 0) {
        hc_log_error("failed, url id: %d, url: %s", url_id, real_url);
    }

    return ret;
}

/*
 * 全过程: .html origin_url -> getPlaylist -> getFlvpath -> real_url
 */
static int sc_get_yk_video_tradition(char *origin_url)
{
    char yk_url[HTTP_SP_URL_LEN_MAX];                /* Youku video public URL */
    char pl_url[HTTP_SP_URL_LEN_MAX];                /* getplaylist URL */
    char fp_url[HTTP_SP_URL_LEN_MAX];                /* getflvpath URL */
    char real_url[HTTP_SP_URL_LEN_MAX];              /* Youku video file's real URL */
    char *p_real_url;
    char *response = NULL;
    int i, j;
    int err = 0, status, ret;
    yk_stream_info_t *streams[YK_STREAM_TYPE_TOTAL] = {NULL}, *strm;
    int download_index;
    ngx_str_t url_str;
    ngx_int_t url_id;

    if (origin_url == NULL) {
        hc_log_error("Invalid input");
        return -1;
    }

    if (strlen(origin_url) >= HTTP_SP_URL_LEN_MAX) {
        hc_log_error("url is longer than %d: %s", HTTP_SP_URL_LEN_MAX, origin_url);
        return -1;
    }

    memset(yk_url, 0, sizeof(yk_url));
    sc_copy_url(yk_url, origin_url, HTTP_SP_URL_LEN_MAX, 0);

    response = rgos_malloc(RESP_BUF_LEN);
    if (response == NULL) {
        hc_log_error("Malloc failed");
        err = -1;
        goto out;
    }

    /*
     * Step 1 - getPlaylist and get flvpath URL.
     */
    memset(pl_url, 0, HTTP_SP_URL_LEN_MAX);
    if (yk_url_to_playlist(yk_url, pl_url) != 0) {
        hc_log_error("yk_url_to_playlist failed, url is: %s", yk_url);
        err = -1;
        goto out;
    }

    bzero(response, RESP_BUF_LEN);
    if (yk_http_session(pl_url, yk_url, response, RESP_BUF_LEN) < 0) {
        hc_log_error("yk_http_session faild, url: %s", pl_url);
        err = -1;
        goto out;
    }

    if (http_parse_status_line(response, strlen(response), &status) < 0) {
        hc_log_error("Parse status line failed:\n%s", response);
        err = -1;
        goto out;
    }

    if (status == NGX_HTTP_OK) {
        (void)0;
    } else {
        hc_log_error("getPlaylist's response status code %d: %s", status, response);
        err = -1;
        goto out;
    }

    memset(streams, 0, sizeof(streams));
    if (yk_parse_playlist(response, streams) != 0) {
        hc_log_error("Parse getPlaylist response failed:\n%s\n", response);
        err = -1;
        goto out;
    }

    download_index = sc_get_yk_download_video_type(streams);
    if (download_index < 0 && download_index >= YK_STREAM_TYPE_TOTAL) {
        hc_log_error("sc_get_yk_download_video_type %d is invalid index\n", download_index);
        err = -1;
        goto out;
    }

    /* 注意，由于我们只关注download_index对应类型，只下载它 */
    i = download_index;
    strm = streams[i];

    if (strm->segs == NULL) {
        hc_log_error("WARNING: stream %s has no segs\n", strm->type);
        err = -1;
        goto out;
    }

    for (j = 0; j < YK_STREAM_SEGS_MAX && strm->segs[j] != NULL; j++) {
        /*
         * Step 2 - getFlvpath and get real URL.
         */
        memset(fp_url, 0, sizeof(fp_url));
        if (yk_seg_to_flvpath(strm->segs[j], fp_url) < 0) {
            hc_log_error("WARNING: yk_seg_to_flvpath failed, continue...\n");
            continue;
        }

        bzero(response, RESP_BUF_LEN);
        if (yk_http_session(fp_url, yk_url, response, RESP_BUF_LEN) < 0) {
            hc_log_error("yk_http_session faild, url: %s\n", pl_url);
            err = -1;
            goto out;
        }

        if (http_parse_status_line(response, strlen(response), &status) < 0) {
            hc_log_error("Parse status line failed:\n%s", response);
            err = -1;
            goto out;
        }

        memset(real_url, 0, sizeof(real_url));
        p_real_url = real_url;
        if (status == NGX_HTTP_OK || status == NGX_HTTP_MOVED_TEMPORARILY) {
            if (yk_parse_flvpath(response, p_real_url) < 0) {
                hc_log_error("Parse getFlvpath response and get real URL failed\n");
                err = -1;
                goto out;
            }
            if (memcmp(p_real_url, HTTP_URL_PREFIX, HTTP_URL_PRE_LEN) == 0) {
                p_real_url = p_real_url + HTTP_URL_PRE_LEN;
            }
            hc_log_debug("segment %-2d URL: %s", strm->segs[j]->no, p_real_url);

            /*
             * Step 3 - using real URL to download.
             */
            /* 直接调用Snooping Module提供添加url的接口，返回url_id */
            url_str.data = (u_char *)p_real_url;
            url_str.len = strlen(p_real_url);
            url_id = ngx_http_sp_url_handle(&url_str, HTTP_C2SP_ACTION_ADD);
            if (url_id < 0) {
                hc_log_error("add %s to Snooping Module failed, return %d", p_real_url, url_id);
                continue;
            }
            /* 最终下载视频 */
            ret = sc_youku_download(p_real_url, url_id);
            if (ret < 0) {
                hc_log_error("segment %-2d inform Nginx failed, %s\n", strm->segs[j]->no, p_real_url);
                ngx_http_sp_url_state_setby_id(url_id, HTTP_LOCAL_DOWN_STATE_ERROR);
            }
        } else {
            hc_log_error("getFlvpath failed, status code %d:\n%s\n", status, response);
            err = -1;
            goto out;
        }
    }

out:
    if (response != NULL) {
        rgos_free(response);
        response = NULL;
    }

    yk_destroy_streams_all(streams);

    return err;
}

/**
 * NAME: sc_get_yk_video
 *
 * DESCRIPTION:
 *      根据原始url下载优酷视频，中间需要解析过程。
 *
 * @origin_url: -IN 优酷视频原始的url。
 *
 * RETURN: -1表示失败，0表示成功。
 */
int sc_get_yk_video(char *origin_url)
{
    int ret;

    if (origin_url == NULL) {
        hc_log_error("need origin URL to parse real URL");
        return -1;
    }

    if (strlen(origin_url) >= HTTP_SP_URL_LEN_MAX) {
        hc_log_error("url longer than %d: %s", HTTP_SP_URL_LEN_MAX, origin_url);
        return -1;
    }

    if (yk_is_tradition_url(origin_url)) {
        ret = sc_get_yk_video_tradition(origin_url);
    } else {
        hc_log_error("no support: %s", origin_url);
        ret = -1;
    }

    return ret;
}

