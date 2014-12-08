/*
 * Copyright(C) 2014 Ruijie Network. All rights reserved.
 */
/*
 * sohu_lib.c
 * Original Author: zhaoyao@ruijie.com.cn, 2014-04-28
 *
 * Sohu related library.
 *
 * History:
 *  v1.0    zhaoyao@ruijie.com.cn   2014-04-28
 *          创建此文件。
 *
 */

#include "ngx_snoop_client.h"
#include "sohu_lib.h"

#define SOHU_HTTP_RECV_DELAY    10 /* 秒 */

static char sohu_request_pattern[] =
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Connection: close\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
    "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
                "Chrome/33.0.1750.154 Safari/537.36\r\n"
    "Accept-Encoding: gzip,deflate,sdch\r\n"
    "Accept-Language: en-US,en;q=0.8,zh-CN;q=0.6,zh;q=0.4\r\n\r\n";

static int sohu_build_request(char *host, char *uri, char *buf)
{
    int len;

    if (buf == NULL || host == NULL || uri == NULL) {
        return -1;
    }

    /* XXX: 计算时，忽略去掉无关的%s字符 */
    len = strlen(host) + strlen(uri) + strlen(sohu_request_pattern) - 4;
    if (len >= BUFFER_LEN) {
        hc_log_error("request length (%d) exceed limit %d", len, BUFFER_LEN);
        return -1;
    }

    sprintf(buf, sohu_request_pattern, uri, host);

    return 0;
}

/**
 * NAME: sohu_http_session
 *
 * DESCRIPTION:
 *      与搜狐服务器的http会话。
 *
 * @url:        -IN  资源url。
 * @response    -OUT HTTP响应内容。
 * @resp_len    -IN  HTTP响应的缓冲空间长度。
 *
 * RETURN: -1表示失败，0表示成功。
 */
int sohu_http_session(char *url, char *response, unsigned long resp_len)
{
    int sockfd = -1, err = 0;
    char host[MAX_HOST_NAME_LEN], *uri_start;
    char buffer[BUFFER_LEN];
    int nsend, nrecv, len, i, total_recv;
    struct timeval tv;
    fd_set r_set;
    int ret;

    if (url == NULL || response == NULL) {
        hc_log_error("input is invalid");
        return -1;
    }

    if (resp_len < BUFFER_LEN) {
        hc_log_error("buffer too small(%lu), minimum should be %d", resp_len, BUFFER_LEN);
        return -1;
    }

    if (resp_len > RESP_BUF_LEN) {
        hc_log_error("buffer too large(%lu), maximum should be %d", resp_len, RESP_BUF_LEN);
        return -1;
    }

    memset(host, 0, MAX_HOST_NAME_LEN);
    for (i = 0; (url[i] != '/') && (url[i] != '\0') && (i < MAX_HOST_NAME_LEN - 1); i++) {
        host[i] = url[i];
    }
    if (url[i] == '\0' || i >= MAX_HOST_NAME_LEN - 1) {
        hc_log_error("url is invalid: %s", url);
        return -1;
    }
    host[i] = '\0';
    uri_start = url + i;

    sockfd = host_connect(host);
    if (sockfd < 0) {
        hc_log_error("can not connect web(80) to %s", host);
        return -1;
    }

    memset(buffer, 0, BUFFER_LEN);
    if (sohu_build_request(host, uri_start, buffer) < 0) {
        hc_log_error("sohu_build_request failed");
        err = -1;
        goto out;
    }
    
    len = strlen(buffer);
    if (len >= BUFFER_LEN) {
        hc_log_error("request too long, host %s, uri %s", host, uri_start);
        err = -1;
        goto out;
    }
    nsend = send(sockfd, buffer, len, 0);
    if (nsend != len) {
        hc_log_error("send %d bytes failed", nsend);
        err = -1;
        goto out;
    }

    memset(response, 0, resp_len);
    total_recv = 0;
    bzero(&tv, sizeof(tv));
    tv.tv_sec = SOHU_HTTP_RECV_DELAY;
    tv.tv_usec = 0;

    while (1) {
        memset(&r_set, 0, sizeof(r_set));
        FD_SET(sockfd, &r_set);
        ret = select(sockfd + 1, &r_set, NULL, NULL, &tv);

        if (ret <= 0) {
            hc_log_error("recv select failed, ret: %d", ret);
            err = -1;
            goto out;
        }

        nrecv = recv(sockfd, response, resp_len - total_recv, MSG_WAITALL);
        if (nrecv > 0) {
            response = response + nrecv;
            total_recv = total_recv + nrecv;
        } else if (nrecv == 0) {
            break;
        } else {
            err = -1;
            goto out;
        }
    }

    if (total_recv <= 0) {
        hc_log_error("recv failed or meet EOF, %d", total_recv);
        err = -1;
        goto out;
    }
    if (total_recv == resp_len) {
        hc_log_error("WARNING: receive %d bytes, response buffer is full!!!", total_recv);
    }

out:
    close(sockfd);

    return err;
}

/**
 * NAME: sohu_is_m3u8_url
 *
 * DESCRIPTION:
 *      判断是否为搜狐m3u8格式的url。
 *      例如 hot.vrs.sohu.com/ipad1683703_4507722770245_4894024.m3u8?plat=0
 *
 * @sohu_url:   -IN 搜狐资源的url。
 *
 * RETURN: 1表示是，0表示不是。
 */
int sohu_is_m3u8_url(char *sohu_url)
{
    static char *tag1 = "sohu.com";
    static char *tag2 = ".m3u8";
    char *cur;

    if (sohu_url == NULL) {
        return 0;
    }

    cur = sohu_url;
    cur = strstr(cur, tag1);
    if (cur == NULL) {
        return 0;
    }

    cur = strstr(cur, tag2);
    if (cur == NULL) {
        return 0;
    }

    return 1;
}

/**
 * NAME: sohu_parse_m3u8_response
 *
 * DESCRIPTION:
 *      解析搜狐视频m3u8型url会话的应答内容。
 *
 * @curr:       -IN  开始解析的位置。
 * @file_url    -OUT 返回解析得到的file型url。
 *
 * RETURN: 当前解析到的位置。
 */
char *sohu_parse_m3u8_response(char *curr, char *file_url)
{
    char *ret = NULL, *p;
    char *tag1 = "file=";
    char *tag2 = "#EXT-X-DISCONTINUITY";
    unsigned long len;

    if (curr == NULL || file_url == NULL) {
        hc_log_error("Invalid input");
        return ret;
    }

    curr = strstr(curr, HTTP_URL_PREFIX);
    if (curr == NULL) {
        hc_log_error("do not find %s", HTTP_URL_PREFIX);
        return ret;
    }

    curr = curr + HTTP_URL_PRE_LEN;
    p = strstr(curr, tag1);
    if (p == NULL) {
        hc_log_error("do not find %s", tag1);
        return ret;
    }
    for ( ; *p != '&' && *p != '\0'; p++) {
        (void)0;
    }
    len = (unsigned long)p - (unsigned long)curr;
    if (len >= HTTP_SP_URL_LEN_MAX) {
        hc_log_error("parsed url len %lu, exceed limit %u", len, HTTP_SP_URL_LEN_MAX);
    } else {
        strncpy(file_url, curr, len);
    }

    ret = strstr(curr, tag2);

    return ret;
}

/**
 * NAME: sohu_parse_file_url_response
 *
 * DESCRIPTION:
 *      解析搜狐视频file型url会话的应答内容。
 *
 * @response:   -IN  http应答。
 * @real_url    -OUT 返回解析得到的最终url。
 *
 * RETURN: -1表示失败，0表示成功。
 */
int sohu_parse_file_url_response(char *response, char *real_url)
{
    char *tag = "Location:";
    char *p;
    int len;

    if (response == NULL || real_url == NULL) {
        return -1;
    }

    p = strstr(response, tag);
    if (p == NULL) {
        return -1;
    }

    p = strstr(p, HTTP_URL_PREFIX);
    if (p == NULL) {
        return -1;
    }

    p = p + HTTP_URL_PRE_LEN;
    len = strlen(p);
    if (len >= HTTP_SP_URL_LEN_MAX) {
        hc_log_error("real url is too long:\n%s", p);
        return -1;
    }

    strcpy(real_url, p);
    /* real_url是以"\r\n\r\n"结尾的，需去掉他们 */
    real_url[len - 4] = '\0';

    return 0;
}

