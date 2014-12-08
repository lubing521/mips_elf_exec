/*
 * Copyright(C) 2014 Ruijie Network. All rights reserved.
 */
/*
 * ngx_snoop_client.c
 * Original Author: zhaoyao@ruijie.com.cn, 2014-04-28
 *
 * Snooping Client's common and mid-layer codes file. Mainly for interacting with Nginx and
 * Snooping Module:
 *      1) Send hot cache file URI to Nginx, and inform Nginx /getfile module to download
           corresponding file from third party (e.g. Youku, Sohu) with upstream method;
 *      2) API of Snooping Client to HTTP Snooping Module;
 *      3) Mid-layer face to video website, like Youku and Sohu.
 *
 * History:
 *  v1.0    zhaoyao@ruijie.com.cn   2014-04-28
 *          创建此文件。
 *
 */

#include <net/networklayer/inet_lib.h>
#include <app/syslog.h>
#include "ngx_snoop_client.h"

#define URL_ID_STR_LEN  32

int snoop_client_log_level = LOG_ERR;

static char sc_ngx_default_ip_addr[] = "127.0.0.1";
static uint16_t sc_ngx_default_port = 8089;

static char sc_ngx_get_pattern[] = "GET /getfile?%s%s HTTP/1.1\r\n"
                                   "Host: %s\r\n"
                                   "Connection: close\r\n\r\n";

static int sc_ngx_build_get(const char *ip,
                            const char *uri,
                            const ngx_int_t url_id,
                            char *buf,
                            unsigned int len)
{
    char urlID[URL_ID_STR_LEN];
    char *para1 = "&urlID=%d";
    char *para2 = "?urlID=%d";
    unsigned int len_real;

    if (buf == NULL || uri == NULL || url_id < 0 || ip == NULL || len != BUFFER_LEN) {
        return -1;
    }

    bzero(urlID, sizeof(urlID));
    if (strchr(uri, '?')) {
        sprintf(urlID, para1, url_id);  /* url_id最多19位，urlID不会溢出 */
    } else {
        sprintf(urlID, para2, url_id);
    }

    len_real = strlen(sc_ngx_get_pattern) + strlen(ip) + strlen(uri) + strlen(urlID);
    len_real -= 6;  /* 去掉%s无关字符，共6个 */
    if (len <= len_real) {
        return -1;
    }

    sprintf(buf, sc_ngx_get_pattern, uri, urlID, ip);

    hc_log_debug("request: %s", buf);

    return 0;
}

/**
 * NAME: sc_ngx_download
 *
 * DESCRIPTION:
 *      调用接口；
 *      将资源真实的URL告知Nginx，使用upstream方式下载资源。
 *
 * @url:        -IN 资源真实的URL，注意是去掉"http://"的，例如58.211.22.175/youku/x/xxx.flv
 * @url_id:     -IN url资源列表中的index
 *
 * RETURN: -1表示失败，0表示成功。
 */
int sc_ngx_download(char *url, ngx_int_t url_id)
{
    int sockfd;
    struct sockaddr_in sa;
    socklen_t salen;
    char *ip_addr;
    char buffer[BUFFER_LEN];
    int nsend, nrecv, len;
    int err = 0, status;

    ip_addr = sc_ngx_default_ip_addr;

    if (url == NULL || url_id < 0) {
        hc_log_error("Invalid argument");
        return -1;
    }

    if (memcmp(url, HTTP_URL_PREFIX, HTTP_URL_PRE_LEN) == 0) {
        hc_log_debug("Input url should not begin with \"http://\"");
        url = url + HTTP_URL_PRE_LEN;
    }

    bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((uint16_t)sc_ngx_default_port);
    if (inet_aton(ip_addr, (struct in_addr *)&sa.sin_addr) < 0) {
        hc_log_error("Inet_aton failed");
        return -1;
    }
    salen = sizeof(sa);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        hc_log_error("Socket failed");
        return -1;
    }

    if (sock_conn_retry(sockfd, (struct sockaddr *)&sa, salen) < 0) {
        err = -1;
        hc_log_error("Socket conn failed");
        goto out;
    }

    memset(buffer, 0, BUFFER_LEN);
    if (sc_ngx_build_get(ip_addr, url, url_id, buffer, BUFFER_LEN) < 0) {
        hc_log_error("sc_ngx_build_get failed");
        err = -1;
        goto out;
    }
    len = strlen(buffer) + 1; /* 加上结束符'\0' */

    nsend = send(sockfd, buffer, len, 0);
    if (nsend != len) {
        hc_log_error("Send failed");
        err = -1;
        goto out;
    }

    memset(buffer, 0, BUFFER_LEN);
    nrecv = recv(sockfd, buffer, BUFFER_LEN, MSG_WAITALL);
    if (nrecv <= 0) {
        hc_log_error("Recv failed or meet EOF");
        err = -1;
        goto out;
    }

    if (http_parse_status_line(buffer, nrecv, &status) < 0) {
        hc_log_error("Parse status line failed:\n%s", buffer);
        err = -1;
        goto out;
    }

    if (status == NGX_HTTP_OK) {
        (void)0;
    } else {
        if (status == NGX_HTTP_MOVED_TEMPORARILY) {
            if (strstr(buffer, HTTP_REDIRECT_URL) != NULL) {
                /* XXX: 这不是错误，资源已被缓存且被设备成功重定向 */
                hc_log_debug("WARNING: resource already cached: %s", url);
                err = 0;
                goto out;
            }
        }
        hc_log_error("Response status code %d:\n%s", status, buffer);
        err = -1;
        goto out;
    }

out:
    close(sockfd);

    return err;
}

static int ngx_sc_add_parse(char *url)
{
    int ret;
    char origin_url[HTTP_SP_URL_LEN_MAX];

    if (url == NULL) {
        hc_log_error("Invalid input");
        return -1;
    }
    if (strlen(url) >= HTTP_SP_URL_LEN_MAX) {
        hc_log_error("url longer than %d: %s", HTTP_SP_URL_LEN_MAX, url);
        return -1;
    }

    hc_log_debug("request url: %s", url);

    /* 使用固定的pattern生成原始url */
    bzero(origin_url, HTTP_SP_URL_LEN_MAX);
    ret = sc_gen_origin_url(url, origin_url);
    if (ret != 0) {
        hc_log_error("generate origin url failed, request url: %s", url);
        return -1;
    }

    /*
     * XXX TODO FIXME: sc_get_xxx_video的行为需要明确的定义，才能知道在出错时如何处理origin。
     *                 错误最可能是添加parsed失败、同服务器通信失败，这都需要对origin进行容错
     *                 处理。
     */
    if (sc_url_is_yk(origin_url)) {
        ret = sc_get_yk_video(origin_url);
    } else if (sc_url_is_sohu(origin_url)) {
        ret = sc_get_sohu_video(origin_url);
    } else {
        hc_log_error("url not support: %s", origin_url);
        return -1;
    }

    if (ret != 0) {
        hc_log_error("parse or down %s failed", origin_url);
    }

    return ret;
}

/*
 * XXX: 注意，操作HTTP_SP2C_ACTION_DOWN可能发生在两种情景下:
 *      1) Snooping模块直接让Nginx去下载一个连接，而不需要parse过程；
 *      2) 在下载失败时，Snooping模块会再次调用down，重新下载。
 */
static int ngx_sc_add_down(char *url)
{
    int ret;
    unsigned int url_len;
    ngx_str_t url_str;
    ngx_int_t url_id;

    if (url == NULL) {
        hc_log_error("Invalid input");
        return -1;
    }
    url_len = strlen(url);
    if (url_len >= HTTP_SP_URL_LEN_MAX) {
        hc_log_error("url longer than %d: %s", HTTP_SP_URL_LEN_MAX, url);
        return -1;
    }

    bzero(&url_str, sizeof(url_str));
    url_str.data = (u_char *)url;
    url_str.len = url_len;
    /* 不管是新的下载还是重新下载，都直接调用ngx_http_sp_url_handle添加url */
    url_id = ngx_http_sp_url_handle(&url_str, HTTP_C2SP_ACTION_ADD);
    if (url_id < 0) {
        hc_log_error("add url to Snooping Module failed, url: %s", url);
        return -1;
    }

    if (sc_url_is_sohu_file_url(url)) {
        /*
         * XXX TODO: 当重下载sohu视频资源时，Snooping直接发送down的action，这时需特殊处理;
         *           本方法仅作为临时处理，后期需改进。
         */
        ret = sc_sohu_download(url, url_id);
    } else {
        ret = sc_ngx_download(url, url_id);
    }

    if (ret != 0) {
        hc_log_error("download failed: %s", url);
        ngx_http_sp_url_state_setby_id(url_id, HTTP_LOCAL_DOWN_STATE_ERROR);
        return -1;
    }

    hc_log_info("inform Nginx to download %s success", url);

    return 0;
}

/**
 * NAME: ngx_http_server_url_handle
 *
 * DESCRIPTION:
 *      根据具体操作，处理http url。
 *
 * @url_name:   -IN 资源url，可能不是最终url，因此中间可能会有解析得到最终url的过程。
 * @action:     -IN 操作类型。
 *
 * RETURN: -1表示失败，0表示成功。
 */
ngx_int_t ngx_http_server_url_handle(ngx_str_t *url_name, u8 action)
{
    char url[HTTP_SP_URL_LEN_MAX];  /* 需要的本地副本 */
    int ret;

    if (url_name == NULL) {
        return -1;
    }
    if (url_name->len >= HTTP_SP_URL_LEN_MAX) {
        hc_log_error("url len %d, longer than %d", url_name->len, HTTP_SP_URL_LEN_MAX);
        return -1;
    }

    ngx_memzero(url, HTTP_SP_URL_LEN_MAX);
    if (ngx_strncmp(url_name->data, HTTP_URL_PREFIX, HTTP_URL_PRE_LEN) == 0) {
        ngx_memcpy(url, url_name->data + HTTP_URL_PRE_LEN, url_name->len - HTTP_URL_PRE_LEN);
        hc_log_debug("WARNING: input url_name has \"http://\"");
    } else {
        ngx_memcpy(url, url_name->data, url_name->len);
    }

    switch (action) {
    case HTTP_SP2C_ACTION_PARSE:
        ret = ngx_sc_add_parse(url);
        break;
    case HTTP_SP2C_ACTION_DOWN:
        ret = ngx_sc_add_down(url);
        break;
    default:
        hc_log_debug("unknown action %u", action);
        ret = -1;
    }

    if (ret != 0) {
        hc_log_error("handle %s failed", url);
    }

    return ret;
}

/**
 * NAME: sc_gen_origin_url
 *
 * DESCRIPTION:
 *      将请求的url转换为固定模式的url。
 *
 * @req_url:    -IN  请求的url。
 * @origin_url: -OUT 转换得到的固定模式url。
 *
 * RETURN: -1表示失败，0表示成功。
 */
int sc_gen_origin_url(char *req_url, char *origin_url)
{
    int ret;

    if (req_url == NULL || origin_url == NULL) {
        return -1;
    }

    if (sc_url_is_yk(req_url)) {
        ret = sc_yk_gen_origin_url(req_url, origin_url);
    } else if (sc_url_is_sohu(req_url)) {
        ret = sc_sohu_gen_origin_url(req_url, origin_url);
    } else {
        ret = -1;
    }

    return ret;
}

void sc_log_level_config(struct_command_data_block *pcdb);
EOLWOS(cfg_sc_log_eol, sc_log_level_config);
NUMBER(cfg_sc_log_num, cfg_sc_log_eol, no_alt, "Log level, [LOG_EMERG(0), LOG_DEBUG(7)]", \
            CDBVAR(int, 1), LOG_EMERG, LOG_DEBUG);
C2PWOS(cfg_sc_log_c2p, cfg_sc_log_num, sc_log_level_config);
KEYWORD(cfg_sc_log_kw, cfg_sc_log_c2p, no_alt, \
            "log-level", "Setting log level", PRIVILEGE_CONFIG);
KEYWORD(cfg_snoop_client_kw, cfg_sc_log_kw, no_alt, \
            "snoop-client", "Snooping Client configuration", PRIVILEGE_CONFIG);
APPEND_POINT(cfg_snoop_client_append_point, cfg_snoop_client_kw);

void sc_log_level_config(struct_command_data_block *pcdb)
{
    int level;

    if (pcdb->parser_status & (SAVE_PARAM | CLEAN_PARAM)) {
        /* c2p mode */
        c2p_printf(TRUE, pcdb, "snoop-client log-level %d", snoop_client_log_level);
        return;
    }

    if (pcdb->flag_nd_prefix) {
        return;
    }

    level = GETCDBVAR(int, 1);
    snoop_client_log_level = level;

    return;
}


