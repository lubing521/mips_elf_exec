/*
 * Copyright(C) 2014 Ruijie Network. All rights reserved.
 */

/*
 * http-snooping.c
 * Original Author:  tangyoucan@ruijie.com.cn, 2014-3-10
 * 
 *  HTTP Snooping功能代码，实现URL的监控，请求的重定向
 * 
 * History   
 *   v1.0 tangyoucan@ruijie.com.cn 2014-3-10
 *             创建此文件
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ef/ef_linklayer/ef_im/ef_im.h>
#include <ef/ef_buffer/ef_buff.h>
#include <ef/ef_buffer/ef_buff_utils.h>
#include <net/networklayer/fpm/fpm_lib.h>
#include <net/networklayer/fpm2/fpm2_lib.h>
#include <sys/list.h>
#include <zlib/zlib.h>
#include <wan_ta_proxy.h>
#include <wan_ta_protocol.h>
#include <wan_ta_packet.h>
#include <wan_ta_protocol.h>
#include <wan_ta_buf.h>
#include <avllib/avlLib.h>
#include "http-snooping.h"

#define MAX(a,b)            ((a)>(b)?(a):(b))

ngx_uint_t ngx_http_resolver = 0xc0a83a6e;
http_snooping_ctx_t g_http_snooping_ctx;
static ngx_int_t ngx_http_snooping_create(tcp_proxy_cb_t *tp_entry);
static void ngx_http_snooping_shut(tcp_proxy_cb_t *tp_entry);
static void ngx_http_snooping_data(tcp_proxy_cb_t *tp_entry);
void ngx_http_snooping_url2local_file(ngx_str_t *url_str, ngx_str_t *local_file, http_cache_url_t *pcache_rule);

#define HTTP_URL_ID_GET(url_node) (url_node - g_http_snooping_ctx.url_base)

//ngx_uint_t ngx_server_array[HTTP_SP_NGX_SERVER_MAX];
ngx_uint_t ngx_server_addr = 0;
ngx_int_t ngx_c2sp_socket = -1;
ngx_int_t ngx_sp2c_socket = -1;
ngx_int_t ngx_write_tick = 0;
wan_ta_app_t ngx_http_snooping_app = 
{
    ngx_http_snooping_create, 
    ngx_http_snooping_data, 
    ngx_http_snooping_shut
};

static task_t g_ngx_sp_write_task = NULL;
static volatile int g_ngx_sp_write_task_exiting = 0;
static task_t g_ngx_sp_rxtx_task = NULL;
static volatile int g_ngx_sp_rxtx_task_exiting = 0;

ngx_str_t http_end = ngx_string("\r\n\r\n");
ngx_str_t http_host = ngx_string("Host: ");
ngx_str_t http_method_get = ngx_string("GET ");
ngx_str_t http_method_post = ngx_string("POST ");
ngx_str_t http_method_options = ngx_string("OPTIONS ");
ngx_str_t http_method_head = ngx_string("HEAD ");
ngx_str_t http_method_delete = ngx_string("DELETE ");
ngx_str_t http_method_trace = ngx_string("TRACE ");
ngx_str_t http_method_connect = ngx_string("CONNECT ");
ngx_str_t http_content_len = ngx_string("Content-Length: ");
ngx_str_t http_content_res_range_byte = ngx_string("Content-Range: bytes=");
ngx_str_t http_content_req_range_byte = ngx_string("Range: bytes=");
ngx_str_t http_content_type = ngx_string("Content-Type: ");
ngx_str_t http_accept_range_byte =  ngx_string("Accept-Ranges: bytes\r\n");
ngx_str_t http_trans_encoding_type_chunked = ngx_string("Transfer-Encoding: chunked\r\n");
ngx_str_t http_chunked_footer = ngx_string("0\r\n");
ngx_str_t http_content_encoding_type = ngx_string("Content-Encoding: ");
ngx_str_t http_content_encoding_none_type = ngx_string("none");
ngx_str_t http_content_encoding_gzip_type = ngx_string("gzip"); 
ngx_str_t http_content_encoding_deflate_type = ngx_string("deflate"); 
ngx_str_t *http_encoding_type_array[] = {
    &http_content_encoding_none_type,
    &http_content_encoding_gzip_type,
    &http_content_encoding_deflate_type
};

ngx_str_t http_new_line = ngx_string("\r\n");
ngx_str_t http_cache_file_root = ngx_string("/mnt/usb0/html/");
AVL_TREE http_cache_file_tree = NULL;
http_cache_file_avlnode_t *http_cache_file_base = NULL;
ngx_str_t http_cache_file_null = HTTP_NGX_NULL_STR;
void ngx_http_sohu_para_key(ngx_str_t *para);

http_cache_url_t http_cache_file[] = {
    {HTTP_CACHE_URL_HEAD_XIEGAN | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_UPSTREAM | HTTP_CACHE_URL_REDIRECT_PARA | HTTP_CACHE_URL_CON_CLOSE,
        HTTP_CACHE_KEY_1_XIEGAN | HTTP_CACHE_KEY_FILE, 
        ngx_string("/youku"),HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR, 
        ngx_string(".flv"), HTTP_NGX_NULL_STR, NULL, NULL},
    {HTTP_CACHE_URL_HEAD_XIEGAN | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_UPSTREAM | HTTP_CACHE_URL_REDIRECT_PARA | HTTP_CACHE_URL_CON_CLOSE,
        HTTP_CACHE_KEY_1_XIEGAN | HTTP_CACHE_KEY_FILE, 
        ngx_string("/youku"),HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR, 
        ngx_string(".mp4"), HTTP_NGX_NULL_STR, NULL, NULL},
     /* /ipad?file=/208/214/7G7WIe0YEdqgt6bRkEBTd2.mp4&*/
    {HTTP_CACHE_URL_TAIL_XIEGAN | HTTP_CACHE_URL_ACTION_CACHE_UPSTREAM | HTTP_CACHE_URL_REDIRECT_PARA | HTTP_CACHE_URL_CON_CLOSE,
        HTTP_CACHE_KEY_FILE | HTTP_CACHE_KEY_PARA, 
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, ngx_string("/ipad"), 
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, NULL, ngx_http_sohu_para_key},
    {HTTP_CACHE_URL_HEAD_XIEGAN | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_UPSTREAM | HTTP_CACHE_URL_REDIRECT_PARA | HTTP_CACHE_URL_CON_CLOSE,
        HTTP_CACHE_KEY_1_XIEGAN | HTTP_CACHE_KEY_FILE, 
        ngx_string("/sohu.vodnew.lxdns.com"),HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR, 
        ngx_string(".mp4"), HTTP_NGX_NULL_STR, NULL, NULL},
    /*{HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_PARA | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_SP_WRITE, 
        HTTP_CACHE_KEY_ALL_XIEGAN,
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR,
        ngx_string(".swf"), HTTP_NGX_NULL_STR, NULL, NULL},*/
    /*
    {HTTP_CACHE_URL_PARA | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_SP_WRITE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_ALL_XIEGAN,
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,
        ngx_string(".zip"), HTTP_NGX_NULL_STR, NULL, NULL},
    {HTTP_CACHE_URL_PARA | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_SP_WRITE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_ALL_XIEGAN,
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,
        ngx_string(".rar"), HTTP_NGX_NULL_STR, NULL, NULL},
    {HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_PARA | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_SP_WRITE,
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_ALL_XIEGAN,
        HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR, 
        ngx_string(".mov"), HTTP_NGX_NULL_STR, NULL, NULL}, */ 
    {HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_UPSTREAM | HTTP_CACHE_URL_REDIRECT_PARA | HTTP_CACHE_URL_CON_CLOSE,
        HTTP_CACHE_KEY_FILE,
        HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR, 
        ngx_string(".flv"), HTTP_NGX_NULL_STR, NULL, NULL},
    {HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_UPSTREAM | HTTP_CACHE_URL_REDIRECT_PARA | HTTP_CACHE_URL_CON_CLOSE,
        HTTP_CACHE_KEY_FILE,
        HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,
        ngx_string(".mp4"), HTTP_NGX_NULL_STR, NULL, NULL},
    {HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_ACTION_UPPARSE | HTTP_CACHE_URL_REDIRECT_PARA | HTTP_CACHE_URL_CON_CLOSE,
        HTTP_CACHE_KEY_FILE,
        HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,
        ngx_string(".m3u8"), HTTP_NGX_NULL_STR, NULL, NULL},
    /*{ HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_SP_WRITE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_ALL_XIEGAN,
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR,
        ngx_string(".gif"), HTTP_NGX_NULL_STR, NULL, NULL},
    {   HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_SP_WRITE,
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_ALL_XIEGAN,
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, 
        ngx_string(".jpg"), HTTP_NGX_NULL_STR, NULL, NULL},
    {HTTP_CACHE_URL_PARA | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_SP_WRITE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_ALL_XIEGAN,
        HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR, 
        ngx_string(".jpeg"), HTTP_NGX_NULL_STR, NULL, NULL},
    {HTTP_CACHE_URL_PARA | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_SP_WRITE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_ALL_XIEGAN,
        HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR, 
        ngx_string(".ico"), HTTP_NGX_NULL_STR, NULL, NULL}, 
    {HTTP_CACHE_URL_PARA | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_SP_WRITE,
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_ALL_XIEGAN,
        HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR, 
        ngx_string(".doc"), HTTP_NGX_NULL_STR, NULL, NULL},
    {HTTP_CACHE_URL_PARA | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_SP_WRITE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_ALL_XIEGAN,
        HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR, 
        ngx_string(".ppt"), HTTP_NGX_NULL_STR, NULL, NULL},
    {HTTP_CACHE_URL_PARA | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_SP_WRITE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_ALL_XIEGAN,
        HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR, 
        ngx_string(".xls"), HTTP_NGX_NULL_STR, NULL, NULL},
    {HTTP_CACHE_URL_PARA | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_SP_WRITE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_ALL_XIEGAN,
        HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR, 
        ngx_string(".pdf"), HTTP_NGX_NULL_STR, NULL, NULL},
    {HTTP_CACHE_URL_PARA | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_SP_WRITE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_ALL_XIEGAN,
        HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR, 
        ngx_string(".apk"), HTTP_NGX_NULL_STR}, NULL, NULL,
    {HTTP_CACHE_URL_PARA | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_SP_WRITE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_ALL_XIEGAN,
        HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR, 
        ngx_string(".ipa"), HTTP_NGX_NULL_STR, NULL, NULL},
    {HTTP_CACHE_URL_PARA | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_SP_WRITE, 
        HTTP_CACHE_KEY_ALL_XIEGAN,
        HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR, 
        ngx_string(".exe"), HTTP_NGX_NULL_STR, NULL, NULL},*/
    {HTTP_CACHE_URL_HEAD_XIEGAN | HTTP_CACHE_URL_MID_XIEGAN | HTTP_CACHE_URL_TAIL_XIEGAN | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_HOMEPAGE_POLL | HTTP_CACHE_URL_CON_CLOSE, 
        HTTP_CACHE_KEY_FILE,
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, ngx_string("/"), 
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, NULL, NULL}
};

ngx_uint_t http_cache_url_file_num = sizeof(http_cache_file)/sizeof(http_cache_url_t);

http_mime_type_t http_mime_null = {AVL_NULL_NODE, {0, NULL}, {0, NULL}};
AVL_TREE http_mime_tree = NULL;
http_mime_type_t http_mime_type[] = {
    {AVL_NULL_NODE, ngx_string("text/html"), ngx_string(".html")},
    {AVL_NULL_NODE, ngx_string("text/html"), ngx_string(".htm")},
    {AVL_NULL_NODE, ngx_string("text/html"), ngx_string(".shtml")},
    {AVL_NULL_NODE, ngx_string("text/css"), ngx_string(".css")},
    {AVL_NULL_NODE, ngx_string("text/xml"), ngx_string(".xml")},
    {AVL_NULL_NODE, ngx_string("image/gif"), ngx_string(".gif")},
    {AVL_NULL_NODE, ngx_string("image/jpeg"), ngx_string(".jpeg")},
    {AVL_NULL_NODE, ngx_string("image/jpeg"), ngx_string(".jpg")},
    {AVL_NULL_NODE, ngx_string("application/javascript"), ngx_string(".js")},
    {AVL_NULL_NODE, ngx_string("application/atom+xml"), ngx_string(".atom")},
    {AVL_NULL_NODE, ngx_string("application/rss+xml"), ngx_string(".rss")},
    {AVL_NULL_NODE, ngx_string("text/mathml"), ngx_string(".mml")},
    {AVL_NULL_NODE, ngx_string("text/plain"), ngx_string(".txt")},
    {AVL_NULL_NODE, ngx_string("text/vnd.sun.j2me.app-descriptor"), ngx_string(".jad")},
    {AVL_NULL_NODE, ngx_string("text/vnd.wap.wml"), ngx_string(".wml")},
    {AVL_NULL_NODE, ngx_string("text/x-component"), ngx_string(".htc")},
    {AVL_NULL_NODE, ngx_string("image/png"), ngx_string(".png")},
    {AVL_NULL_NODE, ngx_string("image/tiff"), ngx_string(".tif")},
    {AVL_NULL_NODE, ngx_string("image/tiff"), ngx_string(".tiff")},
    {AVL_NULL_NODE, ngx_string("image/vnd.wap.wbmp"), ngx_string(".wbmp")},
    {AVL_NULL_NODE, ngx_string("image/x-icon"), ngx_string(".ico")},
    {AVL_NULL_NODE, ngx_string("image/x-jng"), ngx_string(".jng")},
    {AVL_NULL_NODE, ngx_string("image/x-ms-bmp"), ngx_string(".bmp")},
    {AVL_NULL_NODE, ngx_string("image/svg+xml"), ngx_string(".svg")},
    {AVL_NULL_NODE, ngx_string("image/svg+xml"), ngx_string(".svgz")},
    {AVL_NULL_NODE, ngx_string("image/webp"), ngx_string(".webp")},
    {AVL_NULL_NODE, ngx_string("application/font-woff"), ngx_string(".woff")},
    {AVL_NULL_NODE, ngx_string("application/java-archive"), ngx_string(".jar")},
    {AVL_NULL_NODE, ngx_string("application/java-archive"), ngx_string(".war")},
    {AVL_NULL_NODE, ngx_string("application/java-archive"), ngx_string(".ear")},
    {AVL_NULL_NODE, ngx_string("application/json"), ngx_string(".json")},
    {AVL_NULL_NODE, ngx_string("application/mac-binhex40"), ngx_string(".hqx")},
    {AVL_NULL_NODE, ngx_string("application/msword"), ngx_string(".doc")},
    {AVL_NULL_NODE, ngx_string("application/pdf"), ngx_string(".pdf")},
    {AVL_NULL_NODE, ngx_string("application/postscript"), ngx_string(".ps")},
    {AVL_NULL_NODE, ngx_string("application/postscript"), ngx_string(".eps")},
    {AVL_NULL_NODE, ngx_string("application/postscript"), ngx_string(".ai")},
    {AVL_NULL_NODE, ngx_string("application/rtf"), ngx_string(".rtf")},
    {AVL_NULL_NODE, ngx_string("application/vnd.ms-excel"), ngx_string(".xls")},
    {AVL_NULL_NODE, ngx_string("application/vnd.ms-fontobject"), ngx_string(".eot")},
    {AVL_NULL_NODE, ngx_string("application/vnd.ms-powerpoint"), ngx_string(".ppt")},
    {AVL_NULL_NODE, ngx_string("application/vnd.wap.wmlc"), ngx_string(".wmlc")},
    {AVL_NULL_NODE, ngx_string("application/vnd.google-earth.kml+xml"), ngx_string(".kml")},
    {AVL_NULL_NODE, ngx_string("application/vnd.google-earth.kmz"), ngx_string(".kmz")},
    {AVL_NULL_NODE, ngx_string("application/x-7z-compressed"), ngx_string(".7z")},
    {AVL_NULL_NODE, ngx_string("application/x-cocoa"), ngx_string(".cco")},
    {AVL_NULL_NODE, ngx_string("application/x-java-archive-diff"), ngx_string(".jardiff")},
    {AVL_NULL_NODE, ngx_string("application/x-java-jnlp-file"), ngx_string(".jnlp")},
    {AVL_NULL_NODE, ngx_string("application/x-makeself"), ngx_string(".run")},
    {AVL_NULL_NODE, ngx_string("application/x-perl"), ngx_string(".pl")},
    {AVL_NULL_NODE, ngx_string("application/x-perl"), ngx_string(".pm")},
    {AVL_NULL_NODE, ngx_string("application/x-pilot"), ngx_string(".prc")},
    {AVL_NULL_NODE, ngx_string("application/x-pilot"), ngx_string(".pdb")},
    {AVL_NULL_NODE, ngx_string("application/x-rar-compressed"), ngx_string(".rar")},
    {AVL_NULL_NODE, ngx_string("application/x-redhat-package-manager"), ngx_string(".rpm")},
    {AVL_NULL_NODE, ngx_string("application/x-sea"), ngx_string(".sea")},
    {AVL_NULL_NODE, ngx_string("application/x-shockwave-flash"), ngx_string(".swf")},
    {AVL_NULL_NODE, ngx_string("application/x-stuffit"), ngx_string(".sit")},
    {AVL_NULL_NODE, ngx_string("application/x-tcl"), ngx_string(".tcl")},
    {AVL_NULL_NODE, ngx_string("application/x-tcl"), ngx_string(".tk")},
    {AVL_NULL_NODE, ngx_string("application/x-x509-ca-cert"), ngx_string(".der")},
    {AVL_NULL_NODE, ngx_string("application/x-x509-ca-cert"), ngx_string(".pem")},
    {AVL_NULL_NODE, ngx_string("application/x-x509-ca-cert"), ngx_string(".crt")},
    {AVL_NULL_NODE, ngx_string("application/x-xpinstall"), ngx_string(".xpi")},
    {AVL_NULL_NODE, ngx_string("application/xhtml+xml"), ngx_string(".xhtml")},
    {AVL_NULL_NODE, ngx_string("application/zip"), ngx_string(".zip")},
    {AVL_NULL_NODE, ngx_string("application/octet-stream"), ngx_string(".bin")},
    {AVL_NULL_NODE, ngx_string("application/octet-stream"), ngx_string(".dll")},
    {AVL_NULL_NODE, ngx_string("application/octet-stream"), ngx_string(".exe")},
    {AVL_NULL_NODE, ngx_string("application/octet-stream"), ngx_string(".deb")},
    {AVL_NULL_NODE, ngx_string("application/octet-stream"), ngx_string(".dmg")},
    {AVL_NULL_NODE, ngx_string("application/octet-stream"), ngx_string(".iso")},
    {AVL_NULL_NODE, ngx_string("application/octet-stream"), ngx_string(".img")},
    {AVL_NULL_NODE, ngx_string("application/octet-stream"), ngx_string(".msi")},
    {AVL_NULL_NODE, ngx_string("application/octet-stream"), ngx_string(".msp")},
    {AVL_NULL_NODE, ngx_string("application/octet-stream"), ngx_string(".msm")},
    {AVL_NULL_NODE, ngx_string("application/vnd.openxmlformats-officedocument.wordprocessingml.document"), ngx_string(".docx")},
    {AVL_NULL_NODE, ngx_string("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"), ngx_string(".xlsx")},
    {AVL_NULL_NODE, ngx_string("application/vnd.openxmlformats-officedocument.presentationml.presentation"), ngx_string(".pptx")},
    {AVL_NULL_NODE, ngx_string("audio/midi"), ngx_string(".mid")},
    {AVL_NULL_NODE, ngx_string("audio/midi"), ngx_string(".midi")},
    {AVL_NULL_NODE, ngx_string("audio/midi"), ngx_string(".kar")},
    {AVL_NULL_NODE, ngx_string("audio/mpeg"), ngx_string(".mp3")},
    {AVL_NULL_NODE, ngx_string("audio/ogg"), ngx_string(".ogg")},
    {AVL_NULL_NODE, ngx_string("audio/x-m4a"), ngx_string("m4a")},
    {AVL_NULL_NODE, ngx_string("audio/x-realaudio"), ngx_string(".ra")},
    {AVL_NULL_NODE, ngx_string("video/3gpp"), ngx_string(".3gpp")},
    {AVL_NULL_NODE, ngx_string("video/3gpp"), ngx_string(".3gp")},
    {AVL_NULL_NODE, ngx_string("video/mp2t"), ngx_string(".mp2")},
    {AVL_NULL_NODE, ngx_string("video/mpeg"), ngx_string(".mp2")},
    {AVL_NULL_NODE, ngx_string("video/mpeg"), ngx_string(".mp3")},
    {AVL_NULL_NODE, ngx_string("video/mp4"), ngx_string(".mp4")},
    {AVL_NULL_NODE, ngx_string("video/mpeg"), ngx_string(".mpeg")},
    {AVL_NULL_NODE, ngx_string("video/mpeg"), ngx_string(".mpg")},
    {AVL_NULL_NODE, ngx_string("video/quicktime"), ngx_string(".mov")},
    {AVL_NULL_NODE, ngx_string("video/webm"), ngx_string(".webm")},
    {AVL_NULL_NODE, ngx_string("video/x-flv"), ngx_string(".flv")},
    {AVL_NULL_NODE, ngx_string("video/x-m4v"), ngx_string(".m4v")},
    {AVL_NULL_NODE, ngx_string("video/x-mng"), ngx_string(".mng")},
    {AVL_NULL_NODE, ngx_string("video/x-ms-asf"), ngx_string(".asx")},
    {AVL_NULL_NODE, ngx_string("video/x-ms-asf"), ngx_string(".asf")},
    {AVL_NULL_NODE, ngx_string("video/x-ms-wmv"), ngx_string(".wmv")},
    {AVL_NULL_NODE, ngx_string("video/x-msvideo"), ngx_string(".avi")}
};
ngx_uint_t http_mime_num = sizeof(http_mime_type)/sizeof(http_mime_type_t);

//Accept-Ranges: bytes\r\n
ngx_str_t http_con_str[] = {
    ngx_string("Connection: Keep-Alive\r\n"),
    ngx_string("Connection: close\r\n")
};
ngx_str_t http_redirect_301_str = ngx_string("HTTP/1.1 301 Moved Permanently\r\n");
ngx_str_t http_redirect_302_str = ngx_string("HTTP/1.1 302 Found\r\n");
ngx_str_t http_redirect_303_str = ngx_string("HTTP/1.1 303 See Other\r\n");
ngx_str_t http_redirect_307_str = ngx_string("HTTP/1.1 307 Temporary Redirect");
ngx_str_t http_client_403_str = ngx_string("HTTP/1.1 403 Forbidden\r\nConnection: Keep-Alive\r\n\r\n");
ngx_str_t http_status_ok_str = ngx_string("200");

http_cache_url_t http_youku_vidoe_html_url_str[] = {
    {HTTP_CACHE_URL_HEAD_XIEGAN | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP \
        | HTTP_CACHE_URL_ACTION_UPPARSE | HTTP_CACHE_URL_CON_CLOSE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_FILE,
        ngx_string("/v_show"), HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, 
        ngx_string(".html"), HTTP_NGX_NULL_STR, NULL, NULL},
    {HTTP_CACHE_URL_HEAD_XIEGAN | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP \
        | HTTP_CACHE_URL_ACTION_UPPARSE | HTTP_CACHE_URL_CON_CLOSE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_FILE,
        ngx_string("/v_show"),HTTP_NGX_NULL_STR,HTTP_NGX_NULL_STR, 
        ngx_string(".shtml"), HTTP_NGX_NULL_STR, NULL, NULL}
};

#if 0
ngx_str_t http_youku_ctl_host_str[] = {
    ngx_string("p-log.ykimg.com"),
    {0, NULL}
};

ngx_str_t http_youku_vidoe_str[] = {
    ngx_string("/youku/*/*.flv"),
    ngx_string("/youku/*/*.mp4"),
    {0, NULL}
};
#endif
void ngx_http_sohu_m3u8_file_key(ngx_str_t *para);
void ngx_http_pl_youku_para_key(ngx_str_t *para);

http_cache_url_t http_forbiden_rule = {
        HTTP_CACHE_URL_FORBIDEN, 
        0,
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, 
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, NULL, NULL
};

http_cache_url_t http_pl_youku_flvpath_url_str[] = {
    {HTTP_CACHE_URL_HEAD_XIEGAN | HTTP_CACHE_URL_TAIL_XIEGAN | HTTP_CACHE_URL_ACTION_CACHE_SP \
         | HTTP_CACHE_URL_ACTION_UPPARSE | HTTP_CACHE_URL_CON_CLOSE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_1_XIEGAN | HTTP_CACHE_KEY_2_XIEGAN | HTTP_CACHE_KEY_PARA,
        ngx_string("/playlist"), HTTP_NGX_NULL_STR, ngx_string("/m3u8"), 
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, NULL, ngx_http_pl_youku_para_key}
};

http_cache_url_t http_sohu_flvpath_url_str[] = {
    {HTTP_CACHE_URL_HEAD_XIEGAN | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP \
        | HTTP_CACHE_URL_ACTION_UPPARSE | HTTP_CACHE_URL_CON_CLOSE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_FILE,
        ngx_string("/ipad"), HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, 
        ngx_string(".m3u8"), HTTP_NGX_NULL_STR, ngx_http_sohu_m3u8_file_key, NULL},
    {HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_ACTION_UPPARSE \
        | HTTP_CACHE_URL_ACTION_UPPARSE | HTTP_CACHE_URL_CON_CLOSE, 
        HTTP_CACHE_KEY_HOST |HTTP_CACHE_KEY_FILE,
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, 
        ngx_string(".m3u8"), HTTP_NGX_NULL_STR, ngx_http_sohu_m3u8_file_key, NULL}
};

void ngx_http_youku_andriod_para_key(ngx_str_t *para);
http_cache_url_t http_youku_andriod_flvpath_url_str[] = {
    {HTTP_CACHE_URL_TAIL_XIEGAN | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_ACTION_UPPARSE \
        | HTTP_CACHE_URL_CON_CLOSE, 
        HTTP_CACHE_KEY_PARA,
        ngx_string("/common"), ngx_string("/v3"), ngx_string("/play"), 
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, NULL, ngx_http_youku_andriod_para_key}
};

http_cache_url_t http_valf_atm_youku_com_rule[] = {
    {HTTP_CACHE_URL_TAIL_XIEGAN | HTTP_CACHE_URL_REDIRECT_PARA | HTTP_CACHE_URL_ACTION_CACHE_UPSTREAM \
        | HTTP_CACHE_URL_CON_CLOSE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_FILE,
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, ngx_string("/vf"), 
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, NULL, NULL},
    {HTTP_CACHE_URL_TAIL_XIEGAN | HTTP_CACHE_URL_REDIRECT_PARA | HTTP_CACHE_URL_ACTION_CACHE_UPSTREAM \
        | HTTP_CACHE_URL_CON_CLOSE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_FILE,
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, ngx_string("/adv"), 
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, NULL, NULL}
};

http_cache_url_t http_m_aty_sohu_com_rule[] = {
    {HTTP_CACHE_URL_TAIL_XIEGAN | HTTP_CACHE_URL_REDIRECT_PARA | HTTP_CACHE_URL_ACTION_CACHE_UPSTREAM \
        | HTTP_CACHE_URL_CON_CLOSE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_FILE,
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, ngx_string("/m"), 
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, NULL, NULL}
};

http_cache_url_t http_wscctcdn_inter_qiyi_com_rule[] = {
    {HTTP_CACHE_URL_HEAD_XIEGAN | HTTP_CACHE_URL_REDIRECT_PARA | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_ACTION_CACHE_UPSTREAM \
        | HTTP_CACHE_URL_CON_CLOSE, 
        HTTP_CACHE_KEY_FILE,
        ngx_string("/videos"), HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, 
        ngx_string(".f4v"), HTTP_NGX_NULL_STR, NULL, NULL},
    {HTTP_CACHE_URL_HEAD_XIEGAN | HTTP_CACHE_URL_REDIRECT_PARA | HTTP_CACHE_URL_ACTION_CACHE_SP | HTTP_CACHE_URL_ACTION_CACHE_UPSTREAM \
        | HTTP_CACHE_URL_CON_CLOSE, 
        HTTP_CACHE_KEY_FILE,
        ngx_string("/videos"), HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, 
        ngx_string(".ts"), HTTP_NGX_NULL_STR, NULL, NULL}
};

http_cache_url_t http_cache_m_iqiyi_com_rule[] = {
    {HTTP_CACHE_URL_HEAD_XIEGAN | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP \
        | HTTP_CACHE_URL_ACTION_UPPARSE | HTTP_CACHE_URL_CON_CLOSE, 
        HTTP_CACHE_KEY_HOST |HTTP_CACHE_KEY_FILE,
        ngx_string("/dc"), HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, 
        ngx_string(".m3u8"), HTTP_NGX_NULL_STR, NULL, NULL}
};

void ngx_http_qq_com_para_key(ngx_str_t *para);
http_cache_url_t http_cache_vv_video_qq_com_rule[] = {
    {HTTP_CACHE_URL_TAIL_XIEGAN | HTTP_CACHE_URL_POST_FILE | HTTP_CACHE_URL_ACTION_CACHE_SP \
        | HTTP_CACHE_URL_ACTION_UPPARSE | HTTP_CACHE_URL_CON_CLOSE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_FILE | HTTP_CACHE_KEY_PARA,
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, ngx_string("/getinfo"), 
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, NULL, ngx_http_qq_com_para_key}
};

void ngx_http_ark_letv_com_para_key(ngx_str_t *para);
http_cache_url_t http_cache_ark_letv_com_rule[] = {
    {HTTP_CACHE_URL_TAIL_XIEGAN | HTTP_CACHE_URL_REDIRECT_PARA | HTTP_CACHE_URL_ACTION_CACHE_UPSTREAM \
        | HTTP_CACHE_URL_CON_CLOSE, 
        HTTP_CACHE_KEY_HOST | HTTP_CACHE_KEY_FILE | HTTP_CACHE_KEY_PARA,
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, ngx_string("/s"), 
        HTTP_NGX_NULL_STR, HTTP_NGX_NULL_STR, NULL, ngx_http_ark_letv_com_para_key}
};

AVL_TREE http_cache_host_tree = NULL;
http_cache_host_url_t http_cache_host_url[] = {
    {AVL_NULL_NODE, 0, ngx_string("v.youku.com"), &http_youku_vidoe_html_url_str[0], sizeof(http_youku_vidoe_html_url_str)/sizeof(http_cache_url_t)},
    {AVL_NULL_NODE, 0, ngx_string("pl.youku.com"), &http_pl_youku_flvpath_url_str[0], sizeof(http_pl_youku_flvpath_url_str)/sizeof(http_cache_url_t)},
    {AVL_NULL_NODE, 0, ngx_string("my.tv.sohu.com"), &http_sohu_flvpath_url_str[0], sizeof(http_sohu_flvpath_url_str)/sizeof(http_cache_url_t)},
    {AVL_NULL_NODE, 0, ngx_string("hot.vrs.sohu.com"), &http_sohu_flvpath_url_str[0], sizeof(http_sohu_flvpath_url_str)/sizeof(http_cache_url_t)},
    {AVL_NULL_NODE, 0, ngx_string("play.api.3g.youku.com"),&http_youku_andriod_flvpath_url_str[0], sizeof(http_youku_andriod_flvpath_url_str)/sizeof(http_cache_url_t)},
    {AVL_NULL_NODE, HTTP_CACHE_HOST_FORBIDEN, ngx_string("Fvid.atm.youku.com"), &http_forbiden_rule, 1},
    {AVL_NULL_NODE, HTTP_CACHE_HOST_FORBIDEN, ngx_string("atm.youku.com"), &http_forbiden_rule, 1},
    {AVL_NULL_NODE, HTTP_CACHE_HOST_FORBIDEN, ngx_string("html.atm.youku.com"), &http_forbiden_rule, 1},
    {AVL_NULL_NODE, HTTP_CACHE_HOST_FORBIDEN, ngx_string("valb.atm.youku.com"), &http_forbiden_rule, 1},
    {AVL_NULL_NODE, 0, ngx_string("valf.atm.youku.com"), &http_valf_atm_youku_com_rule[0], sizeof(http_valf_atm_youku_com_rule)/sizeof(http_cache_url_t)},
    {AVL_NULL_NODE, HTTP_CACHE_HOST_FORBIDEN, ngx_string("valo.atm.youku.com"), &http_forbiden_rule, 1},
    {AVL_NULL_NODE, HTTP_CACHE_HOST_FORBIDEN, ngx_string("valp.atm.youku.com"), &http_forbiden_rule, 1},
    {AVL_NULL_NODE, HTTP_CACHE_HOST_FORBIDEN, ngx_string("lstat.youku.com"), &http_forbiden_rule, 1},
    {AVL_NULL_NODE, HTTP_CACHE_HOST_FORBIDEN, ngx_string("speed.lstat.youku.com"), &http_forbiden_rule, 1},
    {AVL_NULL_NODE, HTTP_CACHE_HOST_FORBIDEN, ngx_string("urchin.lstat.youku.com"), &http_forbiden_rule, 1},
    {AVL_NULL_NODE, HTTP_CACHE_HOST_FORBIDEN, ngx_string("stat.youku.com"), &http_forbiden_rule, 1},
    {AVL_NULL_NODE, HTTP_CACHE_HOST_FORBIDEN, ngx_string("static.lstat.youku.com"), &http_forbiden_rule, 1},
    {AVL_NULL_NODE, HTTP_CACHE_HOST_FORBIDEN, ngx_string("valc.atm.youku.com"), &http_forbiden_rule, 1},
    {AVL_NULL_NODE, HTTP_CACHE_HOST_FORBIDEN, ngx_string("vid.atm.youku.com"), &http_forbiden_rule, 1},
    {AVL_NULL_NODE, HTTP_CACHE_HOST_FORBIDEN, ngx_string("walp.atm.youku.com"), &http_forbiden_rule, 1},
    {AVL_NULL_NODE, 0, ngx_string("ad.api.3g.youku.com"),&http_valf_atm_youku_com_rule[0], sizeof(http_valf_atm_youku_com_rule)/sizeof(http_cache_url_t)},
    {AVL_NULL_NODE, 0, ngx_string("m.aty.sohu.com"),&http_m_aty_sohu_com_rule[0], sizeof(http_m_aty_sohu_com_rule)/sizeof(http_cache_url_t)},
    /*爱奇艺视频规则*/
    {AVL_NULL_NODE, 0, ngx_string("wscctcdn.inter.qiyi.com"),&http_wscctcdn_inter_qiyi_com_rule[0], sizeof(http_wscctcdn_inter_qiyi_com_rule)/sizeof(http_cache_url_t)},
    {AVL_NULL_NODE, 0, ngx_string("data.video.iqiyi.com"),&http_wscctcdn_inter_qiyi_com_rule[0], sizeof(http_wscctcdn_inter_qiyi_com_rule)/sizeof(http_cache_url_t)},
    {AVL_NULL_NODE, 0, ngx_string("cache.m.iqiyi.com"),&http_cache_m_iqiyi_com_rule[0], sizeof(http_cache_m_iqiyi_com_rule)/sizeof(http_cache_url_t)},
    /*QQ视频规则*/
    {AVL_NULL_NODE, 0, ngx_string("vv.video.qq.com"),&http_cache_vv_video_qq_com_rule[0], sizeof(http_cache_vv_video_qq_com_rule)/sizeof(http_cache_url_t)},
    /*乐视广告规则*/
    {AVL_NULL_NODE, 0, ngx_string("ark.letv.com"),&http_cache_ark_letv_com_rule[0], sizeof(http_cache_ark_letv_com_rule)/sizeof(http_cache_url_t)}   
};

ngx_uint_t http_host_num = sizeof(http_cache_host_url)/sizeof(http_cache_host_url_t);

http_cache_host_url_t http_cache_default_host_url = {AVL_NULL_NODE, 0, HTTP_NGX_NULL_STR, &http_cache_file[0], sizeof(http_cache_file)/sizeof(http_cache_url_t)};

ngx_uint_t http_cache_host_url_num = sizeof(http_cache_host_url)/sizeof(http_cache_host_url_t);

ngx_uint_t http_homepage_hash[HTTP_MAX_HOST] = {0};
#define HTTP_HOMEPAGE_HASH_KEY(sip) (((sip)&0x0000ffff)%HTTP_MAX_HOST)

void http_server_config(struct_command_data_block *pcdb)
{
    //u32 ipaddr;
    if (pcdb == NULL) {
        return;
    }

    if (pcdb->parser_status & (SAVE_PARAM | CLEAN_PARAM)) {
        if ((ngx_server_addr != 0) || (ngx_http_resolver != 0)) {
            c2p_printf(TRUE, pcdb, "!");
            c2p_printf(TRUE, pcdb, "http-cache %a %a", ngx_server_addr, ngx_http_resolver);
            c2p_printf(TRUE, pcdb, "!");
        }
        return;
    } 

    ngx_server_addr = *(IPADDRTYPE *) GETCDBVAR(paddr, 1);
    ngx_http_resolver = *(IPADDRTYPE *) GETCDBVAR(paddr, 2);
    return;
}

EOLWOS(cfg_httpsp_eol, http_server_config);
IPADDR(cfg_httpsp_dns, cfg_httpsp_eol, no_alt, \
       "DNS Server address",  CDBVAR(paddr, 2));
IPADDR(cfg_httpsp_ip, cfg_httpsp_dns, no_alt, \
       "HTTP Server address",  CDBVAR(paddr, 1));
KEYWORD(cfg_httpsp_config, cfg_httpsp_ip, no_alt, \
              "http-cache","HTTP Cache server", PRIVILEGE_CONFIG);
C2PWOS(cfg_httpsp_c2p, cfg_httpsp_config, http_server_config);
APPEND_POINT(cfg_httpsp_app, cfg_httpsp_c2p);


void http_url_cache_show(struct_command_data_block *pcdb)
{
    ngx_int_t i, n;
    http_snooping_url_t *url_node;
    ngx_str_t url_str, local_file_str;
    uchar local_file_buf[HTTP_SP_URL_LEN_MAX + HTTP_LOCAL_FILE_ROOT_MAX];

    printf("HTTP snooping server: %a, resolver: %a\r\n", ngx_server_addr, ngx_http_resolver);
    printf("HTTP url cache %d/%d\r\n", 
           g_http_snooping_ctx.url_max, g_http_snooping_ctx.url_max - g_http_snooping_ctx.url_free);
    printf("Free connect %d/%d, Buffer %d/%d\r\n", 
           g_http_snooping_ctx.con_max, g_http_snooping_ctx.con_num,
           g_http_snooping_ctx.buf_num, g_http_snooping_ctx.buf_max);
    for(i = 0; i < g_http_snooping_ctx.url_max; i++) {
        url_node = &g_http_snooping_ctx.url_base[i];
        if ((url_node->url_flags & HTTP_SNOOPING_FLG_RES) == 0) {
            continue;
        }

        local_file_str.len = HTTP_SP_LFILE_LEN_MAX + HTTP_LOCAL_FILE_ROOT_MAX;
        if (url_node->redirect_addr == 0) {
            memcpy(&local_file_buf[0],http_cache_file_root.data,http_cache_file_root.len);
            local_file_str.data = &local_file_buf[http_cache_file_root.len];
        } else {
            n = sprintf((char *)&local_file_buf[0], "%a:8080/", url_node->redirect_addr);
            local_file_str.data = &local_file_buf[n];
        }
        url_str.len = url_node->server_name.len + url_node->res_url.len;
        url_str.data = url_node->url_buf;
        ngx_http_snooping_url2local_file(&url_str, &local_file_str, url_node->pcache_rule);
        if (url_node->mime_type->postfix.len != 0) {
            ngx_memcpy(&local_file_str.data[local_file_str.len], 
                       url_node->mime_type->postfix.data,
                       url_node->mime_type->postfix.len);
            local_file_str.len += url_node->mime_type->postfix.len;
            local_file_str.data[local_file_str.len] = '\0';
        }
        printf("URL %d:%s \r\n", i, url_node->url_buf);
        printf("    ===> LOCAL(0x%X-0x%X) %s, len:%d, encodeing:%s\r\n",
               url_node->url_flags, url_node->pcache_rule->cache_flg,
               local_file_buf, url_node->url_res_len, 
               http_encoding_type_array[url_node->encoding_type]->data);
        printf("    Url Key %d %s\r\n", url_node->file_key_name.len, url_node->key_buf);
    }
    return;
}

void http_host_hash_show(struct_command_data_block *pcdb)
{
    ngx_int_t i;

    printf("Now time %d\r\n", jiffies/(60*HZ));
    for(i = 0; i < HTTP_MAX_HOST; i++) {
        if (http_homepage_hash[i] != 0) {
            printf("Host %a mask 0.0.255.255 minite %d\r\n", i, (jiffies - http_homepage_hash[i])/(60*HZ));
        }
    }
    return;
}

EOLWOS_K(exec_show_http_host_hash_eol, http_host_hash_show);
EOLWOS_K(exec_show_http_url_cache_eol, http_url_cache_show);
KEYWORD(exec_show_http_host_hash, exec_show_http_host_hash_eol, no_alt, \
             "host-hash","Host Hash", PRIVILEGE_CONFIG);
KEYWORD(exec_show_http_url_cache, exec_show_http_url_cache_eol, exec_show_http_host_hash, \
    "url-cache","URL cache", PRIVILEGE_CONFIG);
KEYWORD(exec_show_http, exec_show_http_url_cache, no_alt, \
    "http","HTTP", PRIVILEGE_CONFIG);
APPEND_POINT(exec_show_http_app, exec_show_http);

void http_url_cache_add(http_snooping_url_t *url_node, ngx_uint_t tick);
void http_url_cache_data_add(http_snooping_url_t *url_node, ngx_uint_t tick);
int http_host_cmp(void *node, GENERIC_ARGUMENT key)
{
    int rc;
    ngx_str_t *host_str = (ngx_str_t *)key.p;
    http_cache_host_url_t *phost_url = (http_cache_host_url_t *)node;
    if (phost_url == NULL) {
        return 0;
    }

    rc = ngx_strcmp(phost_url->host.data, host_str->data);
    return rc;  
}

int http_snooping_mime_cmp(void *node, GENERIC_ARGUMENT key)
{
    int rc;
    ngx_str_t *pmime_key = (ngx_str_t *)key.p;
    http_mime_type_t *pmime_type = (http_mime_type_t *)node;
    ngx_str_t *pmime_node = &pmime_type->mime_type;

    if (pmime_type == NULL) {
        return 0;
    }

    rc = ngx_strcmp(pmime_node, pmime_key);
    return rc;
}

int http_snooping_cache_file_cmp(void *node, GENERIC_ARGUMENT key)
{
    int rc;
    ngx_str_t *pcache_file_key = (ngx_str_t *)key.p;
    http_cache_file_avlnode_t *pcache_file_node = (http_cache_file_avlnode_t *)node;
    ngx_str_t *pcache_file_node_str = pcache_file_node->cache_str;

    if (pcache_file_node == NULL) {
        return 0;
    }

    rc = ngx_strncmp(pcache_file_node_str->data,
                     pcache_file_key->data, 
                     pcache_file_key->len);
    return rc;
}

void ngx_http_ef_getn(ef_buf_chain_t *efb_chain_dst, ef_buf_chain_t *efb_chain_src, ngx_uint_t token)
{
    ngx_uint_t i;
    ef_buf_t *efb;
    if (efb_chain_src->count > token) {
        efb = efb_chain_src->head;
        for (i = 0; i < token; i++) {
            ef_buf_efb_to_chain(efb_chain_dst, efb);
            efb = efb->nextPkt;
        }

        efb_chain_src->head = efb->nextPkt;
        efb_chain_src->count -= token;
        efb->nextPkt = NULL;
    } else {
        ef_buf_copy_chain(efb_chain_dst, efb_chain_src);
        ef_buf_init_chain(efb_chain_src);
    }

    return;
}

http_snooping_url_t *ngx_http_snooping_url_hash_find(ngx_str_t *url_name)
{
    ngx_uint_t url_key;
    http_snooping_url_t *sp_url_find = NULL;
    http_snooping_url_t *sp_url_tmp;
    struct list_head *phash_node;
    url_key = ngx_hash_key(url_name->data, url_name->len);
    url_key %= HTTP_SP_URL_MAX;
    HS_HASH_DATA_SPIN_LOCK(url_key);
    phash_node = &g_http_snooping_ctx.url_hash[url_key];
    list_for_each_entry(sp_url_tmp, phash_node, url_hash_list, http_snooping_url_t) {
        if (url_name->len != sp_url_tmp->file_key_name.len)
            continue ;
 
        if (ngx_strncmp(&sp_url_tmp->file_key_name.data[0], url_name->data, url_name->len) == 0) {
            sp_url_find = sp_url_tmp;
            break;
        }
    }
    HS_HASH_DATA_SPIN_UNLOCK(url_key);
    return sp_url_find;
}



void ngx_http_snooping_url_hash_add(http_snooping_url_t *sp_url)
{
    ngx_uint_t url_key;
    struct list_head *phash_node;

    url_key = ngx_hash_key(&sp_url->file_key_name.data[0],
                           sp_url->file_key_name.len);
    url_key %= HTTP_SP_URL_MAX;
    HS_HASH_DATA_SPIN_LOCK(url_key);
    phash_node = &g_http_snooping_ctx.url_hash[url_key];
    list_add_tail(&sp_url->url_hash_list, phash_node);
    sp_url->url_flags |= HTTP_SNOOPING_FLG_URL_HASH;
    HS_HASH_DATA_SPIN_UNLOCK(url_key);
    return;
}

void ngx_http_snooping_url_hash_del(http_snooping_url_t *sp_url)
{
    ngx_uint_t url_key;
    url_key = ngx_hash_key(&sp_url->file_key_name.data[0],
                           sp_url->file_key_name.len);
    url_key %= HTTP_SP_URL_MAX;
    HS_HASH_DATA_SPIN_LOCK(url_key);
    list_del(&sp_url->url_hash_list);
    sp_url->url_flags &= ~HTTP_SNOOPING_FLG_URL_HASH;
    HS_HASH_DATA_SPIN_UNLOCK(url_key);
    return;
}

http_cache_url_t *ngx_http_url_file_cache(ngx_str_t *host, ngx_str_t *url)
{
    ngx_int_t i, j;
    ngx_str_t head_xiegan_str;
    ngx_str_t mid_xiegan_str;
    ngx_str_t tail_xiegan_str;
    ngx_str_t para_str;
    ngx_str_t post_str;
    http_cache_host_url_t *cache_host;
    http_cache_url_t *url_cache;
    /*http_cache_file_avlnode_t *pcache_avlnode;
    GENERIC_ARGUMENT key;*/
    for (i = 0; i < http_cache_host_url_num; i++) {
       cache_host = &http_cache_host_url[i];
       if (cache_host->host.len == host->len 
           && (ngx_strncmp(cache_host->host.data, host->data, cache_host->host.len) == 0)) {
           break;
       }
    }

    if (i == http_cache_host_url_num) {
        cache_host = &http_cache_default_host_url;
    }

    head_xiegan_str.data = &url->data[0];
    mid_xiegan_str.data = &url->data[0];
    tail_xiegan_str.data = &url->data[0];
    para_str.data = &url->data[url->len];
    para_str.len = 0;
    post_str.data = &url->data[url->len - 1];
    j = 0;
    for(i = 0; i < url->len; i++) {
        if (url->data[i] == '/') {
            if (j == 0) {
                head_xiegan_str.data = &url->data[i];
                mid_xiegan_str.data = &url->data[i];
                tail_xiegan_str.data = &url->data[i];
                j = 1;
            } else if (j == 1) {
                mid_xiegan_str.data = &url->data[i];
                tail_xiegan_str.data = &url->data[i];
                j = 2;
            } else {
                tail_xiegan_str.data = &url->data[i];
            }
        } else if (url->data[i] == '.') {
            post_str.data = &url->data[i];
        } else if (url->data[i] == '?') {
            para_str.data = &url->data[i];
            para_str.len = url->len - i;
            break;
        }
    }

    head_xiegan_str.len = mid_xiegan_str.data - head_xiegan_str.data;
    mid_xiegan_str.len = tail_xiegan_str.data - mid_xiegan_str.data;
    tail_xiegan_str.len = para_str.data - tail_xiegan_str.data;
    post_str.len = para_str.data - post_str.data;
    for (i = 0; i < cache_host->url_num; i++) {
        url_cache = &cache_host->url_cache[i];
        if (url_cache->cache_flg & HTTP_CACHE_URL_HEAD_XIEGAN) {
            if (url_cache->head_xiegan.len != head_xiegan_str.len) {
                continue;
            }
            if (ngx_strncmp(url_cache->head_xiegan.data, head_xiegan_str.data, url_cache->head_xiegan.len) != 0) {
                continue;
            }
        }

        if (url_cache->cache_flg & HTTP_CACHE_URL_MID_XIEGAN) {
            if (url_cache->mid_xiegan.len != mid_xiegan_str.len) {
                continue;
            }
            if (ngx_strncmp(url_cache->mid_xiegan.data, mid_xiegan_str.data, url_cache->mid_xiegan.len) != 0) {
                continue;
            }
        }

        if (url_cache->cache_flg & HTTP_CACHE_URL_TAIL_XIEGAN) {
            if (url_cache->tail_xiegan.len != tail_xiegan_str.len) {
                continue;
            }
            if (ngx_strncmp(url_cache->tail_xiegan.data, tail_xiegan_str.data, url_cache->tail_xiegan.len) != 0) {
                continue;
            }
        }

        if (url_cache->cache_flg & HTTP_CACHE_URL_POST_FILE) {
            if (url_cache->post_file.len != post_str.len) {
                continue;
            }
            if (ngx_strncmp(url_cache->post_file.data, post_str.data, url_cache->post_file.len) != 0) {
                continue;
            }
        }

        if (url_cache->cache_flg & HTTP_CACHE_URL_PARA) {
            if (url_cache->para.len != para_str.len) {
                continue;
            }
            if (ngx_strncmp(url_cache->para.data, para_str.data, url_cache->para.len) != 0) {
                continue;
            }
        }
        break;
    }

    if (i == cache_host->url_num) {
        return NULL;
    }

    return url_cache;
}

http_mime_type_t *ngx_http_url_mime_type(ngx_str_t *content_type)
{
    uchar conten_buf_tmp;
    http_mime_type_t *pmime;
    GENERIC_ARGUMENT key;
    conten_buf_tmp = content_type->data[content_type->len];
    content_type->data[content_type->len]  = '\0';
    key.p = content_type;

    pmime = avlSearch(http_mime_tree, key, http_snooping_mime_cmp);
    if (pmime == NULL) {
        pmime = &http_mime_null;
    }

    content_type->data[content_type->len]  = conten_buf_tmp;
    return pmime;
}

ngx_int_t ngx_http_sp_urlid_get(ngx_str_t *url_name)
{
    unsigned long flags;
    http_cache_url_t *cache_rule;
    http_snooping_url_t *url_node = NULL;
    ngx_int_t url_id = -1;
    ngx_str_t url_key;
    ngx_int_t err_step = 0;
    uchar url_tmp_buf[HTTP_SP_URL_LEN_MAX];

    if ((url_name == NULL) || (url_name->len > (HTTP_SP_URL_LEN_MAX -1))) {
        err_step = 1;
        goto exit_label;
    }

    cache_rule = ngx_http_url_rule_get(url_name);
    if (cache_rule == NULL) {
        err_step = 2;
        goto exit_label;
    }

    url_key.data = &url_tmp_buf[0];
    url_key.len = url_name->len + 1;
    url_key.data[0] = '/';
    ngx_memcpy(&url_key.data[1], url_name->data, url_name->len);
    url_tmp_buf[url_key.len] = '\0';

    ngx_http_snooping_url2local_key(&url_key, cache_rule);
    if (url_key.len >= HTTP_SP_LFILE_LEN_MAX) {
        err_step = 3;
        goto exit_label;
    }

    _local_irqsave(flags);
    url_node = ngx_http_snooping_url_hash_find(&url_key);
    _local_irqrestore(flags);
exit_label:
    if (url_node != NULL) {
        url_id = NGX_HTTP_URLNODE2ID(url_node);
    }
    return url_id;
}

void ngx_http_sp_url_state_setby_id(ngx_int_t url_id, u8 state)
{
    ngx_int_t err_step = 0;
    http_snooping_url_t *url_node = NULL;
    if (!NGX_HTTP_URLID_VALID(url_id)) {
        err_step = 1;
        goto exit_label;    
    }

    url_node = NGX_HTTP_URLID2NODE(url_id);
    if ((url_node->url_flags & HTTP_SNOOPING_FLG_RES) == 0) {
        err_step = 2;
        goto exit_label;        
    }

    switch (state) {
        case HTTP_LOCAL_DOWN_STATE_OK:
            url_node->local_down_state = HTTP_LOCAL_DOWN_STATE_OK;
            url_node->url_flags |= HTTP_SNOOPING_FLG_NGX_DOWN;
            url_node->url_action = HTTP_C2SP_ACTION_ADD;
            http_url_cache_add(url_node, 0);
            break;
        case HTTP_LOCAL_DOWN_STATE_ERROR:
            url_node->local_down_state = HTTP_LOCAL_DOWN_STATE_INIT;
            http_url_cache_add(url_node, 11);
            break;
        default:
            url_node->local_down_state = HTTP_LOCAL_DOWN_STATE_INIT;
            break;
    }
exit_label:
    return;
}

void ngx_http_sp_url_state_setby_name(ngx_str_t *url_name, u8 state)
{
    ngx_int_t url_id;
    url_id = ngx_http_sp_urlid_get(url_name);
    if (NGX_HTTP_URLID_VALID(url_id)) {
        ngx_http_sp_url_state_setby_id(url_id, state);
    }

    return;
}

ngx_int_t ngx_http_sp_id2fpath(ngx_int_t url_id, ngx_str_t *pFileBuf)
{
    ngx_int_t err_step = 0;
    http_snooping_url_t *url_node = NULL;
    if (!NGX_HTTP_URLID_VALID(url_id)) {
        err_step = 1;
        goto exit_label;        
    }
    url_node = NGX_HTTP_URLID2NODE(url_id);
    if ((url_node->url_flags & HTTP_SNOOPING_FLG_RES) == 0) {
        err_step = 2;
        goto exit_label;        
    }

    if ((pFileBuf == NULL) || (pFileBuf->len < HTTP_SP_URL_LEN_MAX)){
        err_step = 3;
        goto exit_label;        
    }
    ngx_str_t url_str;
    url_str.data = &url_node->url_buf[0];
    url_str.len = strlen(url_str.data);
    ngx_http_snooping_url2local_file(&url_str, pFileBuf, url_node->pcache_rule);
exit_label:
    return err_step;
}


ngx_int_t ngx_http_sp_url_handle(ngx_str_t *url_name, u8 action)
{
    unsigned long flags;
    struct list_head *plist;
    http_cache_url_t *cache_rule;
    http_snooping_url_t *url_node = NULL;
    ngx_int_t url_id = -1;
    ngx_str_t url_key;
    ngx_int_t i, err_step = 0;
    uchar url_tmp_buf[HTTP_SP_URL_LEN_MAX];

    if (url_name->len > (HTTP_SP_URL_LEN_MAX -1)) {
        err_step = 1;
        goto exit_label;
    }

    cache_rule = ngx_http_url_rule_get(url_name);
    if ((cache_rule == NULL) 
        || ((cache_rule->cache_flg & HTTP_CACHE_URL_ACTION_CACHE_UPSTREAM) == 0)) {
        err_step = 2;
        goto exit_label;
    }

    url_key.data = &url_tmp_buf[0];
    url_key.len = url_name->len + 1;
    url_key.data[0] = '/';
    ngx_memcpy(&url_key.data[1], url_name->data, url_name->len);
    url_tmp_buf[url_key.len] = '\0';

    ngx_http_snooping_url2local_key(&url_key, cache_rule);
    if (url_key.len >= HTTP_SP_LFILE_LEN_MAX) {
        err_step = 3;
        goto exit_label;
    }

    _local_irqsave(flags);
    url_node = ngx_http_snooping_url_hash_find(&url_key);
    _local_irqrestore(flags);
    switch (action) {
        case HTTP_C2SP_ACTION_ADD:
            if (url_node == NULL) {
                HS_URL_SPIN_LOCK(flags);
                if (g_http_snooping_ctx.url_free > 0) {
                    plist = g_http_snooping_ctx.url_free_list.next;
                    list_del(plist);
                    url_node = list_entry(plist, http_snooping_url_t, res_list);
                    list_add(&url_node->res_list, &g_http_snooping_ctx.url_use_list);
                    g_http_snooping_ctx.url_free--;
                    url_node->url_flags = 0;
                    ngx_memcpy(&url_node->key_buf[0], url_key.data, url_key.len);
                    url_node->key_buf[url_key.len] = '\0';
                    url_node->file_key_name.data = &url_node->key_buf[0];
                    url_node->file_key_name.len = url_key.len;
                    url_node->pcache_rule = cache_rule;
                    url_node->redirect_addr = ngx_server_addr;
                    url_node->encoding_type = HTTP_ECODING_TYPE_NONE;
                    ngx_http_snooping_url_hash_add(url_node);
                    url_node->url_data_lock = 0;
                    url_node->write_error_num = 0;
                    url_node->url_flags |= HTTP_SNOOPING_FLG_RES;
                    url_node->mime_type = &http_mime_null;
                }
                HS_URL_SPIN_UNLOCK(flags);

                if (url_node != NULL) {
                    url_node->pkt_send_tick = jiffies;
                    INIT_LIST_HEAD(&url_node->data_buf_list);
                    ngx_memcpy(&url_node->url_buf[0], url_name->data, url_name->len);
                    url_node->url_buf[url_name->len] = '\0';
                    url_node->server_name.data = &url_node->url_buf[0];
                    for (i = 0; i < url_name->len; i++) {
                        if (url_node->url_buf[i] == '/') {
                            break;
                        }
                    }
                    url_node->server_name.len = i;
                    url_node->res_url.data = &url_node->url_buf[url_node->server_name.len];
                    url_node->res_url.len = url_name->len - url_node->server_name.len; 
                    url_node->url_flags |= HTTP_SNOOPING_FLG_RES_LFB;
                    url_node->url_access_num = 1;
                    url_id = NGX_HTTP_URLNODE2ID(url_node);
                }
            }else {
                url_node->url_access_num++;
                url_id = NGX_HTTP_URLNODE2ID(url_node);
            }
            break;
        case HTTP_C2SP_ACTION_DELETE:
            if (url_node != NULL) {
/*
                HS_URL_SPIN_LOCK(flags);
                if (url_node->url_flags & HTTP_SNOOPING_FLG_URL_HASH) {
                    ngx_http_snooping_url_hash_del(url_node);
                }

                HS_URL_CACHE_DATA_SPIN_LOCK();
                if (url_node->url_flags & HTTP_SNOOPING_FLG_CACHE_LIST) {
                    list_del(&url_node->cache_buf_list);
                    g_http_snooping_ctx.cache_num--;
                }
                HS_URL_CACHE_DATA_SPIN_UNLOCK();
                list_del(&url_node->res_list);
                list_add(&url_node->res_list, &g_http_snooping_ctx.url_free_list);
                url_node->url_flags = 0;
                g_http_snooping_ctx.url_free++;
                HS_URL_SPIN_UNLOCK(flags);
*/
                url_node->url_action = HTTP_C2SP_ACTION_DELETE;
                http_url_cache_add(url_node, 0);
                url_id = NGX_HTTP_URLNODE2ID(url_node);
            }
            break;
        default:
            break;
    }
exit_label:
    return url_id;
}

void ngx_http_youku_url_add(ngx_str_t *url_name)
{
    http_snooping_url_t *sp_url = NULL;
    ngx_uint_t i, flags;
    struct list_head *plist;
    ngx_uint_t err_step = 0;
    //tcp_proxy_cb_t *tp_entry;
    uchar url_tmp_buf[HTTP_SP_URL_LEN_MAX + HTTP_LOCAL_FILE_ROOT_MAX];
    ngx_str_t url_key;
    http_cache_url_t *url_cache;
    url_key.data = &url_tmp_buf[0];
    url_key.len = url_name->len + 1;

    if (url_key.len > (HTTP_SP_URL_LEN_MAX -1)) {
        err_step = 1;
        goto exit_label;
    }

    url_cache = ngx_http_url_rule_get(url_name);
    if (url_cache == NULL) {
        err_step = 2;
        goto exit_label;
    }

    url_key.data[0] = '/';
    ngx_memcpy(&url_key.data[1], url_name->data, url_name->len);
    url_tmp_buf[url_key.len] = '\0';
    ngx_http_snooping_url2local_key(&url_key, url_cache);
    if (url_key.len > HTTP_SP_LFILE_LEN_MAX) {
        err_step = 3;
        goto exit_label;
    }

    _local_irqsave(flags);
    sp_url = ngx_http_snooping_url_hash_find(&url_key);
    if (sp_url == NULL) {
        HS_URL_SPIN_LOCK(flags);
        if (g_http_snooping_ctx.url_free > 0) {
            plist = g_http_snooping_ctx.url_free_list.next;
            list_del(plist);
            sp_url = list_entry(plist, http_snooping_url_t, res_list);
            list_add(&sp_url->res_list, &g_http_snooping_ctx.url_use_list);
            g_http_snooping_ctx.url_free--;
            ngx_memcpy(&sp_url->key_buf[0], url_key.data, url_key.len);
            sp_url->key_buf[url_key.len] = '\0';
            sp_url->file_key_name.data = &sp_url->key_buf[0];
            sp_url->file_key_name.len = url_key.len;
            sp_url->pcache_rule = url_cache;
            sp_url->encoding_type = HTTP_ECODING_TYPE_NONE;
            sp_url->url_flags = HTTP_SNOOPING_FLG_RES;
            ngx_http_snooping_url_hash_add(sp_url);
            sp_url->url_data_lock = 0;
            sp_url->write_error_num = 0;
            sp_url->mime_type = &http_mime_null;
        }
        HS_URL_SPIN_UNLOCK(flags);
    }
    _local_irqrestore(flags);

    if (sp_url != NULL) {
        sp_url->pkt_send_tick = jiffies;
        INIT_LIST_HEAD(&sp_url->data_buf_list);
        ngx_memcpy(&sp_url->url_buf[0], url_name->data, url_name->len);
        sp_url->url_buf[url_name->len] = '\0';
        sp_url->server_name.data = &sp_url->url_buf[0];
        for (i = 0; i < url_name->len; i++) {
            if (sp_url->url_buf[i] == '/') {
                break;
            }
        }
        sp_url->server_name.len = i;
        sp_url->res_url.data = &sp_url->url_buf[sp_url->server_name.len];
        sp_url->res_url.len = url_name->len - sp_url->server_name.len; 
        sp_url->url_flags |= HTTP_SNOOPING_FLG_NGX_DOWN | HTTP_SNOOPING_FLG_RES_LFB;
        sp_url->url_access_num ++;
/*
        HS_URL_CACHE_DATA_SPIN_LOCK();
        list_add(&sp_url->cache_buf_list, &g_http_snooping_ctx.url_cache_list); 
        sp_url->url_flags |= HTTP_SNOOPING_FLG_CACHE_LIST;
        g_http_snooping_ctx.cache_num++;
        HS_URL_CACHE_DATA_SPIN_UNLOCK();
*/
        http_url_cache_add(sp_url, 1);
    }
exit_label:
    return ;
}

void ngx_http_ark_letv_com_para_key(ngx_str_t *para)
{
    /*/getvinfo?otype=json&vid=v00149uf4ir&callback=a
      &defn=auto&charge=0&g_tk=&qq=&qqlog=&appver=2.8.0.2812
      &sysver=ios7.0.4&device=iPad&lang=zh_CN&platform=10103&newplatform=10103
      &appVer=2.8.0&encryptVer=1.0
      &cKey=54D73C94D03F0CF36B89B8CC6ECDEBC48871ADC1282AA0A370FB49895A4B2C42&drm=0*/
    ngx_uint_t i, j;
    for (i = 0; i <  para->len; i++) {
        if (para->data[i] == 'a'
           && para->data[i + 1] == 'r'
           && para->data[i + 2] == 'k'
           && para->data[i + 3] == '=') {
           break;
        }
    }

    if (i == para->len) {
        para->len = 0;
        return;
    }

    para->data = &para->data[i-1];
    para->data[0] = '_';
    para->len = para->len - i + 1;
    j = 0;
    for (i = 0; i < para->len; i++) {
        if (para->data[i] == '&') {
            j++;
        }

        if (j == 1) {
            break;
        }
    }
    para->len = i;
    return;
}


void ngx_http_qq_com_para_key(ngx_str_t *para)
{
    /*/getvinfo?otype=json&vid=v00149uf4ir&callback=a
      &defn=auto&charge=0&g_tk=&qq=&qqlog=&appver=2.8.0.2812
      &sysver=ios7.0.4&device=iPad&lang=zh_CN&platform=10103&newplatform=10103
      &appVer=2.8.0&encryptVer=1.0
      &cKey=54D73C94D03F0CF36B89B8CC6ECDEBC48871ADC1282AA0A370FB49895A4B2C42&drm=0*/
    ngx_uint_t i, j;
    for (i = 0; i <  para->len; i++) {
        if (para->data[i] == 'v'
           && para->data[i + 1] == 'i'
           && para->data[i + 2] == 'd'
           && para->data[i + 3] == '=') {
           break;
        }
    }

    if (i == para->len) {
        para->len = 0;
        return;
    }

    para->data = &para->data[i-1];
    para->data[0] = '_';
    para->len = para->len - i + 1;
    j = 0;
    for (i = 0; i < para->len; i++) {
        if (para->data[i] == '&') {
            j++;
        }

        if (j == 1) {
            break;
        }
    }
    para->len = i;
    return;
}


void ngx_http_sohu_m3u8_file_key(ngx_str_t *para)
{
    /*/ipad66760987_4507713588805_98873198.m3u8*/
    ngx_uint_t i, j = 0, k = 0;
    for (i = 0; i < para->len; i++) {
        if (para->data[i] == '_'){
           j++;
        }

        if (j != 1) {
            para->data[k++] = para->data[i];
        }
    }

    para->len = k;
    return;
}


void ngx_http_sohu_para_key(ngx_str_t *para)
{
    /*ipad?file=/208/214/7G7WIe0YEdqgt6bRkEBTd2.mp4*/
    ngx_uint_t i, j;
    for (i = 0; i <  para->len; i++) {
        if (para->data[i + 0] == 'f'
           && para->data[i + 1] == 'i'
           && para->data[i + 2] == 'l'
           && para->data[i + 3] == 'e') {
           break;
        }
    }

    if (i == para->len) {
        para->len = 0;
        return;
    }

    para->data = &para->data[i-1];
    para->data[0] = '_';
    para->len = para->len - i + 1;
    j = 0;
    for (i = 0; i < para->len; i++) {
        if (para->data[i] == '&') {
            j++;
        } else if (para->data[i] == '/') {
            para->data[i] = '_';
        }

        if (j == 1) {
            break;
        }
    }

    para->len = i;
    para->data[para->len] = '\0';
    return;
}

void ngx_http_youku_andriod_para_key(ngx_str_t *para)
{
    /*/common/v3/play?_t_=1396340174&e=md5&_s_=5b3b2ebe74f73f32ea48187bc48b8cef
      &point=1&id=XNTM4MzE4OTQ0&local_time=&local_vid=&format=1,5,6,7,8
      &language=guoyu&did=97b053c9004566f3b310ce8bb81e3a38&ctype=20
      &local_point=&audiolang=1&pid=7a4f3a9290c8d4dd&guid=222f9e2c33038dc12eea75d57479e204
      &mac=00:90:A2:56:71:B0&imei=&ver=3.7.2&network=WIFI*/
    ngx_uint_t i, j;
    for (i = 0; i <  para->len; i++) {
        if (para->data[i] == 'i'
           && para->data[i + 1] == 'd'
           && para->data[i + 2] == '=') {
           break;
        }
    }

    if (i == para->len) {
        para->len = 0;
        return;
    }

    para->data = &para->data[i-1];
    para->data[0] = '_';
    para->len = para->len - i + 1;
    j = 0;
    for (i = 0; i < para->len; i++) {
        if (para->data[i] == '&') {
            j++;
        }

        if (j == 1) {
            break;
        }
    }
    para->len = i;
    return;
}

void ngx_http_pl_youku_para_key(ngx_str_t *para)
{
    /*?&vid=XMjk3NjEyNDE2&type=flv*/
    ngx_uint_t i, j;
    for (i = 0; i <  para->len; i++) {
        if (para->data[i + 0] == 'v'
           && para->data[i + 1] == 'i'
           && para->data[i + 2] == 'd'
           && para->data[i + 3] == '=') {
           break;
        }
    }

    if (i == para->len) {
        para->len = 0;
        return;
    }

    para->data = &para->data[i-1];
    para->data[0] = '_';
    para->len = para->len - i + 1;
    j = 0;
    for (i = 0; i < para->len; i++) {
        if (para->data[i] == '&') {
            j++;
        }

        if (j == 2) {
            break;
        }
    }
    para->len = i;
    return;
}

void ngx_http_snooping_url2local_key(ngx_str_t *url_name, http_cache_url_t *url_cache)
{
    ngx_uint_t i, j;
    ngx_int_t xiegan_num, last_xiegan = 0;
    ngx_str_t para = {0, NULL};
    ngx_str_t file = {0, NULL};
    /*处理url*/
    xiegan_num = -1;
    j = 0;
    for (i = 0; i < url_name->len; i++) {
        if (url_name->data[i] == '/') {
            xiegan_num++;
            last_xiegan = i;
        } else if (url_name->data[i] == '?') {
            file.len = i - last_xiegan;
            para.data = &url_name->data[i];
            para.len = url_name->len - i;
            break;
        }

        if(url_cache->key_flg & (1 << xiegan_num)) {
            url_name->data[j++] = url_name->data[i];
        }
    }

    file.data = &url_name->data[last_xiegan];
    if (file.len == 0) {
        file.len = url_name->len - last_xiegan;
    }
    url_name->len = j;

    if (file.len 
        && (HTTP_CACHE_KEY_FILE & url_cache->key_flg)) {
        if (url_cache->url_file_key_parse) {
            url_cache->url_file_key_parse(&file);
        }

        if (file.len) {
            ngx_memcpy(&url_name->data[url_name->len], file.data, file.len);
            url_name->len += file.len;
        }
    }

    if (para.len 
        && (HTTP_CACHE_KEY_PARA & url_cache->key_flg)) {
        if (url_cache->url_para_key_parse) {
            url_cache->url_para_key_parse(&para);
        }

        if (para.len) {
            ngx_memcpy(&url_name->data[url_name->len], para.data, para.len);
            url_name->len += para.len;
        }
    }

    url_name->data[url_name->len]= '\0';
    return ;
}

void ngx_http_snooping_url2local_file(ngx_str_t *url_str, ngx_str_t *local_file, http_cache_url_t *pcache_rule)
{
    ngx_uint_t i;
    ngx_str_t para = {0, NULL};
    uchar url_para[HTTP_SP_URL_LEN_MAX];
    //local_file->len = url_str->len;
    for(i = 0; i < url_str->len; i++) {
        if (url_str->data[i] == '/') {
            break;
        } else if (url_str->data[i] == '.') {
            local_file->data[i] = '_';
        } else {
            local_file->data[i] = url_str->data[i];
        }
    }

    if (i == (url_str->len - 1)) {
        local_file->len = i;
    } else {
        local_file->data[i] = url_str->data[i];
        i++;
        for(; i < url_str->len; i++) {
            if (url_str->data[i] == '/') {
                local_file->data[i] = '_';
            } else if (url_str->data[i] == '?') {
                para.data = &url_str->data[i];
                para.len = url_str->len - i;
                break;
            } else {
                local_file->data[i] = url_str->data[i];
            }
        }
        local_file->len = i;
    }

    if (para.len 
        && (pcache_rule->key_flg & HTTP_CACHE_KEY_PARA)
        && pcache_rule->url_para_key_parse) {
        for(i = 0; i < para.len; i++) {
           url_para[i] = para.data[i]; 
        }
        url_para[i] = '\0';
        para.data = &url_para[0];
        pcache_rule->url_para_key_parse(&para);
        if (para.len &&  ((local_file->len + para.len) < HTTP_SP_LFILE_LEN_MAX)) {
            ngx_memcpy(&local_file->data[local_file->len], para.data, para.len);
            local_file->len += para.len;
        }
    }
    local_file->data[local_file->len] = '\0';
    return;
}

http_snooping_url_t *ngx_http_snooping_url_record(http_sp_connection_t *con)
{
    http_snooping_url_t *sp_url = NULL;
    struct list_head *plist;
    //tcp_proxy_cb_t *tp_entry;
    uchar url_tmp_buf[HTTP_SP_URL_LEN_MAX];
    ngx_str_t url_name;
    //ngx_str_t *pcache_str;
    http_cache_url_t *url_cache;

    url_name.len = con->server_name.len + con->res_url.len + 1;
    if (url_name.len > (HTTP_SP_URL_LEN_MAX -1)) {
        goto exit_label;
    }

    url_tmp_buf[0] = '/';
    ngx_memcpy(&url_tmp_buf[1], con->server_name.data, con->server_name.len);
    ngx_memcpy(&url_tmp_buf[con->server_name.len + 1], con->res_url.data, con->res_url.len);
    url_tmp_buf[url_name.len] = '\0';
    url_name.data = &url_tmp_buf[1];
    url_name.len -= 1;
    //printk("%s-%d %d %s\r\n", __FILE__, __LINE__, url_name.len, url_name.data);
    //url_cache = ngx_http_url_file_cache(&con->server_name, &con->res_url);
    url_cache = ngx_http_url_rule_get(&url_name);
    if (url_cache == NULL) {
        goto exit_label;
    }

/*
    if (url_cache->cache_flg & HTTP_CACHE_URL_FORBIDEN) {
        ngx_http_snooping_url_forbidden(con);
        goto exit_label;
    }
*/
    url_name.data = &url_tmp_buf[0];
    url_name.len += 1;
    ngx_http_snooping_url2local_key(&url_name, url_cache);
    if (url_name.len > HTTP_SP_LFILE_LEN_MAX) {
        goto exit_label;
    }
    sp_url = ngx_http_snooping_url_hash_find(&url_name);
    if (url_cache->cache_flg & HTTP_CACHE_URL_ACTION_CACHE_SP) {
        if (sp_url == NULL) {
            HS_URL_DATA_SPIN_LOCK();
            if (g_http_snooping_ctx.url_free > 0) {
                plist = g_http_snooping_ctx.url_free_list.next;
                list_del(plist);
                sp_url = list_entry(plist, http_snooping_url_t, res_list);
                list_add(&sp_url->res_list, &g_http_snooping_ctx.url_use_list);
                g_http_snooping_ctx.url_free--;
                ngx_memcpy(&sp_url->key_buf[0], url_name.data, url_name.len);
                sp_url->key_buf[url_name.len] = '\0';
                sp_url->file_key_name.data = &sp_url->key_buf[0];
                sp_url->file_key_name.len = url_name.len;
                sp_url->redirect_addr = 0;
                sp_url->write_error_num = 0;
                sp_url->url_flags = HTTP_SNOOPING_FLG_RES;
                ngx_http_snooping_url_hash_add(sp_url);
                sp_url->pcache_rule = url_cache;
                sp_url->pkt_send_tick = jiffies;
                sp_url->url_data_lock = 0;
            }
            HS_URL_DATA_SPIN_UNLOCK();

            if (sp_url != NULL) {
                INIT_LIST_HEAD(&sp_url->data_buf_list);
                sp_url->server_name.data = &sp_url->url_buf[0];
                sp_url->server_name.len = con->server_name.len;
                ngx_memcpy(&sp_url->url_buf[0], con->server_name.data, con->server_name.len);
                sp_url->res_url.data = &sp_url->url_buf[sp_url->server_name.len];
                sp_url->res_url.len = con->res_url.len;
                ngx_memcpy(sp_url->res_url.data, con->res_url.data, con->res_url.len);
                sp_url->url_buf[sp_url->server_name.len + con->res_url.len] = '\0';
                sp_url->url_access_num = 0;
                //sp_url->local_file_name.len = 0; 
                //sp_url->local_file_path.len = 0;
                sp_url->encoding_type = HTTP_ECODING_TYPE_NONE;
/*
                HS_URL_CACHE_DATA_SPIN_LOCK();
                list_add(&sp_url->cache_buf_list, &g_http_snooping_ctx.url_cache_list);
                sp_url->url_flags |= HTTP_SNOOPING_FLG_CACHE_LIST; 
                g_http_snooping_ctx.cache_num++;
                HS_URL_CACHE_DATA_SPIN_UNLOCK();
*/
                sp_url->url_action = HTTP_C2SP_ACTION_ADD;
                http_url_cache_data_add(sp_url, 0);
                if (url_cache->tail_xiegan.len == 1) {
                    con->post_fix = 1;
                } else {
                    con->post_fix = 0;
                }
                sp_url->mime_type = &http_mime_null;
            }
        } 
    }

    if (sp_url != NULL) {
        sp_url->url_access_num ++;
    }
exit_label:
    con->con_url = sp_url;
    return sp_url;
}

void ngx_http_snooping_cache_buf_alloc(ngx_uint_t data_len, http_snooping_url_t *file_url)
{
    ngx_uint_t buf_num, i;
    http_snooping_buf_t *cache_buf;
    buf_num = (data_len + HTTP_SP_BUF_LEN_MAX - 1)/HTTP_SP_BUF_LEN_MAX;
    INIT_LIST_HEAD(&file_url->data_buf_list);
    HS_BUF_DATA_SPIN_LOCK();
    if (g_http_snooping_ctx.buf_num >= buf_num) {
        for (i = 0; i < buf_num; i++) {
            cache_buf = (http_snooping_buf_t *)g_http_snooping_ctx.data_buf_free_list.next;
            list_del(&cache_buf->res_list);
            list_add_tail(&cache_buf->res_list, &file_url->data_buf_list);
            cache_buf->buf_len = 0;
        }
        file_url->url_data_current = (http_snooping_buf_t *)file_url->data_buf_list.next;
        g_http_snooping_ctx.buf_num -= buf_num;
    }
    HS_BUF_DATA_SPIN_UNLOCK();

    return ;
}

void ngx_http_snooping_cache_buf(http_sp_connection_t *con, ef_buf_chain_t *efb_chain)
{
    ngx_int_t cpy_byte, cpy_start, err_step = 0;
    ef_buf_t *efb_snd;
    tcp_app_data_t http_data;
    http_snooping_url_t *file_url;
    //http_snooping_buf_t *pcache_buf;
    file_url = con->con_url;

    if (con->trans_encoding_type) {
        err_step = 1;
        goto exit_label;
    }

    if (file_url->url_res_len <= 0) {
        err_step = 3;
        goto exit_label;
    }

    if (efb_chain->count == 0) {
        err_step = 4;
        goto exit_label;
    }

    if (file_url->data_start == 0) {
        file_url->data_start = 1;
        if (file_url->pcache_rule->cache_flg & HTTP_CACHE_URL_SP_WRITE) {
            ngx_http_snooping_cache_buf_alloc(con->data_len, file_url);
            if (!list_empty(&file_url->data_buf_list)) {
                file_url->url_flags |= HTTP_SNOOPING_FLG_CACHE_BUF_ALLOC;
            }
        }
    }
/*
    if ((file_url->url_flags & HTTP_SNOOPING_FLG_CACHE_BUF_ALLOC) == 0) {
        err_step = 5;
        goto exit_label;
    }
*/
    efb_snd = efb_chain->head;
    if (file_url->url_flags & HTTP_SNOOPING_FLG_CACHE_BUF_ALLOC) {
        while(efb_snd) {
            tcp_data_get(&http_data, efb_snd);
            if (file_url->url_data_len <= con->response_header_len) {
                if (file_url->url_data_len + http_data.buf_len > con->response_header_len) {
                    cpy_start = con->response_header_len - file_url->url_data_len;
                    cpy_byte = http_data.buf_len - cpy_start;
                } else {
                    cpy_byte = 0;
                }
            } else {
                cpy_start = 0;
                cpy_byte = http_data.buf_len;
            }

            while(cpy_byte) {
                if (cpy_byte > (HTTP_SP_BUF_LEN_MAX - file_url->url_data_current->buf_len)) {
                    ngx_memcpy(&file_url->url_data_current->data[file_url->url_data_current->buf_len],
                               &http_data.buf[cpy_start], 
                               HTTP_SP_BUF_LEN_MAX - file_url->url_data_current->buf_len);
                    cpy_byte -= HTTP_SP_BUF_LEN_MAX - file_url->url_data_current->buf_len;
                    cpy_start += HTTP_SP_BUF_LEN_MAX - file_url->url_data_current->buf_len;
                    file_url->url_data_current->buf_len = HTTP_SP_BUF_LEN_MAX;
                    file_url->url_data_current = (http_snooping_buf_t *)file_url->url_data_current->res_list.next;
                } else {
                    ngx_memcpy(&file_url->url_data_current->data[file_url->url_data_current->buf_len],
                               &http_data.buf[cpy_start], 
                               cpy_byte);
                    file_url->url_data_current->buf_len += cpy_byte;
                    if (file_url->url_data_current->buf_len == HTTP_SP_BUF_LEN_MAX) {
                        file_url->url_data_current = (http_snooping_buf_t *)file_url->url_data_current->res_list.next;
                    }
                    cpy_byte -= cpy_byte;
                }
            }
            file_url->url_data_len += http_data.buf_len;
            efb_snd = efb_snd->nextPkt;
        }
    } else {
        while(efb_snd) {
            tcp_data_get(&http_data, efb_snd);
            file_url->url_data_len += http_data.buf_len;

            efb_snd = efb_snd->nextPkt;
        }
    }

    if (file_url->url_data_len >= (con->data_len + con->response_header_len)) {
        file_url->url_data_len -= con->response_header_len;
        con->res_state = HTTP_SP_STATE_DONE;    
    }

exit_label:

    return ;
}

void ngx_http_snooping_url_forbidden(http_sp_connection_t *con)
{
    tcp_app_data_t http_data;
    ef_buf_t *efb_snd;
    int token;
    tcp_proxy_cb_t *tp_cb;

    tp_cb = (tcp_proxy_cb_t *)con->wan_ta_client_tp;
    token = tp_cb->tp_socket->write_queue_free_space(tp_cb->sk);
    if (token == 0) {
        goto exit_label;
    }

    efb_snd= r_ef_buf_alloc();
    if (efb_snd == NULL) {
        return;
    }

    wan_ta_init_nondata_efb(efb_snd, 1, 0x10/*TCPHDR_ACK*/);
    if (efb_snd == NULL){
        goto exit_label;
    }

    tcp_data_get(&http_data, efb_snd);
    memcpy(http_data.buf, http_client_403_str.data, http_client_403_str.len);
    efb_snd->pktlen = efb_snd->pktlen + http_client_403_str.len - http_data.buf_len;
    efb_snd->nextPkt = NULL;
    tp_cb->tp_socket->sendmsg(tp_cb->sk, efb_snd);
exit_label:
    ef_buf_free_chain(con->request_pkt_chain.head, 
                      con->request_pkt_chain.tail, 
                      con->request_pkt_chain.count);
    ef_buf_init_chain(&con->request_pkt_chain);
    return ;
}

void ngx_http_snooping_url_redirect(http_sp_connection_t *con)
{
    http_snooping_url_t *sp_url;
    tcp_app_data_t http_data;
    ef_buf_t *efb_snd;
    int token;
    tcp_proxy_cb_t *tp_cb;
    uchar *pbuf;
    ngx_int_t n, i;
    ngx_uint_t homepage_hash_key;
    ngx_uint_t server_ip;
    uchar *con_str;
    sp_url = con->con_url;
    http_cache_url_t *pcache_rule = sp_url->pcache_rule;

    homepage_hash_key = HTTP_HOMEPAGE_HASH_KEY(con->client_addr);
    if (pcache_rule->cache_flg & HTTP_CACHE_URL_HOMEPAGE_POLL) {
        if (jiffies < (http_homepage_hash[homepage_hash_key] + HTTP_HOMEPAGE_TIMEOUT)
            && http_homepage_hash[homepage_hash_key] != 0) {
            return;
        }
    }

    tp_cb = (tcp_proxy_cb_t *)con->wan_ta_client_tp;
    token = tp_cb->tp_socket->write_queue_free_space(tp_cb->sk);
    if (token == 0) {
        goto exit_label;
    }

    efb_snd= r_ef_buf_alloc();
    if (efb_snd == NULL) {
        return;
    }

    wan_ta_init_nondata_efb(efb_snd, 1, 0x10/*TCPHDR_ACK*/);
    if (efb_snd == NULL){
        goto exit_label;
    }

    if (pcache_rule 
       && (HTTP_CACHE_URL_CON_CLOSE & pcache_rule->cache_flg)) {
       con_str = http_con_str[1].data;
    } else {
       con_str = http_con_str[0].data;
    }

    tcp_data_get(&http_data, efb_snd);
    pbuf = (uchar *)http_data.buf;
    ngx_memcpy(pbuf, http_redirect_302_str.data, http_redirect_302_str.len);
    pbuf += http_redirect_302_str.len;

    if (sp_url->redirect_addr != 0) {
        server_ip = sp_url->redirect_addr;
    } else {
        server_ip = con->local_server_addr;
    }

    if (pcache_rule->cache_flg & HTTP_CACHE_URL_HOMEPAGE_POLL) {
        n = sprintf(pbuf, "%sLocation: http://%a:8080/", con_str, server_ip);
        http_homepage_hash[homepage_hash_key] = jiffies;
    } else {
        ngx_str_t url_str, local_file_str;
        uchar local_file_buf[HTTP_SP_URL_LEN_MAX];

        url_str.len = sp_url->server_name.len + sp_url->res_url.len;
        url_str.data = sp_url->url_buf;
        local_file_str.len = HTTP_SP_URL_LEN_MAX;
        local_file_str.data = &local_file_buf[0];
        ngx_http_snooping_url2local_file(&url_str, &local_file_str, sp_url->pcache_rule);
        if (sp_url->mime_type->postfix.len != 0) {
            ngx_memcpy(&local_file_str.data[local_file_str.len], 
                       sp_url->mime_type->postfix.data,
                       sp_url->mime_type->postfix.len);
            local_file_str.len += sp_url->mime_type->postfix.len;
            local_file_str.data[local_file_str.len] = '\0';
        }
        n = sprintf(pbuf, "%sLocation: http://%a:8080/%s", con_str, server_ip, local_file_str.data);
        if (sp_url->pcache_rule->cache_flg & HTTP_CACHE_URL_REDIRECT_PARA) {
            for (i = 0; i < con->res_url.len; i++) {
                if (con->res_url.data[i] == '?')
                    break;
            }

            for(; i < con->res_url.len; i++) {
                pbuf[n++] = con->res_url.data[i];
            }
        }
    }

    pbuf[n++] = '\r';
    pbuf[n++] = '\n';
    pbuf[n++] = '\r';
    pbuf[n++] = '\n';
    efb_snd->pktlen = efb_snd->pktlen + http_redirect_302_str.len - http_data.buf_len + n;
    efb_snd->nextPkt = NULL;
    tp_cb->tp_socket->sendmsg(tp_cb->sk, efb_snd);
exit_label:
    ef_buf_free_chain(con->request_pkt_chain.head, 
                      con->request_pkt_chain.tail, 
                      con->request_pkt_chain.count);
    ef_buf_init_chain(&con->request_pkt_chain);
    return ;
}

void ngx_http_snooping_request_parse(tcp_proxy_cb_t *tp_entry, ef_buf_chain_t *efb_chain, ngx_uint_t token)
{
    ngx_uint_t i,pkt_num, http_states_parse, error_step = 0;
    ef_buf_t *efb_ins_head;
    tcp_app_data_t http_data;
    uchar *req_pos, *req_tmp;
    http_snooping_url_t *sp_url;
    uchar data_str[12];

    http_sp_connection_t *con = (http_sp_connection_t *)tp_entry->wan_ta_app_cb;

    efb_ins_head = efb_chain->head;
    pkt_num = efb_chain->count;
    con->req_rec_pkt += efb_chain->count;
    ef_buf_chain2chain(&con->request_pkt_chain, efb_chain);
    ef_buf_init_chain(efb_chain);

    if (con->request_pkt_chain.count == 0 || token == 0) {
        return ;
    }

    http_states_parse = 1;
    while (http_states_parse) {
        switch (con->req_state) {
            case HTTP_SP_STATE_INIT:
            case HTTP_SP_STATE_INIT_DATA:
                if (con->req_state == HTTP_SP_STATE_INIT) {
                    con->res_state = HTTP_SP_STATE_INIT;
                    con->parse_res_pos = 0;
                    con->rec_res_data_len = 0;
                    con->parse_req_pos = 0;
                    con->req_data_len = 0;
                }

                for (i = 0; i < pkt_num; i++) {
                    req_pos = &con->current_request_header_buf[con->parse_req_pos];
                    tcp_data_get(&http_data, efb_ins_head);
                    if (http_data.buf_len + con->parse_req_pos > (HTTP_SP_REQ_HEAD_LEN_MAX - 1)) {
                        http_data.buf_len = HTTP_SP_REQ_HEAD_LEN_MAX - 1 - con->parse_req_pos;
                        ngx_memcpy(req_pos, 
                                   http_data.buf, http_data.buf_len);
                        con->parse_req_pos += http_data.buf_len;
                        break;
                    } 

                    ngx_memcpy(req_pos, http_data.buf, http_data.buf_len);
                    con->parse_req_pos += http_data.buf_len;
                    efb_ins_head = efb_ins_head->nextPkt;
                }

                con->current_request_header_buf[con->parse_req_pos] = '\0';
                req_pos = &con->current_request_header_buf[0];
                req_tmp = ngx_strstr(req_pos, http_end.data);
                if (req_tmp == NULL) {
                    if (con->parse_req_pos > (HTTP_SP_REQ_HEAD_LEN_MAX - 1)) {
                        error_step = 1;
                        con->req_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                    } else {
                        http_states_parse = 0;
                        con->req_state = HTTP_SP_STATE_INIT_DATA;
                    }
                } else {
                    con->req_state = HTTP_SP_STATE_REQUEST_HEADER;
                    con->request_header_len = req_tmp + http_end.len - req_pos;
                }
                break;

            case HTTP_SP_STATE_REQUEST_HEADER:
                req_pos = &con->current_request_header_buf[0];
                if (ngx_str4cmp(req_pos, 'G', 'E', 'T', ' ')) {
                    con->req_method = HTTP_SP_METHOD_GET;
                    req_pos += 4;
                } else if (ngx_str5cmp(req_pos, 'P', 'O', 'S', 'T',' ')) {
                    con->req_method = HTTP_SP_METHOD_POST;
                    req_pos += 5;
                } else if (ngx_str4cmp(req_pos, 'P', 'U', 'T', ' ')) {
                    con->req_method = HTTP_SP_METHOD_PUT;
                    req_pos += 4;
                } else if (ngx_str5cmp(req_pos, 'H', 'E', 'A', 'D',' ')){
                    con->req_method = HTTP_SP_METHOD_HEAD;
                    req_pos += 5;
                } else {
                    error_step = 2;
                    con->req_method = HTTP_SP_METHOD_NONE;
                    con->req_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                    goto end_step2;
                }

                con->res_url.data = req_pos;
                req_tmp = ngx_strchr(req_pos, ' ');
                if (req_tmp == NULL) {
                    error_step = 3;
                    con->req_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                    goto end_step2;
                }

                con->res_url.len = req_tmp - con->res_url.data;
                if (con->res_url.len >= HTTP_SP_URL_LEN_MAX) {
                    error_step = 4;
                    con->req_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                    goto end_step2;
                }

                req_pos = req_tmp + 1;
                req_tmp = ngx_strstr(req_pos, http_host.data);
                if (req_tmp == NULL) {
                    error_step = 5;
                    con->req_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                    goto end_step2;
                } 

                req_pos = req_tmp + http_host.len;
                con->server_name.data = req_pos;
                req_tmp = ngx_strstr(req_pos, http_new_line.data);
                if (req_tmp == NULL) {
                    error_step = 6;
                    con->req_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                    goto end_step2;
                } 

                con->server_name.len = req_tmp - con->server_name.data;
                req_pos = req_tmp + http_new_line.len;
                if (con->req_method == HTTP_SP_METHOD_GET) {
                    sp_url = ngx_http_snooping_url_record(con);
                    if (sp_url != NULL) {
                        if (sp_url->pcache_rule->cache_flg & HTTP_CACHE_URL_FORBIDEN){
                            ngx_http_snooping_url_forbidden(con);
                        } else if ((sp_url->url_flags & HTTP_SNOOPING_FLG_RES_WRITE) 
                            || sp_url->pcache_rule->cache_flg & HTTP_CACHE_URL_HOMEPAGE_POLL) {
                            ngx_http_snooping_url_redirect(con);
                        }  
                    }
                    con->req_state = HTTP_SP_STATE_DONE;
                } else {
                    req_tmp = ngx_strstr(req_pos, http_content_len.data);
                    if (req_tmp == NULL) {
                        con->req_data_len = 0;
                        con->req_state = HTTP_SP_STATE_DONE;
                    } else {
                        req_tmp += http_content_len.len;
                        for (i = 0; i < 11; i++) {
                            if (req_tmp[i] == '\r') {
                                break;
                            } else {
                                data_str[i] = req_tmp[i];
                            }
                        }

                        if (i == 11) {
                            con->req_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                            error_step = 7;
                            goto end_step2;
                        }

                        data_str[i] = '\0';
                        con->req_data_len = ngx_atoi(data_str, i);
                        if (con->req_data_len == -1) {
                            con->req_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                            error_step = 8;
                            goto end_step2;
                        }
                        con->req_state = HTTP_SP_STATE_DATA_LINE;
                    }
                }
    end_step2:
            break;
            case HTTP_SP_STATE_DONE:
                ngx_http_ef_getn(efb_chain, &con->request_pkt_chain, token);
                if (con->request_pkt_chain.count) {
                    tp_event_notify(tp_entry->sk, TP_EVENT_APP_DATA);
                    //tp_entry->event_bits |= TP_EVENT_APP_DATA;
                }
                http_states_parse = 0;
                break;
            case HTTP_SP_STATE_DATA_LINE:
                ngx_http_ef_getn(efb_chain, &con->request_pkt_chain, token);
                if (con->request_pkt_chain.count) {
                    tp_event_notify(tp_entry->sk, TP_EVENT_APP_DATA);
                    //tp_entry->event_bits |= TP_EVENT_APP_DATA;
                }

                //con->req_state = HTTP_SP_STATE_DATA_LINE;
                http_states_parse = 0;
                break;
            case HTTP_SP_STATE_REQUEST_HEADER_ERROR:
                ngx_http_ef_getn(efb_chain, &con->request_pkt_chain, token);
                if (con->request_pkt_chain.count) {
                    tp_event_notify(tp_entry->sk, TP_EVENT_APP_DATA);
                    //tp_entry->event_bits |= TP_EVENT_APP_DATA;
                }

                http_states_parse = 0;
                con->parse_req_pos = 0;
                con->con_url = NULL;
                break;
            default:
                http_states_parse = 0;
                break;
        }
    }
    con->req_snd_pkt += efb_chain->count;
    return;
}

int ngx_http_get_request_pkt_chain(tcp_proxy_cb_t *tp_entry)
{
    http_sp_connection_t *con = (http_sp_connection_t *)tp_entry->wan_ta_app_cb;
    return con->request_pkt_chain.count;
}

int ngx_http_get_response_pkt_chain(tcp_proxy_cb_t *tp_entry)
{
    http_sp_connection_t *con = (http_sp_connection_t *)tp_entry->wan_ta_app_cb;
    return con->response_pkt_chain.count;
}

void ngx_http_get_con_pkt(tcp_proxy_cb_t *tp_entry, u32 *pBuf)
{
    http_sp_connection_t *con = (http_sp_connection_t *)tp_entry->wan_ta_app_cb;
    pBuf[0] = con->req_rec_pkt;
    pBuf[1] = con->req_snd_pkt;
    pBuf[2] = con->res_rec_pkt;
    pBuf[3] = con->res_snd_pkt;
}

void ngx_http_snooping_response_parse(tcp_proxy_cb_t *tp_entry, ef_buf_chain_t *efb_chain, int token)
{
    uchar *req_pos, *req_tmp;
    tcp_app_data_t http_data;
    ngx_uint_t i,pkt_num, error_step = 0, http_states_parse;
    ef_buf_t *efb_ins_head;
    //ef_buf_chain_t efb_chain_ret;
    //http_snooping_buf_t *pcache_buf;
    http_sp_connection_t *con = (http_sp_connection_t *)tp_entry->wan_ta_app_cb;
    efb_ins_head = efb_chain->head;
    pkt_num = efb_chain->count;
    con->res_rec_pkt += pkt_num;
    ef_buf_chain2chain(&con->response_pkt_chain, efb_chain);
    ef_buf_init_chain(efb_chain);

    if (con->response_pkt_chain.count == 0 || token == 0) {
        return ;
    }

    if (con->con_url == NULL
        || con->req_state == HTTP_SP_STATE_REQUEST_HEADER_ERROR) {
        ngx_http_ef_getn(efb_chain, &con->response_pkt_chain, token);
        con->res_snd_pkt += efb_chain->count;
        if (con->response_pkt_chain.count) {
            tp_event_notify(tp_entry->sk, TP_EVENT_APP_DATA);
            //tp_entry->event_bits |= TP_EVENT_APP_DATA;
        }
        con->req_state = HTTP_SP_STATE_INIT;
        con->parse_req_pos = 0;
        return;
    }

    http_states_parse = 1;
    while (http_states_parse) {
        switch (con->res_state) {
            case HTTP_SP_STATE_INIT:
            case HTTP_SP_STATE_INIT_DATA:
                if (con->res_state == HTTP_SP_STATE_INIT) {
                    con->parse_res_pos = 0;
                    con->parse_req_pos = 0;
                    con->rec_req_data_len = 0;
                    con->req_data_len = 0;
                    con->req_state = HTTP_SP_STATE_INIT;
                }

                for (i = 0; i < pkt_num; i++) {
                    req_pos = &con->current_response_header_buf[con->parse_res_pos];
                    tcp_data_get(&http_data, efb_ins_head);

                    if ((http_data.buf_len + con->parse_res_pos) > (HTTP_SP_RES_HEAD_LEN_MAX - 1)) {
                        http_data.buf_len = (HTTP_SP_RES_HEAD_LEN_MAX - 1) - con->parse_res_pos;
                        ngx_memcpy(req_pos, http_data.buf, http_data.buf_len);
                        con->parse_res_pos += http_data.buf_len;
                        break;
                    }

                    ngx_memcpy(req_pos, http_data.buf, http_data.buf_len);
                    con->parse_res_pos += http_data.buf_len;
                    efb_ins_head = efb_ins_head->nextPkt;
                }

                con->current_response_header_buf[con->parse_res_pos] = '\0';
                req_pos = &con->current_response_header_buf[0];
                req_tmp = (u_char *) ngx_strstr(req_pos, http_end.data);
                if (req_tmp == NULL) {
                    if (con->parse_res_pos >= (HTTP_SP_RES_HEAD_LEN_MAX - 1)) {
                        error_step = 1;
                        con->res_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                    } else {
                        http_states_parse = 0;
                        con->res_state = HTTP_SP_STATE_INIT_DATA;
                    }
                } else {
                    con->res_state =  HTTP_SP_STATE_REQUEST_HEADER;
                    con->response_header_len = req_tmp + http_end.len - req_pos;
                }

                break;
            case HTTP_SP_STATE_REQUEST_HEADER:
                req_pos = &con->current_response_header_buf[0];
                //解析status
                req_tmp = ngx_strchr(req_pos, ' ');
                if (req_tmp == NULL) {
                    con->res_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                    error_step = 2;
                    goto end_step2;
                }

                con->response_status.data = req_tmp + 1;
                req_pos = con->response_status.data;
                req_tmp = ngx_strchr(req_pos, ' ');
                if (req_tmp == NULL) {
                    con->res_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                    error_step = 3;
                    goto end_step2;
                }

                con->response_status.len = req_tmp - req_pos;
                if (!ngx_str3_cmp(con->response_status.data, '2','0','0',' ')) {
                    con->res_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                    error_step = 4;
                    goto end_step2;
                }

                req_pos = &con->current_response_header_buf[0];
                req_tmp = ngx_strstr(req_pos, http_content_type.data);
                if (req_tmp == NULL) {
                    con->res_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                    error_step = 5;
                    goto end_step2;
                }

                con->content_type.data = req_tmp + http_content_type.len;
                req_pos = con->content_type.data;
                req_tmp = ngx_strstr(req_pos, http_new_line.data);
                if (req_tmp == NULL) {
                    con->res_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                    error_step = 6;
                    goto end_step2;
                }
                con->content_type.len = req_tmp - req_pos;
                for (i = 1; i < con->content_type.len; i++) {
                    if (con->content_type.data[i] == ';') {
                        con->content_type.len = i - 1;
                        break;
                    }
                }

                if (con->post_fix) {
                    con->con_url->mime_type = ngx_http_url_mime_type(&con->content_type);
                }

                req_pos = &con->current_response_header_buf[0];
                req_tmp = ngx_strstr(req_pos, http_trans_encoding_type_chunked.data);
                if (req_tmp == NULL) {
                    con->trans_encoding_type = 0;
                    req_tmp = ngx_strstr(req_pos, http_content_len.data);
                    if (req_tmp == NULL) {
                        con->res_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                        error_step = 8;
                        goto end_step2;   
                    }

                    con->content_len.data = req_tmp + http_content_len.len;
                    req_pos = con->content_len.data;
                    req_tmp = ngx_strstr(req_pos, http_new_line.data);
                    if (req_tmp == NULL) {
                        con->res_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                        error_step = 9;
                        goto end_step2;
                    }
                    con->content_len.len = req_tmp - req_pos;
                    if (con->content_len.len > 20) {
                        con->res_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                        error_step = 10;
                        goto end_step2;
                    }

                    con->data_len = ngx_atoi(con->content_len.data, con->content_len.len);
                    if (con->data_len == -1) {
                        con->data_len = 0;
                        con->res_state = HTTP_SP_STATE_REQUEST_HEADER_ERROR;
                        error_step = 11;
                        goto end_step2;
                    }

                    con->con_url->url_res_len = con->data_len;
                } else {
                    con->trans_encoding_type = 1;
                }

                req_pos = &con->current_response_header_buf[0];
                req_tmp = ngx_strstr(req_pos, http_content_encoding_type.data);
                if (req_tmp == NULL) {
                    con->encoding_type = HTTP_ECODING_TYPE_NONE;
                } else {
                    req_pos = req_tmp + http_content_encoding_type.len;
                    if (ngx_str4cmp(req_pos, 'g', 'z', 'i', 'p')) {
                        con->encoding_type = HTTP_ECODING_TYPE_GZIP;
                    } else {
                        con->encoding_type = HTTP_ECODING_TYPE_DEFLATE;
                    }
                }
                con->con_url->data_start = 0;
                con->con_url->encoding_type = con->encoding_type;
                con->res_state = HTTP_SP_STATE_DATA_LINE;
                con->con_url->url_data_len = 0;
                con->con_url->url_flags |= HTTP_SNOOPING_FLG_RES_LFB;
    end_step2:
                break;
            case HTTP_SP_STATE_DONE:
                con->con_url->url_flags |= HTTP_SNOOPING_FLG_RES_CACHE;
                http_states_parse = 0;
                break;
            case HTTP_SP_STATE_DATA_LINE:
                ngx_http_ef_getn(efb_chain, &con->response_pkt_chain, token);
                ngx_http_snooping_cache_buf(con, efb_chain);
                if (con->response_pkt_chain.count) {
                    tp_event_notify(tp_entry->sk, TP_EVENT_APP_DATA);
                    //tp_entry->event_bits |= TP_EVENT_APP_DATA;
                }
                if (HTTP_SP_STATE_DATA_LINE == con->res_state) {
                    http_states_parse = 0;
                }
                break;
            case HTTP_SP_STATE_REQUEST_HEADER_ERROR:
                ngx_http_ef_getn(efb_chain, &con->response_pkt_chain, token);
                if (con->response_pkt_chain.count) {
                     tp_event_notify(tp_entry->sk, TP_EVENT_APP_DATA);
                    //tp_entry->event_bits |= TP_EVENT_APP_DATA;
                }
                con->parse_res_pos = 0;
                http_states_parse = 0;
                break;
            default:
                ngx_http_ef_getn(efb_chain, &con->response_pkt_chain, token);
                if (con->response_pkt_chain.count) {
                    tp_event_notify(tp_entry->sk, TP_EVENT_APP_DATA);
                    //tp_entry->event_bits |= TP_EVENT_APP_DATA;
                }
                http_states_parse = 0;
                break;
        }
    }
    con->res_snd_pkt += efb_chain->count;

    http_cache_url_t *pcache_rule = con->con_url->pcache_rule;
    if (pcache_rule 
       && (HTTP_CACHE_URL_CON_CLOSE & pcache_rule->cache_flg)
       && efb_chain->count
       && (con->con_url->url_flags & HTTP_SNOOPING_FLG_RES_WRITE)) {
        ef_buf_free_chain(efb_chain->head, 
                          efb_chain->tail, 
                          efb_chain->count);
        ef_buf_init_chain(efb_chain);
    }

    return;
}

void http_url_cache_add(http_snooping_url_t *url_node, ngx_uint_t tick)
{
    unsigned long flags;
    HS_URL_CACHE_SPIN_LOCK(flags);
    if ((url_node->url_flags & HTTP_SNOOPING_FLG_CACHE_LIST) == 0) {
        list_add_tail(&url_node->cache_buf_list, &g_http_snooping_ctx.url_cache_list[(ngx_write_tick+tick)%HTTP_CACHE_TIME_OUT]);
        url_node->url_flags |= HTTP_SNOOPING_FLG_CACHE_LIST;
        g_http_snooping_ctx.cache_num++;
    }
    HS_URL_CACHE_SPIN_UNLOCK(flags);
    return ;
}

void http_url_cache_data_add(http_snooping_url_t *url_node, ngx_uint_t tick)
{
    HS_URL_CACHE_DATA_SPIN_LOCK();
    if ((url_node->url_flags & HTTP_SNOOPING_FLG_CACHE_LIST) == 0) {
        list_add_tail(&url_node->cache_buf_list, &g_http_snooping_ctx.url_cache_list[(ngx_write_tick+tick)%HTTP_CACHE_TIME_OUT]);
        url_node->url_flags |= HTTP_SNOOPING_FLG_CACHE_LIST;
        g_http_snooping_ctx.cache_num++;
    }
    HS_URL_CACHE_DATA_SPIN_UNLOCK();
    return ;
}

http_snooping_url_t *http_url_cache_get()
{
    unsigned long flags;
    http_snooping_url_t *url_node;
    struct list_head *plist, *cache_list;

    HS_URL_CACHE_SPIN_LOCK(flags);
    cache_list = &g_http_snooping_ctx.url_cache_list[ngx_write_tick%HTTP_CACHE_TIME_OUT];
    if (list_empty(cache_list)) {
        url_node = NULL;
    } else {
        plist = cache_list->next;
        list_del_init(plist);
        url_node = list_entry(plist, http_snooping_url_t, cache_buf_list);
        url_node->url_flags &= ~HTTP_SNOOPING_FLG_CACHE_LIST;
        g_http_snooping_ctx.cache_num--;
    }
    HS_URL_CACHE_SPIN_UNLOCK(flags);
    return url_node;
}

http_snooping_buf_t *http_url_cache_buf_get(http_snooping_url_t *url_node)
{
    http_snooping_buf_t *pbuf;
    //unsigned long flags;
    struct list_head *plist, *pbuf_list;

    pbuf_list = &url_node->data_buf_list;
    if (list_empty(pbuf_list)) {
        pbuf = NULL;
    } else {
        plist = pbuf_list->next;
        list_del(plist);
        pbuf = list_entry(plist, http_snooping_buf_t, res_list);
    }

    return pbuf;
}

void http_cache_buf_free(http_snooping_buf_t *pbuf)
{

    unsigned long flags;
    HS_BUF_SPIN_LOCK(flags);
    list_add(&pbuf->res_list, &g_http_snooping_ctx.data_buf_free_list);
    g_http_snooping_ctx.buf_num++;
    HS_BUF_SPIN_UNLOCK(flags);
    return ;
}

http_snooping_buf_t *http_cache_buf_data_get()
{
    struct list_head *plist;
    http_snooping_buf_t *pbuf;
    HS_BUF_DATA_SPIN_LOCK();
    if (g_http_snooping_ctx.buf_num == 0) {
        pbuf = NULL;
    } else {
        plist = g_http_snooping_ctx.data_buf_free_list.next;
        pbuf = list_entry(plist, http_snooping_buf_t, res_list);
        g_http_snooping_ctx.buf_num--;
    }
    HS_BUF_DATA_SPIN_UNLOCK();
    return pbuf;
}


uchar m_http_deflate_buf[24*1024];
ngx_int_t ngx_http_buf_write(ngx_int_t fd, uchar *pwrite, ngx_uint_t len)
{
    ngx_int_t rc;
    ssize_t  n, written, size;
    written = 0;
    size = len;
    for ( ;; ) {
        n = write(fd, pwrite + written, size);
        if (n == -1) {
            rc = -1;
            goto exit_label;
        }

        written += n;
        if ((size_t) n == size) {
            break;
        }

        size -= n;
    }
exit_label:
    return rc;
}

void nxg_http_gzip_write(http_snooping_url_t *url_node)
{
    ngx_int_t fd, err_step = 0;
    http_snooping_buf_t *pbuf;
    ngx_str_t url_str, local_file_str;
    uchar local_file_buf[HTTP_SP_URL_LEN_MAX + HTTP_LOCAL_FILE_ROOT_MAX];

    url_str.len = url_node->server_name.len + url_node->res_url.len;
    url_str.data = url_node->url_buf;
    local_file_str.len = HTTP_SP_URL_LEN_MAX + HTTP_LOCAL_FILE_ROOT_MAX;
    memcpy(&local_file_buf[0],http_cache_file_root.data,http_cache_file_root.len);
    local_file_str.data = &local_file_buf[http_cache_file_root.len];
    ngx_http_snooping_url2local_file(&url_str, &local_file_str, url_node->pcache_rule);
    if (url_node->mime_type->postfix.len != 0) {
        ngx_memcpy(&local_file_str.data[local_file_str.len], 
                   url_node->mime_type->postfix.data,
                   url_node->mime_type->postfix.len);
        local_file_str.len += url_node->mime_type->postfix.len;
        local_file_str.data[local_file_str.len] = '\0';
    }

    strcat(local_file_buf, ".gz");
    fd = ngx_open_file(&local_file_buf[0], 
                       NGX_FILE_WRONLY, 
                       NGX_FILE_CREATE_OR_OPEN,
                       NGX_FILE_DEFAULT_ACCESS);
    if (fd == NGX_INVALID_FILE) {
        err_step = 1;
        goto exit_label;
    }
    url_node->url_flags |= HTTP_SNOOPING_FLG_RES_LFN;

    pbuf = http_url_cache_buf_get(url_node);
    while (pbuf != NULL) {
        ngx_http_buf_write(fd, pbuf->data, pbuf->buf_len);
        http_cache_buf_free(pbuf);
        pbuf = http_url_cache_buf_get(url_node);
    }
    close(fd);
    printf("Uncompress File %s Start\r\n", local_file_buf);
    file_uncompress(&local_file_buf[0]);
    url_node->url_flags |= HTTP_SNOOPING_FLG_RES_WRITE;
    printf("Save URL %s to Local File %s (File length %d) OK\r\n",
            url_node->url_buf, local_file_buf, url_node->url_res_len);
exit_label:
    if (err_step != 0) {
        pbuf = http_url_cache_buf_get(url_node);
        while (pbuf != NULL) {
            http_cache_buf_free(pbuf);
            pbuf = http_url_cache_buf_get(url_node);
        }
        url_node->url_flags &= ~(HTTP_SNOOPING_FLG_RES_CACHE | HTTP_SNOOPING_FLG_CACHE_BUF_ALLOC);
    }
    return;
}

void ngx_http_deflate_write(http_snooping_url_t *url_node)
{
    z_stream d_stream;
    ngx_int_t fd, err_step = 0;
    ngx_str_t url_str, local_file_str;
    uchar local_file_buf[HTTP_SP_URL_LEN_MAX + HTTP_LOCAL_FILE_ROOT_MAX];
    http_snooping_buf_t *pbuf;
    ngx_int_t code;
    ngx_uint_t bytesWritten;
    static char dummy_head[2] =
    {
        0x8 + 0x7 * 0x10,
        (((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
    };

    url_str.len = url_node->server_name.len + url_node->res_url.len;
    url_str.data = url_node->url_buf;
    local_file_str.len = HTTP_SP_URL_LEN_MAX + HTTP_LOCAL_FILE_ROOT_MAX;
    memcpy(&local_file_buf[0],http_cache_file_root.data,http_cache_file_root.len);
    local_file_str.data = &local_file_buf[http_cache_file_root.len];
    ngx_http_snooping_url2local_file(&url_str, &local_file_str, url_node->pcache_rule);
    if (url_node->mime_type->postfix.len != 0) {
        ngx_memcpy(&local_file_str.data[local_file_str.len], 
                   url_node->mime_type->postfix.data,
                   url_node->mime_type->postfix.len);
        local_file_str.len += url_node->mime_type->postfix.len;
        local_file_str.data[local_file_str.len] = '\0';
    }

    fd = ngx_open_file(&local_file_buf[0], 
                       NGX_FILE_WRONLY, 
                       NGX_FILE_CREATE_OR_OPEN,
                       NGX_FILE_DEFAULT_ACCESS);
    if (fd == NGX_INVALID_FILE) {
        err_step = 1;
        goto exit_label;
    }

    printf("Deflate File %s Start\r\n", local_file_buf);
    url_node->url_flags |= HTTP_SNOOPING_FLG_RES_LFN;
    ngx_memset(&d_stream, 0, sizeof (d_stream));
    if (inflateInit(&d_stream) != Z_OK) {
        err_step = 2;
        goto exit_label;
    }

    pbuf = http_url_cache_buf_get(url_node);
    if ((pbuf->data[0] != dummy_head[0]) 
        && (pbuf->data[1] != dummy_head[1])) {
        // some servers (notably Apache with mod_deflate) don't generate zlib headers
        // insert a dummy header and try again
        d_stream.next_in = (Bytef*) dummy_head;
        d_stream.avail_in = 2;
        d_stream.next_out = m_http_deflate_buf;
        d_stream.avail_out = (uInt)24*1024;
        code = inflate(&d_stream, Z_NO_FLUSH);
        if (code != Z_OK) {
            err_step = 3;
            goto exit_label;
        }
    }

    while (pbuf != NULL) {
        d_stream.next_in = pbuf->data;
        d_stream.avail_in = pbuf->buf_len;
        do {
            d_stream.next_out = m_http_deflate_buf;
            d_stream.avail_out = (uInt)24*1024;
            code = inflate(&d_stream, Z_NO_FLUSH);
            bytesWritten = (uInt)24*1024 - d_stream.avail_out;
            if (code == Z_STREAM_END) {
                if (bytesWritten) {
                    //写文件
                    ngx_http_buf_write(fd, m_http_deflate_buf, bytesWritten);
                }
                
                inflateEnd(&d_stream);
                break;
            } else if (code == Z_OK) {
                if (bytesWritten) {
                    //写文件
                    ngx_http_buf_write(fd, m_http_deflate_buf, bytesWritten);
                }
            } else if (code == Z_BUF_ERROR) {
                if (bytesWritten) {
                    //写文件
                    ngx_http_buf_write(fd, m_http_deflate_buf, bytesWritten);
                }
            } else {
                err_step = 4;
                goto exit_label;
            }
        }while(d_stream.avail_out == 0);

        http_cache_buf_free(pbuf);
        pbuf = http_url_cache_buf_get(url_node);
    }
    close(fd);
    url_node->url_flags |= HTTP_SNOOPING_FLG_RES_WRITE;
    printf("Save URL %s to Local File %s (File length %d) OK\r\n",
            url_node->url_buf, local_file_buf, url_node->url_res_len);
exit_label:

    if (err_step != 0) {
        if (err_step > 1) {
            close(fd);
            inflateEnd(&d_stream);
        }

        pbuf = http_url_cache_buf_get(url_node);
        while (pbuf != NULL) {
            http_cache_buf_free(pbuf);
            pbuf = http_url_cache_buf_get(url_node);
        }
        url_node->url_flags &= ~(HTTP_SNOOPING_FLG_RES_CACHE | HTTP_SNOOPING_FLG_CACHE_BUF_ALLOC);
    }

    return;
}

void ngx_http_none_write(http_snooping_url_t *url_node)
{
    ngx_int_t fd, err_step = 0;
    http_snooping_buf_t *pbuf;
    ngx_str_t url_str, local_file_str;
    uchar local_file_buf[HTTP_SP_URL_LEN_MAX + HTTP_LOCAL_FILE_ROOT_MAX];

    //ssize_t  n, written, size, fd;
    url_str.len = url_node->server_name.len + url_node->res_url.len;
    url_str.data = url_node->url_buf;
    local_file_str.len = HTTP_SP_URL_LEN_MAX + HTTP_LOCAL_FILE_ROOT_MAX;
    memcpy(&local_file_buf[0],http_cache_file_root.data,http_cache_file_root.len);
    local_file_str.data = &local_file_buf[http_cache_file_root.len];
    ngx_http_snooping_url2local_file(&url_str, &local_file_str, url_node->pcache_rule);
    if (url_node->mime_type->postfix.len != 0) {
        ngx_memcpy(&local_file_str.data[local_file_str.len], 
                   url_node->mime_type->postfix.data,
                   url_node->mime_type->postfix.len);
        local_file_str.len += url_node->mime_type->postfix.len;
        local_file_str.data[local_file_str.len] = '\0';
    }
    fd = ngx_open_file(&local_file_buf[0], 
                       NGX_FILE_WRONLY, 
                       NGX_FILE_CREATE_OR_OPEN,
                       NGX_FILE_DEFAULT_ACCESS);
    if (fd == NGX_INVALID_FILE) {
        err_step = 1;
        goto exit_label;
    }
    url_node->url_flags |= HTTP_SNOOPING_FLG_RES_LFN;
    pbuf = http_url_cache_buf_get(url_node);
    while (pbuf != NULL) {
        ngx_http_buf_write(fd, pbuf->data, pbuf->buf_len);
        http_cache_buf_free(pbuf);
        pbuf = http_url_cache_buf_get(url_node);
    }
    close(fd);
    url_node->url_flags |= HTTP_SNOOPING_FLG_RES_WRITE;
    printf("Save URL %s to Local File %s (File length %d) OK\r\n",
            url_node->url_buf, local_file_buf, url_node->url_res_len);
exit_label:
    if (err_step != 0) {
        pbuf = http_url_cache_buf_get(url_node);
        while (pbuf != NULL) {
            http_cache_buf_free(pbuf);
            pbuf = http_url_cache_buf_get(url_node);
        }
        url_node->url_flags &= ~(HTTP_SNOOPING_FLG_RES_CACHE | HTTP_SNOOPING_FLG_CACHE_BUF_ALLOC);
    }
    return;
}

http_cache_url_t *ngx_http_url_rule_get(ngx_str_t *url_str)
{
    ngx_int_t i, j;
    ngx_str_t head_xiegan_str;
    ngx_str_t mid_xiegan_str;
    ngx_str_t tail_xiegan_str;
    ngx_str_t para_str;
    ngx_str_t post_str;
    http_cache_host_url_t *cache_host;
    http_cache_url_t *url_cache;
    ngx_str_t host = {0, NULL};
    ngx_str_t res_url = {0, NULL};
    GENERIC_ARGUMENT host_key;
    host.data = &url_str->data[0];
    for (i = 0; i < url_str->len; i++) {
        if (url_str->data[i] == '/') {
            host.len = i;
            res_url.data = &url_str->data[i];
            res_url.len = url_str->len - i;
            break;
        } 
    }

    if (res_url.len == 0 || host.len == 0) {
        return NULL;
    }

    host_key.p = &host;
    host.data[host.len] = '\0';
    cache_host = avlSearch(http_cache_host_tree, host_key, http_host_cmp);
    host.data[host.len] = '/';
/*
    for (i = 0; i < http_cache_host_url_num; i++) {
       cache_host = &http_cache_host_url[i];
       if (cache_host->host.len == host.len 
           && (ngx_strncmp(cache_host->host.data, host.data, cache_host->host.len) == 0)) {
           break;
       }
    }
*/
    if (cache_host != NULL) {
        if (HTTP_CACHE_HOST_FORBIDEN & cache_host->cache_flg) {
            return &http_forbiden_rule;
        }
    } else {
        cache_host = &http_cache_default_host_url;
    }

    head_xiegan_str.data = &res_url.data[0];
    mid_xiegan_str.data = &res_url.data[0];
    tail_xiegan_str.data = &res_url.data[0];
    para_str.data = &res_url.data[res_url.len];
    para_str.len = 0;
    post_str.data = &res_url.data[0];
    j = 0;
    for(i = 0; i < res_url.len; i++) {
        if (res_url.data[i] == '/') {
            if (j == 0) {
                head_xiegan_str.data = &res_url.data[i];
                mid_xiegan_str.data = &res_url.data[i];
                tail_xiegan_str.data = &res_url.data[i];
                j = 1;
            } else if (j == 1) {
                mid_xiegan_str.data = &res_url.data[i];
                tail_xiegan_str.data = &res_url.data[i];
                j = 2;
            } else {
                tail_xiegan_str.data = &res_url.data[i];
            }
        } else if (res_url.data[i] == '.') {
            post_str.data = &res_url.data[i];
        } else if (res_url.data[i] == '?') {
            para_str.data = &res_url.data[i];
            para_str.len = res_url.len - i;
            break;
        }
    }

    head_xiegan_str.len = mid_xiegan_str.data - head_xiegan_str.data;
    mid_xiegan_str.len = tail_xiegan_str.data - mid_xiegan_str.data;
    tail_xiegan_str.len = para_str.data - tail_xiegan_str.data;
    post_str.len = para_str.data - post_str.data;
    for (i = 0; i < cache_host->url_num; i++) {
        url_cache = &cache_host->url_cache[i];
        if (url_cache->cache_flg & HTTP_CACHE_URL_HEAD_XIEGAN) {
            if (url_cache->head_xiegan.len != head_xiegan_str.len) {
                continue;
            }
            if (ngx_strncmp(url_cache->head_xiegan.data, head_xiegan_str.data, url_cache->head_xiegan.len) != 0) {
                continue;
            }
        }

        if (url_cache->cache_flg & HTTP_CACHE_URL_MID_XIEGAN) {
            if (url_cache->mid_xiegan.len != mid_xiegan_str.len) {
                continue;
            }
            if (ngx_strncmp(url_cache->mid_xiegan.data, mid_xiegan_str.data, url_cache->mid_xiegan.len) != 0) {
                continue;
            }
        }

        if (url_cache->cache_flg & HTTP_CACHE_URL_TAIL_XIEGAN) {
            if (url_cache->tail_xiegan.len != tail_xiegan_str.len) {
                continue;
            }
            if (ngx_strncmp(url_cache->tail_xiegan.data, tail_xiegan_str.data, url_cache->tail_xiegan.len) != 0) {
                continue;
            }
        }

        if (url_cache->cache_flg & HTTP_CACHE_URL_POST_FILE) {
            if (url_cache->post_file.len != post_str.len) {
                continue;
            }
            if (ngx_strncmp(url_cache->post_file.data, post_str.data, url_cache->post_file.len) != 0) {
                continue;
            }
        }

        if (url_cache->cache_flg & HTTP_CACHE_URL_PARA) {
            if (url_cache->para.len != para_str.len) {
                continue;
            }
            if (ngx_strncmp(url_cache->para.data, para_str.data, url_cache->para.len) != 0) {
                continue;
            }
        }
        break;
    }

    if (i == cache_host->url_num) {
        return NULL;
    }

    return url_cache;
}

void ngx_http_url_write(http_snooping_url_t *url_node)
{
    http_snooping_buf_t *pbuf;
    //ngx_dir_t url_dir;
    ngx_int_t err_step = 0;
    //off_t off_set, fd;
    //ngx_file_t *pfile;
    //ssize_t  n, written, size;

    if ((url_node->url_flags & HTTP_SNOOPING_FLG_RES_CACHE) == 0) {
        err_step = 1;
        goto exit_label;
    }

    if ((url_node->url_flags & HTTP_SNOOPING_FLG_CACHE_BUF_ALLOC) == 0) {
        err_step = 2;
        url_node->url_flags &= ~HTTP_SNOOPING_FLG_RES_CACHE;
        goto exit_label;
    }

    if (url_node->url_flags & HTTP_SNOOPING_FLG_RES_WRITE) {
        err_step = 3;
        goto exit_label;
    }

    if (url_node->url_res_len != url_node->url_data_len) {
        pbuf = http_url_cache_buf_get(url_node);
        while (pbuf != NULL) {
            http_cache_buf_free(pbuf);
            pbuf = http_url_cache_buf_get(url_node);
        }
        url_node->url_data_len = 0;
        url_node->url_flags &= ~HTTP_SNOOPING_FLG_RES_CACHE;
        err_step = 4;
        goto exit_label;
    }
/*
    if (url_node->encoding_type == HTTP_ECODING_TYPE_GZIP) {
        strcat(url_node->local_file_buf, ".gz");
    }

    fd = ngx_open_file(&url_node->local_file_buf[0], 
                       NGX_FILE_WRONLY, 
                       NGX_FILE_CREATE_OR_OPEN,
                       NGX_FILE_DEFAULT_ACCESS);
    if (fd == NGX_INVALID_FILE) {
        err_step = 3;
        goto exit_label;
    }
    url_node->url_flags |= HTTP_SNOOPING_FLG_RES_LFN;
*/
    if (url_node->encoding_type == HTTP_ECODING_TYPE_GZIP) {
        nxg_http_gzip_write(url_node);
    } else if (url_node->encoding_type == HTTP_ECODING_TYPE_DEFLATE) {
        ngx_http_deflate_write(url_node);
    } else {
        ngx_http_none_write(url_node);
    }
#if 0
    pbuf = http_url_cache_buf_get(url_node);
    while (pbuf != NULL) {
        ngx_http_buf_write(fd, pbuf->data, pbuf->buf_len);
/*
        written = 0;
        size = pbuf->buf_len;
        for ( ;; ) {
            n = write(fd, pbuf->data + written, size);
            if (n == -1) {
                http_cache_buf_free(pbuf);
                err_step = 4;
                close(fd);
                goto exit_label;
            }

            written += n;
            if ((size_t) n == size) {
                break;
            }

            size -= n;
        }
*/
        http_cache_buf_free(pbuf);
        pbuf = http_url_cache_buf_get(url_node);
    }
    close(fd);

    if (url_node->encoding_type == HTTP_ECODING_TYPE_GZIP) {
        printk("Uncompress File %s Start\r\n", url_node->local_file_buf);
        file_uncompress(url_node->local_file_buf);
        url_node->local_file_buf[url_node->local_file_name.len + url_node->local_file_path.len] = '\0';
    }
    url_node->url_flags |= HTTP_SNOOPING_FLG_RES_WRITE;
    printk("Save URL %s to Local File %s (File length %d) OK\r\n",
            url_node->url_buf, url_node->local_file_buf, url_node->url_res_len);
#endif

exit_label:
/*
    if (err_step != 0) {
        printk("%s-%d err_step = %d %s\r\n", __FILE__, __LINE__, 
               err_step, url_node->url_buf);
    }
*/
    return ;
}

void ngx_http_snooping_url_local_file_init(http_snooping_url_t *sp_url)
{
    ngx_uint_t i;
    ngx_str_t url_str, local_file_str;
    uchar local_file_buf[HTTP_SP_URL_LEN_MAX + HTTP_LOCAL_FILE_ROOT_MAX];

    if ((sp_url->url_flags & HTTP_SNOOPING_FLG_RES_LFB) == 0) {
        return ;
    }

    if (sp_url->url_flags & HTTP_SNOOPING_FLG_RES_LFOK) {
        return ;
    }

    if ((sp_url->pcache_rule->cache_flg & HTTP_CACHE_URL_SP_WRITE) == 0) {
        sp_url->url_flags |= HTTP_SNOOPING_FLG_RES_LFP | HTTP_SNOOPING_FLG_RES_LFOK;
        return ;
    }

    ngx_memset(local_file_buf, 0, HTTP_SP_LFILE_LEN_MAX + HTTP_LOCAL_FILE_ROOT_MAX);
    url_str.len = sp_url->server_name.len + sp_url->res_url.len;
    url_str.data = sp_url->url_buf;
    local_file_str.len = HTTP_SP_URL_LEN_MAX + HTTP_LOCAL_FILE_ROOT_MAX;
    local_file_str.data = &local_file_buf[0];
    ngx_memcpy(&local_file_buf[0], http_cache_file_root.data,http_cache_file_root.len);
    local_file_str.data = &local_file_buf[http_cache_file_root.len];
    ngx_http_snooping_url2local_file(&url_str, &local_file_str, sp_url->pcache_rule);
    for (i = 1; i < local_file_str.len; i++) {
        if (local_file_str.data[i] == '/') {
            local_file_str.data[i] = '\0';
            (void)ngx_create_dir(&local_file_buf[0], 0702);
            break;
        }
    }
    sp_url->url_flags |= HTTP_SNOOPING_FLG_RES_LFP | HTTP_SNOOPING_FLG_RES_LFOK;
    return ;
}


void ngx_http_snooping_write_process(void)
{
    http_snooping_url_t *url_node;
    ngx_uint_t i = 0, flags;
    ngx_str_t url_str;
    ngx_int_t rc;
    ngx_int_t fd;
    fd = ngx_open_file("/mnt/usb0/html/http-cache.txt", 
                       NGX_FILE_APPEND, 
                       NGX_FILE_CREATE_OR_OPEN,
                       NGX_FILE_DEFAULT_ACCESS);
    while (i < 32) {
        i++;
        url_node = http_url_cache_get();
        if (url_node == NULL) {
             ngx_write_tick++;
             break;
        }
/*
        if ((url_node->url_flags & HTTP_SNOOPING_FLG_RES_WRITE) == 0) {
            url_node->write_error_num++;
        }

        if (url_node->write_error_num >= 2000) {
            HS_URL_SPIN_LOCK(flags);
            if (url_node->url_flags & HTTP_SNOOPING_FLG_URL_HASH) {
                ngx_http_snooping_url_hash_del(url_node);
            }

            list_del(&url_node->res_list);
            list_add(&url_node->res_list, &g_http_snooping_ctx.url_free_list);
            url_node->url_flags = 0;
            g_http_snooping_ctx.url_free++;
            HS_URL_SPIN_UNLOCK(flags);
            continue;
        }
*/
        ngx_http_snooping_url_local_file_init(url_node);
        if ((url_node->url_flags & HTTP_SNOOPING_FLG_RES_LFOK) == 0) {
            http_url_cache_add(url_node, 3);
            continue;
        }

        if ((url_node->pcache_rule->cache_flg & HTTP_CACHE_URL_SP_WRITE)
             && ((url_node->url_flags & HTTP_SNOOPING_FLG_RES_WRITE) == 0)) {
            ngx_http_url_write(url_node);
            if ((url_node->url_flags & HTTP_SNOOPING_FLG_RES_WRITE) == 0) {
                http_url_cache_add(url_node, 3);
                continue;
            }
        }

        if ((url_node->pcache_rule->cache_flg & HTTP_CACHE_URL_ACTION_UPPARSE)
             && ((url_node->url_flags & HTTP_SNOOPING_FLG_NGX_PARSE_OK) == 0)) {
            if (ngx_server_addr == 0) {
                //rc = yk_main(url_node->url_buf);
                url_str.data = url_node->url_buf;
                url_str.len = url_node->res_url.len + url_node->server_name.len;
                rc = ngx_http_server_url_handle(&url_str, HTTP_SP2C_ACTION_PARSE);
                if (rc != 0) {
                    http_url_cache_add(url_node, 11);
                    continue;
                } else {
                    url_node->url_flags |= HTTP_SNOOPING_FLG_NGX_PARSE_OK;
                }
            } else {
                if ((jiffies - url_node->pkt_send_tick) >= 20*HZ) {
                    http_sp2c_req_pkt_t sp2c_req;
                    struct sockaddr_in sc_ad;
                    memset((char *)&sc_ad,0,sizeof(sc_ad));
                    memset((char *)&sp2c_req,0,sizeof(http_sp2c_req_pkt_t));
                    sp2c_req.session_id = HTTP_URL_ID_GET(url_node);
                    sp2c_req.sp2c_action = HTTP_SP2C_ACTION_PARSE;
                    sp2c_req.url_len = url_node->res_url.len + url_node->server_name.len;
                    ngx_memcpy(&sp2c_req.url_data[0], url_node->url_buf, sp2c_req.url_len);
                    sc_ad.sin_family = AF_INET;
                    sc_ad.sin_port = htons(HTTP_SP2C_PORT);
                    sc_ad.sin_addr = htonl(ngx_server_addr);
                    sendto(ngx_sp2c_socket, &sp2c_req, sizeof(http_sp2c_res_pkt_t), 0, (struct sockaddr *)&sc_ad,sizeof(struct sockaddr));
                    url_node->pkt_send_tick = jiffies;
                }
                http_url_cache_add(url_node, 11);
                url_node->url_flags |= HTTP_SNOOPING_FLG_NGX_WAIT_RESPNOSE;
                continue;
            }
        }

        if ((url_node->pcache_rule->cache_flg & HTTP_CACHE_URL_ACTION_CACHE_UPSTREAM)
             && ((url_node->url_flags & HTTP_SNOOPING_FLG_NGX_DOWN) == 0)) {
            if (ngx_server_addr == 0) {
                url_str.data = url_node->url_buf;
                url_str.len = url_node->res_url.len + url_node->server_name.len;
                rc = ngx_http_server_url_handle(&url_str, HTTP_SP2C_ACTION_DOWN);
                if (rc != 0) {
                    http_url_cache_add(url_node, 11);
                    continue;
                } else {
                    url_node->url_flags |= HTTP_SNOOPING_FLG_NGX_PARSE_OK;
                }
            } else {
                if ((jiffies - url_node->pkt_send_tick) >= 20*HZ) {
                    http_sp2c_req_pkt_t sp2c_req;
                    struct sockaddr_in sc_ad;
                    memset((char *)&sc_ad,0,sizeof(sc_ad));
                    memset((char *)&sp2c_req,0,sizeof(http_sp2c_req_pkt_t));
                    sp2c_req.session_id = HTTP_URL_ID_GET(url_node);
                    sp2c_req.sp2c_action = HTTP_SP2C_ACTION_DOWN;
                    sp2c_req.url_len = url_node->res_url.len + url_node->server_name.len;
                    ngx_memcpy(&sp2c_req.url_data[0], url_node->url_buf, sp2c_req.url_len);
                    sc_ad.sin_family = AF_INET;
                    sc_ad.sin_port = htons(HTTP_SP2C_PORT);
                    sc_ad.sin_addr = htonl(ngx_server_addr);
                    sendto(ngx_sp2c_socket, &sp2c_req, sizeof(http_sp2c_res_pkt_t), 0, (struct sockaddr *)&sc_ad,sizeof(struct sockaddr));
                    url_node->pkt_send_tick = jiffies;
                }
                http_url_cache_add(url_node, 11);
                url_node->url_flags |= HTTP_SNOOPING_FLG_NGX_WAIT_RESPNOSE;
                continue;
            }
        }

        if (url_node->url_flags & HTTP_SNOOPING_FLG_NGX_DOWN) {
            url_node->url_flags |= HTTP_SNOOPING_FLG_RES_WRITE;
        }

        if (fd != NGX_INVALID_FILE) {
            if (url_node->url_action == HTTP_C2SP_ACTION_ADD) {
                ngx_http_buf_write(fd , "@@@@+", 5);
            } else {
                ngx_http_buf_write(fd , "@@@@-", 5);
            }

            ngx_http_buf_write(fd , url_node->url_buf, url_node->res_url.len + url_node->server_name.len);
            ngx_http_buf_write(fd , "\r\n", 2);

            if (url_node->url_action == HTTP_C2SP_ACTION_DELETE) {
                HS_URL_SPIN_LOCK(flags);
                if (url_node->url_flags & HTTP_SNOOPING_FLG_URL_HASH) {
                    ngx_http_snooping_url_hash_del(url_node);
                }

                list_del(&url_node->res_list);
                list_add(&url_node->res_list, &g_http_snooping_ctx.url_free_list);
                url_node->url_flags = 0;
                g_http_snooping_ctx.url_free++;
                HS_URL_SPIN_UNLOCK(flags);
            }
        }
    }

    if (fd != NGX_INVALID_FILE) {
        close(fd);
    }
    return;
}

void ngx_http_snooping_load_url()
{
    ngx_int_t fd, offset, i, len = 0, url_id;
    u8 action;
    uchar url_file_buf[HTTP_SP_URL_LEN_MAX + 6 + 2 + 1];
    url_file_buf[HTTP_SP_URL_LEN_MAX + 6 + 2] = '\0';
    ngx_str_t url_str;

    fd = ngx_open_file("/mnt/usb0/html/http-cache.txt", 
                       NGX_FILE_RDONLY, 
                       NGX_FILE_OPEN,
                       NGX_FILE_DEFAULT_ACCESS);
    if (fd != NGX_INVALID_FILE) {
        offset = 0;
	    for (;;) {
		    if (lseek(fd, (off_t) offset, SEEK_SET) == -1) {
		        break;
            }

	        len = (int)read(fd, url_file_buf,sizeof(url_file_buf));
	        if (len <= 7) {
                break;
            }

            if (!ngx_str4cmp(url_file_buf, '@', '@', '@', '@')) {
                break;
            }

            for(i = 4; i < len; i++) {
                if (url_file_buf[i] == '\r' 
                    && url_file_buf[i+1] == '\n') {
                    break;
                }
            }

            if (i == len) {
                break;
            }

            url_file_buf[i] = '\0';
            if (url_file_buf[4] == '+') {
                action = HTTP_C2SP_ACTION_ADD;
            } else {
                action = HTTP_C2SP_ACTION_DELETE;
            }
            url_str.data = &url_file_buf[5];
            url_str.len = i - 5;
            url_id = ngx_http_sp_url_handle(&url_str, action);
		    if (action == HTTP_C2SP_ACTION_ADD) {
		        ngx_http_sp_url_state_setby_id(url_id, HTTP_LOCAL_DOWN_STATE_OK);
		    }
            offset += i + 2;
	    }
        close(fd);
        unlink("/mnt/usb0/html/http-cache.txt");
    }
}
void ngx_http_snooping_write_task(ulong argc, void *argv) 
{
    sleep(HZ * 60);
    ngx_http_snooping_load_url();
    while (1) {
        if (g_ngx_sp_write_task_exiting) {
            printk("%s<%d>: g_ngx_sp_write_task exiting...\r\n", __func__, __LINE__);
            break;
        }
        sleep(HZ * 2);
        ngx_http_snooping_write_process();
    }

    return;
}

void ngx_http_sp2c_rec_process(void)
{
    http_snooping_url_t *url_node;
    http_sp2c_res_pkt_t sp2c_req;
    struct sockaddr_in sc_ad;
    ngx_int_t rc, err_step = 0;
    unsigned int peerlen = sizeof(sc_ad);

    /* ZHAOYAO XXX TODO: 资源释放 */

    rc = recvfrom(ngx_sp2c_socket, (void *)&sp2c_req, sizeof(http_sp2c_res_pkt_t), 0, (struct sockaddr *)&sc_ad, &peerlen);
    if (rc != sizeof(http_sp2c_res_pkt_t)) {
        err_step = 1;
        goto exit_label;
    }

    if (sp2c_req.status != HTTP_SP_STATUS_OK) {
        err_step = 2;
        goto exit_label;
    }

    if (sp2c_req.session_id >= HTTP_SP_URL_MAX) {
        err_step = 3;
        goto exit_label;
    }

    url_node = g_http_snooping_ctx.url_base + sp2c_req.session_id;
    if ((url_node->url_flags & HTTP_SNOOPING_FLG_NGX_WAIT_RESPNOSE) == 0) {
        err_step = 4;
        goto exit_label;
    }

    url_node->url_flags &= ~HTTP_SNOOPING_FLG_NGX_WAIT_RESPNOSE;
    url_node->url_flags |= HTTP_SNOOPING_FLG_NGX_PARSE_OK;
exit_label:

    return;
}

void ngx_http_c2sp_rec_process(void)
{
    ngx_int_t url_id;
    http_c2sp_req_pkt_t c2sp_req;
    ngx_str_t url_str;
    struct sockaddr_in sc_ad;
    ngx_int_t rc, err_step = 0;
    unsigned int peerlen = sizeof(sc_ad);

    /* ZHAOYAO XXX TODO: 资源释放 */

    rc = recvfrom(ngx_c2sp_socket, (void *)&c2sp_req, sizeof(http_c2sp_req_pkt_t), 0, (struct sockaddr *)&sc_ad, &peerlen);
    if (rc != sizeof(http_c2sp_req_pkt_t)) {
        err_step = 1;
        goto exit_label;
    }

    if (c2sp_req.c2sp_action < HTTP_C2SP_ACTION_GET
        || c2sp_req.c2sp_action > HTTP_C2SP_ACTION_DELETE) {
        err_step = 2;
        goto exit_label;
    }

    url_str.len = c2sp_req.url_len;
    url_str.data = c2sp_req.usr_data;
    url_id = ngx_http_sp_url_handle(&url_str, c2sp_req.c2sp_action);
    if (c2sp_req.c2sp_action == HTTP_C2SP_ACTION_ADD) {
        ngx_http_sp_url_state_setby_id(url_id, HTTP_LOCAL_DOWN_STATE_OK);
    }

    http_c2sp_res_pkt_t c2sp_res;
    c2sp_res.session_id = c2sp_req.session_id;
    c2sp_res.status = HTTP_SP_STATUS_OK;
    sendto(ngx_c2sp_socket, &c2sp_res, sizeof(http_c2sp_res_pkt_t), 0, (struct sockaddr *)&sc_ad,sizeof(struct sockaddr));
exit_label:

    return;
}

void ngx_http_snooping_rxtx_task(ulong argc, void *argv) 
{
    ngx_int_t err_step = 0, fd, rc, i;
    struct sockaddr_in sc_ad;
    fd_set read_fset;
    struct timeval time_out;
   
    memset((char *)&sc_ad,0,sizeof(sc_ad));
    sc_ad.sin_addr = INADDR_ANY;
    sc_ad.sin_family = AF_INET;
    ngx_c2sp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (ngx_c2sp_socket == -1) {
        printk("ERROR: %s<%d>: create ngx_c2sp_socket failed.\r\n", __func__, __LINE__);
        goto err_out0;
    }

    i = 1;
    setsockopt(ngx_c2sp_socket, SOL_SOCKET, SO_REUSEADDR,(char *)&i,sizeof(i));
    /*i = sizeof(http_c2sp_res_pkt_t);
    setsockopt(ngx_c2sp_socket, SOL_SOCKET, SO_RCVBUF,(char *)&i,sizeof(i));*/
    sc_ad.sin_port = htons(HTTP_C2SP_PORT);
    if (bind(ngx_c2sp_socket, (struct sockaddr *)&sc_ad, sizeof(struct sockaddr)) == -1) {
        printk("ERROR: %s<%d>: bind ngx_c2sp_socket failed.\r\n", __func__, __LINE__);
        goto err_out1;
    }

    ngx_sp2c_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (ngx_sp2c_socket == -1) {
        printk("ERROR: %s<%d>: create ngx_sp2c_socket failed.\r\n", __func__, __LINE__);
        goto err_out1;
    }

    i = 1;
    setsockopt(ngx_sp2c_socket, SOL_SOCKET, SO_REUSEADDR,(char *)&i, sizeof(i));
    /*i = sizeof(http_sp2c_res_pkt_t);
    setsockopt(ngx_sp2c_socket, SOL_SOCKET, SO_RCVBUF,(char *)&i, sizeof(i));*/
    sc_ad.sin_port = htons(HTTP_SP2C_PORT);
    if (bind(ngx_sp2c_socket, (struct sockaddr *)&sc_ad, sizeof(struct sockaddr)) == -1) {
        printk("ERROR: %s<%d>: bind ngx_sp2c_socket failed.\r\n", __func__, __LINE__);
        goto err_out2;
    }

    fd = MAX(ngx_sp2c_socket, ngx_c2sp_socket);
    while (1) {
        if (g_ngx_sp_rxtx_task_exiting) {
            printk("%s<%d>: exiting...\r\n", __func__, __LINE__);
            break;
        }
        time_out.tv_sec = 1;    /* ZHAOYAO XXX: 为了监听exit，降低timeout时间，从10s变为1s */
        time_out.tv_usec = 0;
        FD_ZERO(&read_fset);
        FD_SET(ngx_sp2c_socket, &read_fset);
        FD_SET(ngx_c2sp_socket, &read_fset);
        rc = select(fd + 1, &read_fset, NULL, NULL, &time_out);
        if (rc > 0) {
            if (FD_ISSET(ngx_sp2c_socket, &read_fset)) {
                ngx_http_sp2c_rec_process();
            }

            if (FD_ISSET(ngx_c2sp_socket, &read_fset)) {
                ngx_http_c2sp_rec_process();
            }
        } else if (rc == 0) {
            continue;
        } else {
            printk_err("HTTP-SNOOPING", "Select", "%s-%d\r\n", __FILE__, __LINE__);
            break;
        }
    }

err_out2:
    printk("%s<%d>: close ngx_sp2c_socket ...\r\n", __func__, __LINE__);
    close(ngx_sp2c_socket);

err_out1:
    printk("%s<%d>: close ngx_c2sp_socket ...\r\n", __func__, __LINE__);
    close(ngx_c2sp_socket);

err_out0:
    return;
}


/* ZHAOYAO TODO FIXME: 在这里extern是临时之举，需修订 */
extern struct_transtion TNAME(cfg_snoop_client_append_point);


void ngx_http_snooping_init(ngx_log_t *log) 
{
    ngx_int_t err_step = 0;
    ngx_int_t i;
    http_snooping_url_t *url_node;
    http_sp_connection_t *con_node;
    http_snooping_buf_t *buf_node;
    http_mime_type_t *pmime;
    http_cache_host_url_t *phost_url;
    //http_cache_file_avlnode_t *pcache_file_node;
    //ngx_str_t *pStr;
    GENERIC_ARGUMENT node_key;
    g_http_snooping_ctx.con_lock = 0; 
    g_http_snooping_ctx.url_lock = 0;
    g_http_snooping_ctx.buf_lock = 0; 
    g_http_snooping_ctx.http_sp_log = log;
    g_http_snooping_ctx.http_sp_url_pool = ngx_create_pool(NGX_DEFAULT_POOL_SIZE, g_http_snooping_ctx.http_sp_log);
    if (g_http_snooping_ctx.http_sp_url_pool == NULL) {
        err_step = 1;
        goto end_label;
    }

    for(i = 0; i < HTTP_MAX_HOST; i++) {
        http_homepage_hash[i] = 0;
    }

    g_http_snooping_ctx.url_max = HTTP_SP_URL_MAX;
    g_http_snooping_ctx.url_free = g_http_snooping_ctx.url_max;
    g_http_snooping_ctx.url_base = (http_snooping_url_t *)kmalloc(g_http_snooping_ctx.url_max * sizeof(http_snooping_url_t), 0);
    if (g_http_snooping_ctx.url_base == NULL) {
        err_step = 2;
        goto end_label;
    }
    memset(g_http_snooping_ctx.url_base, 0, g_http_snooping_ctx.url_max * sizeof(http_snooping_url_t));
    INIT_LIST_HEAD(&g_http_snooping_ctx.url_free_list);
    INIT_LIST_HEAD(&g_http_snooping_ctx.url_use_list);
    for (i = 0; i < g_http_snooping_ctx.url_max; i++) {
        url_node = &g_http_snooping_ctx.url_base[i];
        list_add(&url_node->res_list, &g_http_snooping_ctx.url_free_list);
    }

    for (i = 0; i < HTTP_SP_URL_MAX; i++) {
        g_http_snooping_ctx.url_hash_lock[i] = 0;
        INIT_LIST_HEAD(&g_http_snooping_ctx.url_hash[i]);
    }

    g_http_snooping_ctx.con_max = 2048;
    g_http_snooping_ctx.con_base = (http_sp_connection_t *)kmalloc(g_http_snooping_ctx.con_max * sizeof(http_sp_connection_t), 0); 
    if (g_http_snooping_ctx.con_base == NULL) {
        err_step = 3;
        goto end_label;
    }
    memset(g_http_snooping_ctx.con_base, 0, g_http_snooping_ctx.con_max * sizeof(http_sp_connection_t));
    g_http_snooping_ctx.con_num = 0;
    INIT_LIST_HEAD(&g_http_snooping_ctx.con_free_list);
    INIT_LIST_HEAD(&g_http_snooping_ctx.con_use_list);
    g_http_snooping_ctx.con_lock = 0;
    for (i = 0; i < g_http_snooping_ctx.con_max; i++) {
        con_node = &g_http_snooping_ctx.con_base[i];
        list_add(&con_node->res_list, &g_http_snooping_ctx.con_free_list);        
    }

    g_http_snooping_ctx.cache_num = 0;
    g_http_snooping_ctx.cache_pad = 0;
    for(i = 0; i < HTTP_CACHE_TIME_OUT; i++) {
        INIT_LIST_HEAD(&g_http_snooping_ctx.url_cache_list[i]);
    }

    INIT_LIST_HEAD(&g_http_snooping_ctx.data_buf_free_list);
    g_http_snooping_ctx.buf_max = HTTP_SP_BUF_NUM_MAX;
    g_http_snooping_ctx.buf_num = HTTP_SP_BUF_NUM_MAX;
    g_http_snooping_ctx.data_buf_base[0] = (http_snooping_buf_t *)kmalloc(HTTP_SP_BUF_NUM_MAX * sizeof(http_snooping_buf_t) ,0);
    if (g_http_snooping_ctx.data_buf_base[0] == NULL) {
        err_step = 4;
        goto end_label;
    }
/*
    g_http_snooping_ctx.data_buf_base[1] = (http_snooping_buf_t *)kmalloc(HTTP_SP_BUF_NUM_MAX * sizeof(http_snooping_buf_t) ,0);
    if (g_http_snooping_ctx.data_buf_base[1] == NULL) {
        err_step = 5;
        goto end_label;
    }
*/
    memset(g_http_snooping_ctx.data_buf_base[0], 0, HTTP_SP_BUF_NUM_MAX * sizeof(http_snooping_buf_t));
    //memset(g_http_snooping_ctx.data_buf_base[1], 0, HTTP_SP_BUF_NUM_MAX * sizeof(http_snooping_buf_t));
    http_snooping_buf_t *buf_base;
    buf_base = g_http_snooping_ctx.data_buf_base[0];
    for (i = 0; i < HTTP_SP_BUF_NUM_MAX; i++) {
        buf_node = &buf_base[i];
        list_add(&buf_node->res_list, &g_http_snooping_ctx.data_buf_free_list);     
    }
/*
    buf_base = g_http_snooping_ctx.data_buf_base[1];
    for (i = 0; i < HTTP_SP_BUF_NUM_MAX; i++) {
        buf_node = &buf_base[i];
        list_add(&buf_node->res_list, &g_http_snooping_ctx.data_buf_free_list);     
    }
*/

    for (i = 0; i < http_mime_num; i++) {
        pmime = &http_mime_type[i];
        node_key.p = &pmime->mime_type;
        avlInsert(&http_mime_tree, pmime, node_key, http_snooping_mime_cmp);
    }

    for (i = 0; i < http_host_num; i++) {
        phost_url = &http_cache_host_url[i];
        node_key.p = &phost_url->host;
        avlInsert(&http_cache_host_tree, phost_url, node_key, http_host_cmp);
    }

    g_ngx_sp_write_task = create_task("nginx-http-write", ngx_http_snooping_write_task, 0, NULL,
        (128 * 1024), APP_TASK);
    if (g_ngx_sp_write_task == NULL) {
        err_step = 8;
        printk_err("NGINX", "SUB_SYS", "%s", "Create Http Snooping Write Task Failed\n");
    }

    g_ngx_sp_rxtx_task = create_task("nginx-httpsp-rxtx", ngx_http_snooping_rxtx_task, 0, NULL,
        (64 * 1024), APP_TASK);
    if (g_ngx_sp_rxtx_task == NULL) {
        err_step = 9;
        printk_err("NGINX", "SUB_SYS", "%s", "Create Http Snooping RXTX Task Failed\n");
    }

    wan_ta_app_reg(&ngx_http_snooping_app, WAN_TA_APP_HTTP_SNOOPING);
    //cli_add_command(PARSE_ADD_SHOW_CMD, &TNAME(exec_show_http_app), "Show http url cache Command");
    //cli_add_command(PARSE_ADD_CFG_TOP_CMD, &TNAME(cfg_httpsp_app), "http snooping");
    //cli_add_command(PARSE_ADD_CFG_TOP_CMD, &TNAME(cfg_snoop_client_append_point),
    //                    "http snooping client");
end_label:
    printk("%s-%d err_step = %d\r\n", __FILE__, __LINE__, err_step);
    return;
}

void ngx_http_snooping_uninit()
{
    /* ZHAOYAO TODO: 卸载CLI，调试中并不添加CLI */
    

    /* ZHAOYAO XXX: 卸载传输优化注册 */
    wan_ta_app_dreg(WAN_TA_APP_HTTP_SNOOPING);

    /* ZHAOYAO XXX: 关闭g_ngx_sp_rxtx_task任务 */
    if (g_ngx_sp_rxtx_task != NULL) {
        g_ngx_sp_rxtx_task_exiting = 1;
        sleep(2 * HZ);
        printk_rt("%s<%d>: deleting g_ngx_sp_rxtx_task.\r\n", __func__, __LINE__);
        delete_task(g_ngx_sp_rxtx_task);
        g_ngx_sp_rxtx_task = NULL;
        g_ngx_sp_rxtx_task_exiting = 0;
    }

    /* ZHAOYAO XXX: 关闭g_ngx_sp_write_task任务 */
    if (g_ngx_sp_write_task != NULL) {
        g_ngx_sp_write_task_exiting = 1;
        sleep(3 * HZ);
        printk_rt("%s<%d>: deleting g_ngx_sp_write_task.\r\n", __func__, __LINE__);
        delete_task(g_ngx_sp_write_task);
        g_ngx_sp_write_task = NULL;
        g_ngx_sp_write_task_exiting = 0;
    }

    /* ZHAOYAO XXX: 销毁AVL树http_cache_host_tree和http_mime_tree，AVL的节点都是静态分配，所以不需要free */
    http_cache_host_tree = NULL;
    http_mime_tree = NULL;

    /* ZHAOYAO TODO: 释放g_http_snooping_ctx.data_buf_base[0]空间, 注意数据面与控制面的锁 */
    list_del_init(&g_http_snooping_ctx.data_buf_free_list);
    kfree(g_http_snooping_ctx.data_buf_base[0]);

    /* ZHAOYAO TODO: 释放g_http_snooping_ctx.con_base空间, 注意数据面与控制面的锁 */
    list_del_init(&g_http_snooping_ctx.con_free_list);
    list_del_init(&g_http_snooping_ctx.con_use_list);
    kfree(g_http_snooping_ctx.con_base);

    /* ZHAOYAO TODO: 释放g_http_snooping_ctx.url_base空间, 注意数据面与控制面的锁 */
    list_del_init(&g_http_snooping_ctx.url_free_list);
    list_del_init(&g_http_snooping_ctx.url_use_list);
    kfree(g_http_snooping_ctx.url_base);

    /* ZHAOYAO TODO: 释放g_http_snooping_ctx.http_sp_url_pool池 */
    ngx_destroy_pool(g_http_snooping_ctx.http_sp_url_pool);
}

ngx_int_t ngx_http_snooping_create(tcp_proxy_cb_t *tp_entry)
{
    http_sp_connection_t *pcon_node = NULL;
    ef_intfcb *src_intf;
    struct list_head *plist;

    if (tp_entry->in_addr_dst == ngx_server_addr) {
        return WAN_TA_APP_NOT_MATCH;
    }

    if (tp_entry->in_port_dst == 8080 || tp_entry->in_port_dst == 80) {
        HS_CON_DATA_SPIN_LOCK();
        if (g_http_snooping_ctx.con_num < g_http_snooping_ctx.con_max) {
            plist = g_http_snooping_ctx.con_free_list.next;
            list_del(plist);
            pcon_node = list_entry(plist, http_sp_connection_t, res_list);
            //memset(pcon_node, 0, sizeof(http_sp_connection_t));
            g_http_snooping_ctx.con_num++;
            list_add(&pcon_node->res_list, &g_http_snooping_ctx.con_use_list);
        }
        HS_CON_DATA_SPIN_UNLOCK();

        if (pcon_node != NULL){
            pcon_node->con_flags = HTTP_SNOOPING_FLG_RES;
            pcon_node->wan_ta_client_tp = (tcp_proxy_cb_t *)tp_entry;
            pcon_node->server_addr = tp_entry->in_addr_dst;
            pcon_node->client_addr = tp_entry->in_addr_src;
            pcon_node->server_port = tp_entry->in_port_dst;
            pcon_node->client_port = tp_entry->in_port_src;
            pcon_node->req_method = HTTP_SP_METHOD_NONE;
            pcon_node->req_state = HTTP_SP_STATE_INIT;
            pcon_node->res_state = HTTP_SP_STATE_INIT;
            pcon_node->parse_req_pos = 0;
            pcon_node->parse_res_pos = 0;
            pcon_node->rec_req_data_len = 0;
            pcon_node->rec_res_data_len = 0;
            pcon_node->req_rec_pkt = 0;
            pcon_node->req_snd_pkt = 0;
            pcon_node->res_rec_pkt = 0;
            pcon_node->res_snd_pkt = 0;
            pcon_node->con_url = NULL;
            src_intf = (ef_intfcb *)tp_entry->src_intf;
            pcon_node->local_server_addr = src_intf->ipaddr;
            ef_buf_init_chain(&pcon_node->request_pkt_chain);
            ef_buf_init_chain(&pcon_node->response_pkt_chain);
            tp_entry->wan_ta_app_cb = pcon_node;
            tp_entry->pair->wan_ta_app_cb = pcon_node;
            tp_entry->wan_ta_app_type = WAN_TA_APP_HTTP_SNOOPING;
            tp_entry->pair->wan_ta_app_type = WAN_TA_APP_HTTP_SNOOPING;
        } else {
            tp_entry->wan_ta_app_type = WAN_TA_APP_NONE;
            tp_entry->pair->wan_ta_app_type = WAN_TA_APP_NONE;
        }
        return WAN_TA_APP_MATCH;
    }

    return WAN_TA_APP_NOT_MATCH;
}

void ngx_http_snooping_shut(tcp_proxy_cb_t *tp_entry)
{
    http_sp_connection_t *pcon_node;
    struct list_head *plist;
    if (tp_entry->wan_ta_app_type == WAN_TA_APP_HTTP_SNOOPING) {
        tp_entry->wan_ta_app_type = WAN_TA_APP_NONE;
        pcon_node = (http_sp_connection_t *)tp_entry->wan_ta_app_cb;
        tp_entry->wan_ta_app_cb = NULL;
        if(tp_entry->pair->wan_ta_app_type == WAN_TA_APP_NONE) {
            HS_CON_DATA_SPIN_LOCK();
            if (pcon_node->con_flags & HTTP_SNOOPING_FLG_RES) {
                if (pcon_node->con_url != NULL) {
                    pcon_node->con_url->url_flags |= HTTP_SNOOPING_FLG_RES_CACHE;
                    pcon_node->con_url = NULL;
                }
                pcon_node->con_flags = 0;
                if (pcon_node->request_pkt_chain.count > 0) {
                    ef_buf_free_chain(pcon_node->request_pkt_chain.head, 
                                      pcon_node->request_pkt_chain.tail, 
                                      pcon_node->request_pkt_chain.count);
                    ef_buf_init_chain(&pcon_node->request_pkt_chain);
                }

                if (pcon_node->response_pkt_chain.count > 0) {
                    ef_buf_free_chain(pcon_node->response_pkt_chain.head, 
                                      pcon_node->response_pkt_chain.tail, 
                                      pcon_node->response_pkt_chain.count);
                    ef_buf_init_chain(&pcon_node->response_pkt_chain);
                }
                plist = &pcon_node->res_list;
                list_del(plist);
                list_add(plist, &g_http_snooping_ctx.con_free_list);
                g_http_snooping_ctx.con_num--;
            }
            HS_CON_DATA_SPIN_UNLOCK();
        } 
    }
    return ;
}

void ngx_http_snooping_data(tcp_proxy_cb_t *tp_entry)
{
    tcp_proxy_cb_t *tp_cb;
    ngx_uint_t token;
    ef_buf_t *efb;
    ef_buf_chain_t pkt_chain;
    http_sp_connection_t *pcon_node;
    tp_cb = tp_entry;
    token = tp_cb->tp_socket->write_queue_free_space(tp_cb->pair->sk);
    efb = tp_cb->tp_socket->recvmsg(tp_cb->sk, token);
    ef_buf_init_chain(&pkt_chain);
    while (efb) {
        ef_buf_efb_to_chain(&pkt_chain, efb);
        efb = efb->nextPkt;
    }

    pcon_node = (http_sp_connection_t *)tp_entry->wan_ta_app_cb;
    if (pcon_node->wan_ta_client_tp == tp_entry) {
        ngx_http_snooping_request_parse(tp_entry, &pkt_chain, token);
    } else {
        ngx_http_snooping_response_parse(tp_entry, &pkt_chain, token);
    }

    if (pkt_chain.count != 0) {
        efb = pkt_chain.head;
        pkt_chain.tail->nextPkt = NULL;
        tp_cb->tp_socket->sendmsg(tp_cb->pair->sk, efb);
    }

    return;
}


