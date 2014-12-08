/*
 * Copyright(C) 2014 Ruijie Network. All rights reserved.
 */
/*
 * ngx_sc_sohu.c
 * Original Author: zhaoyao@ruijie.com.cn, 2014-04-28
 *
 * Snooping Client's Sohu-related routines.
 *
 * History:
 *  v1.0    zhaoyao@ruijie.com.cn   2014-04-28
 *          创建此文件。
 *
 */

#include "ngx_snoop_client.h"
#include "sohu_lib.h"

static char sc_sohu_origin_url_pattern1[] = "hot.vrs.sohu.com/ipad%s";
static char sc_sohu_origin_url_pattern2[] = "my.tv.sohu.com/ipad/%s";

/**
 * NAME: sc_url_is_sohu
 *
 * DESCRIPTION:
 *      判断是否为搜狐url。
 *
 * @url:     -IN 资源url。
 *
 * RETURN: 1表示是，0表示不是。
 */
int sc_url_is_sohu(char *url)
{
    if (strstr(url, "sohu") != NULL) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * NAME: sc_url_is_sohu_file_url
 *
 * DESCRIPTION:
 *      判断是否为搜狐视频file型url。
 *
 * @url:     -IN 资源url。
 *
 * RETURN: 1表示是，0表示不是。
 */
int sc_url_is_sohu_file_url(char *url)
{
    if (strstr(url, "?file=/") != NULL) {
        return 1;
    } else {
        return 0;
    }
}

#if 0
/**
 * NAME: sc_sohu_is_local_path
 *
 * DESCRIPTION:
 *      判断是否为搜狐视频本地路径。
 *
 * @local_path: -IN 视频资源本地路径。
 *
 * RETURN: 1表示是，0表示不是。
 */
int sc_sohu_is_local_path(char *local_path)
{
    if (strstr(local_path, "_file=_") != NULL) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * NAME: sc_sohu_file_url_to_local_path
 *
 * DESCRIPTION:
 *      判断是否为搜狐视频本地路径。
 *
 * @local_path: -IN 视频资源本地路径。
 *
 * RETURN: 1表示是，0表示不是。
 */
int sc_sohu_file_url_to_local_path(char *file_url, char *local_path, int len)
{
    char *p, *q;
    int first_slash = 1;

    if (file_url == NULL || local_path == NULL) {
        return -1;
    }

    if (!sc_url_is_sohu_file_url(file_url)) {
        return -1;
    }

    if (len <= strlen(file_url)) {
        return -1;
    }

    for (p = file_url, q = local_path; *p != '\0'; p++, q++) {
        if (first_slash && *p == '.') {
            *q = '_';
            continue;
        }
        if (*p == '/') {
            if (first_slash) {
                *q = *p;
                first_slash = 0;
            } else {
                *q = '_';
            }
            continue;
        }
        if (*p == '?') {
            *q = '_';
            continue;
        }
        *q = *p;
    }
    *q = '\0';

    return 0;
}
#endif

/**
 * NAME: sc_sohu_gen_origin_url
 *
 * DESCRIPTION:
 *      生成固定模式的搜狐视频原始url。
 *
 * @req_url:    -IN  请求的url。
 * @origin_url: -OUT 生成的固定模式url。
 *
 * RETURN: -1表示失败，0表示成功。
 */
int sc_sohu_gen_origin_url(char *req_url, char *origin_url)
{
    char buf[BUFFER_LEN];
    char tag1[] = "hot.vrs.sohu.com/ipad";
    char tag2[] = "my.tv.sohu.com/ipad/";
    char suffix[] = ".m3u8";
    char *start = NULL;
    int len = 0;
    int pattern_len;
    int tag;

    if (req_url == NULL || origin_url == NULL) {
        return -1;
    }

    if ((start = strstr(req_url, tag1)) != NULL) {
        tag = 1;
        pattern_len = strlen(sc_sohu_origin_url_pattern1) - 2;  /* 省去"%s" */
    } else if ((start = strstr(req_url, tag2)) != NULL) {
        tag = 2;
        pattern_len = strlen(sc_sohu_origin_url_pattern2) - 2;  /* 省去"%s" */
    } else {
        return -1;
    }

    if (strstr(start, suffix) == NULL) {
        return -1;
    }

    if (tag == 1) {
        start = start + strlen(tag1);
    } else {
        start = start + strlen(tag2);
    }

    for (len = 0; start[len] != '?' && start[len] != '\0'; len++) {
        (void)0;
    }

    bzero(buf, sizeof(buf));
    if (len >= BUFFER_LEN) {
        return -1;
    }
    if (pattern_len + len >= HTTP_SP_URL_LEN_MAX) {
        return -1;
    }
    strncpy(buf, start, len);
    if (tag == 1) {
        sprintf(origin_url, sc_sohu_origin_url_pattern1, buf);
    } else {
        sprintf(origin_url, sc_sohu_origin_url_pattern2, buf);
    }

    return 0;
}

/**
 * NAME: sc_sohu_download
 *
 * DESCRIPTION:
 *      下载搜狐视频。
 *
 * @file_url:    -IN 搜狐视频file型url。
 * @url_id:      -IN 视频的url ID。
 *
 * RETURN: -1表示失败，0表示成功。
 */
int sc_sohu_download(char *file_url, ngx_int_t url_id)
{
    char response[BUFFER_LEN];
    char real_url[HTTP_SP_URL_LEN_MAX];
    int status, ret;
    int response_len;

    if (file_url == NULL || url_id < 0) {
        return -1;
    }

    bzero(response, sizeof(response));
    if (sohu_http_session(file_url, response, BUFFER_LEN) < 0) {
        hc_log_error("sohu_http_session faild, url: %s", file_url);
        return -1;
    }

    response_len = strlen(response);
    if (http_parse_status_line(response, response_len, &status) < 0) {
        hc_log_error("parse status line failed:\n%s", response);
        return -1;
    }

    if (status == NGX_HTTP_MOVED_PERMANENTLY) {
        (void)0;
    } else if (status == NGX_HTTP_MOVED_TEMPORARILY && strstr(response, HTTP_REDIRECT_URL)) {
        hc_log_debug("resource is already cached, url: %s", file_url);
        return 0;
    } else {
        hc_log_error("file_url response status code %d:\n%s", status, response);
        return -1;
    }

    bzero(real_url, HTTP_SP_URL_LEN_MAX);
    ret = sohu_parse_file_url_response(response, real_url);
    if (ret != 0) {
        hc_log_error("parse real_url failed, response is\n%s", response);
        return -1;
    }

    ret = sc_ngx_download(real_url, url_id);
    if (ret < 0) {
        hc_log_error("inform Nginx failed, url: %s", real_url);
    }

    return ret;
}

/*
 * 各种url举例:
 * m3u8_url:   hot.vrs.sohu.com/ipad1683703_4507722770245_4894024.m3u8?plat=0
 * file_url:   220.181.61.212/ipad?file=/109/193/XKUNcCADy8eM9ypkrIfhU4.mp4
 * local_path: 220.181.61.212/ipad_file=_109_193_XKUNcCADy8eM9ypkrIfhU4.mp4
 * real_url:   118.123.211.11/sohu/ts/zC1LzEwlslvesEAgoE.........
 */
static int sc_get_sohu_video_m3u8(char *origin_url)
{
    int err = 0, status, ret;
    char m3u8_url[HTTP_SP_URL_LEN_MAX];
    char file_url[HTTP_SP_URL_LEN_MAX];
    char *response = NULL, *curr;
    int response_len;
    ngx_str_t url_str;
    ngx_int_t url_id;

    if (origin_url == NULL) {
        hc_log_error("invalid input.");
        return -1;
    }

    memset(m3u8_url, 0, HTTP_SP_URL_LEN_MAX);
    sc_copy_url(m3u8_url, origin_url, HTTP_SP_URL_LEN_MAX, 0);

    response = malloc(RESP_BUF_LEN);
    if (response == NULL) {
        hc_log_error("allocate response buffer (%d bytes) failed", RESP_BUF_LEN);
        return -1;
    }

    bzero(response, RESP_BUF_LEN);
    if (sohu_http_session(m3u8_url, response, RESP_BUF_LEN) < 0) {
        hc_log_error("sohu_http_session faild, URL: %s", m3u8_url);
        err = -1;
        goto out;
    }

    response_len = strlen(response);
    if (http_parse_status_line(response, response_len, &status) < 0) {
        hc_log_error("parse status line failed:\n%s", response);
        err = -1;
        goto out;
    }

    if (status == NGX_HTTP_OK) {
        (void)0;
    } else {
        hc_log_error("m3u8 response status code %d:\n%s", status, response);
        err = -1;
        goto out;
    }

    for (curr = response; curr != NULL; ) {
        bzero(file_url, HTTP_SP_URL_LEN_MAX);
        curr = sohu_parse_m3u8_response(curr, file_url);

        url_str.data = (u_char *)file_url;
        url_str.len = strlen(file_url);
        /* 调用Snooping Module提供的接口，添加url */
        url_id = ngx_http_sp_url_handle(&url_str, HTTP_C2SP_ACTION_ADD);
        if (url_id < 0) {
            /* TODO: 以前添加成功的url怎么办? */
            hc_log_error("add url to Snooping Module failed: %s", file_url);
            continue;
        }

        ret = sc_sohu_download(file_url, url_id);
        if (ret != 0) {
            hc_log_error("sc_sohu_download failed, file url: %s", file_url);
            ngx_http_sp_url_state_setby_id(url_id, HTTP_LOCAL_DOWN_STATE_ERROR);
            continue;
        }
    }

out:
    free(response);

    return err;
}

/**
 * NAME: sc_get_sohu_video
 *
 * DESCRIPTION:
 *      根据最原始的url，下载搜狐视频。
 *
 * @origin_url:    -IN 搜狐视频原始url。
 *
 * RETURN: -1表示失败，0表示成功。
 */
int sc_get_sohu_video(char *origin_url)
{
    int ret = 0;

    if (origin_url == NULL || strlen(origin_url) >= HTTP_SP_URL_LEN_MAX) {
        hc_log_error("invalid origin_url: %s", origin_url);
        return -1;
    }

    if (sohu_is_m3u8_url(origin_url)) {
        ret = sc_get_sohu_video_m3u8(origin_url);
    } else {
        hc_log_error("no support: %s", origin_url);
        ret = -1;
    }

    return ret;
}

