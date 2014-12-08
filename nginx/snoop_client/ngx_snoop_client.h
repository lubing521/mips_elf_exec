/*
 * Copyright(C) 2014 Ruijie Network. All rights reserved.
 */
/*
 * ngx_snoop_client.h
 * Original Author: zhaoyao@ruijie.com.cn, 2014-03-11
 *
 * Hot cache Snooping Client's common header file.
 *
 * History:
 *  v1.0    zhaoyao@ruijie.com.cn   2014-03-11
 *          创建此文件。
 *
 */

#ifndef _NGX_SNOOP_CLIENT_H_
#define _NGX_SNOOP_CLIENT_H_

#include <sys/string.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <net/networklayer/inet.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ef/ef_buffer/ef_buff.h>
#include <ef/ef_buffer/ef_buff_utils.h>
#include <avllib/avlLib.h>
#include <app/syslog.h>
#include "../http-snooping/http-snooping.h"

#define BUFFER_LEN          1024

#define MAX_HOST_NAME_LEN   32
#define MAX_SLEEP           32      /* TCP connect maximum retry time (sec) */

#define HTTP_URL_PREFIX    "http://"
#define HTTP_URL_PRE_LEN    7       /* strlen("http://") */
#define HTTP_PORT_DEFAULT   80

#define HTTP_REDIRECT_URL  "Location: http://"

#define IP_ADDR_STR_MAX_LEN 15
#define IP_ADDR_STR_MIN_LEN 7
#define IP_ADDR_DIGITS_MAX  3
#define IP_ADDR_MAX_DOT_CNT 3

#define RESP_BUF_LEN       (0x1 << 15)         /* 32KB */

#define true 1
#define false 0

#define VIDEO_FLV_SUFFIX       "flv"
#define VIDEO_FLV_SUFFIX_LEN    3
#define VIDEO_MP4_SUFFIX       "mp4"
#define VIDEO_MP4_SUFFIX_LEN    3

#define SC_NGX_ROOT_PATH       "/mnt/usb0/"
#define SC_NGX_ROOT_PATH_LEN  (sizeof(SC_NGX_ROOT_PATH) - 1)

extern int snoop_client_log_level;

#define hc_log_info(fmt, arg...) \
    do { \
        if (snoop_client_log_level >= LOG_INFO) { \
            printk("*INFO*  %s: " fmt "\n", __func__, ##arg); \
        } \
    } while (0)

#define hc_log_debug(fmt, arg...) \
    do { \
        if (snoop_client_log_level >= LOG_DEBUG) { \
            printk("*DEBUG* %s[%d]: " fmt "\n", __func__, __LINE__, ##arg); \
        } \
    } while (0)

#define hc_log_error(fmt, arg...) \
    do { \
        if (snoop_client_log_level >= LOG_ERR) { \
            printk("*ERROR* %s[%d]: " fmt "\n", __func__, __LINE__, ##arg); \
        } \
    } while (0)

int sock_conn_retry(int sockfd, const struct sockaddr *addr, socklen_t alen);
int http_parse_status_line(char *buf, int len, int *status);
int sc_gen_origin_url(char *req_url, char *origin_url);
int sc_yk_gen_origin_url(char *req_url, char *origin_url);
int sc_sohu_gen_origin_url(char *req_url, char *origin_url);
int sc_url_is_yk(char * url);
int sc_get_yk_video(char * origin_url);
int sc_url_is_sohu(char * url);
int sc_get_sohu_video(char * origin_url);
int sc_url_is_sohu_file_url(char * url);
int sc_sohu_download(char * file_url,ngx_int_t url_id);
int sc_ngx_download(char * url,ngx_int_t url_id);
void sc_copy_url(char *url, char *o_url, unsigned int len, char with_para);
int host_connect(const char *hostname);

#endif  /* _NGX_SNOOP_CLIENT_H_ */

