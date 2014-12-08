/*
 * $id: $
 * Copyright (C)2001-2009 锐捷网络.  All rights reserved.
 *
 * http-snooping.h
 * Original Author:  tangyoucan@ruijie.com.cn, 2014-03-03
 *
 * HTTP头文件
 *
 * History :
 *  v1.0    (tangyoucan@ruijie.com.cn)  2014-03-03  Initial version.
 */
#ifndef __HTTPSNOOPING_H__
#define __HTTPSNOOPING_H__

#define HTTP_SP_URL_MAX (32*1024)
#define HTTP_SP_RES_ID_INVALID (HTTP_SP_URL_MAX + 1)

#define HTTP_TA_STATE_CREATE 1
#define HTTP_TA_STATE_URL_PARSE 2
#define HTTP_TA_STATE_URL_PARSE_DONE 3
#define HTTP_TA_STATE_URL_REDIRECT 4
#define HTTP_TA_STATE_URL_DELETE 5

#define HTTP_SNOOPING_FLG_RES (1<<0)
#define HTTP_SNOOPING_FLG_FILTER (1<<1)
#define HTTP_SNOOPING_FLG_CACHE_LIST (1<<2)
#define HTTP_SNOOPING_FLG_URL_HASH (1<<3)
#define HTTP_SNOOPING_FLG_SADDR (1<<4)
#define HTTP_SNOOPING_FLG_SNAME (1<<5)
#define HTTP_SNOOPING_FLG_RES_URL (1<<6)
#define HTTP_SNOOPING_FLG_RES_LFB (1<<7)
#define HTTP_SNOOPING_FLG_RES_LFOK (1<<8)
#define HTTP_SNOOPING_FLG_RES_LFN (1<<9)
#define HTTP_SNOOPING_FLG_RES_LFP (1<<10)
#define HTTP_SNOOPING_FLG_CACHE_BUF_ALLOC (1<<11)
#define HTTP_SNOOPING_FLG_RES_CACHE (1<<12)
#define HTTP_SNOOPING_FLG_RES_WRITE (1<<13)
#define HTTP_SNOOPING_FLG_NGX_DOWN (1<<14)
#define HTTP_SNOOPING_FLG_NGX_PARSE_OK (1<<15)
#define HTTP_SNOOPING_FLG_NGX_WAIT_RESPNOSE  (1 << 16)

//#define HTTP_SP_URL_LEN_MAX 512
#define HTTP_SP_URL_LEN_MAX 600
#define HTTP_SP_LFILE_LEN_MAX 256
#define HTTP_SP_REQ_HEAD_LEN_MAX (3072 - 512)
#define HTTP_SP_RES_HEAD_LEN_MAX 1024

#define HTTP_SP_METHOD_NONE   0
#define HTTP_SP_METHOD_GET    1
#define HTTP_SP_METHOD_POST   2
#define HTTP_SP_METHOD_HEAD   3
#define HTTP_SP_METHOD_PUT    4
#define HTTP_SP_METHOD_DELETE 5

#define HTTP_SP_STATE_INIT                      1
#define HTTP_SP_STATE_INIT_DATA                 2
#define HTTP_SP_STATE_REQUEST_HEADER            3
#define HTTP_SP_STATE_REQUEST_HEADER_DATA       4
#define HTTP_SP_STATE_DATA_LINE                 5
#define HTTP_SP_STATE_DATA_LINE_DATA            6
#define HTTP_SP_STATE_DONE                      7
#define HTTP_SP_STATE_REQUEST_HEADER_ERROR      8
#define HTTP_SP_STATE_HEADER_ERROR              9
#define HTTP_SP_STATE_DATA_ERROR                10

#define HTTP_URL_PASS 1
#define HTTP_URL_REDIRECT 2
#define HTTP_URL_REJECT 3

#define HTTP_SP_BUF_NUM_MAX (4*1024)
#define HTTP_SP_BUF_LEN_MAX (8*1024 - sizeof(struct list_head) - 4)
#define HTTP_NGX_NULL_STR {0, NULL}

typedef struct http_mime_type_s
{
    AVL_NODE avl_node;
    ngx_str_t mime_type;
    ngx_str_t postfix;
}http_mime_type_t;
/*
typedef struct http_mime_avlnode_s
{
    AVL_NODE avl_node;
    http_mime_type_t *mime_data;
}http_mime_avlnode_t;
*/
typedef struct http_cache_file_avlnode_s
{
    AVL_NODE avl_node;
    ngx_str_t *cache_str;
}http_cache_file_avlnode_t;

#define HTTP_CACHE_URL_HEAD_XIEGAN  (1 << 0)
#define HTTP_CACHE_URL_MID_XIEGAN  (1 << 1)
#define HTTP_CACHE_URL_TAIL_XIEGAN  (1 << 2)
#define HTTP_CACHE_URL_POST_FILE  (1 << 3)
#define HTTP_CACHE_URL_PARA  (1 << 4)
#define HTTP_CACHE_URL_ACTION_CACHE_SP  (1 << 5)
#define HTTP_CACHE_URL_ACTION_CACHE_UPSTREAM  (1 << 6)
#define HTTP_CACHE_URL_ACTION_UPPARSE  (1 << 7)
#define HTTP_CACHE_URL_SP_WRITE  (1 << 8)
#define HTTP_CACHE_URL_REDIRECT_PARA  (1 << 9)
#define HTTP_CACHE_URL_FORBIDEN  (1 << 10)
#define HTTP_CACHE_URL_CON_CLOSE  (1 << 11)
#define HTTP_CACHE_URL_HOMEPAGE_POLL  (1 << 12)


#define HTTP_CACHE_KEY_HOST  (1 << 0)
#define HTTP_CACHE_KEY_1_XIEGAN  (1 << 1)
#define HTTP_CACHE_KEY_2_XIEGAN  (1 << 2)
#define HTTP_CACHE_KEY_3_XIEGAN  (1 << 3)
#define HTTP_CACHE_KEY_4_XIEGAN  (1 << 4)
#define HTTP_CACHE_KEY_5_XIEGAN  (1 << 5)
#define HTTP_CACHE_KEY_6_XIEGAN  (1 << 6)
#define HTTP_CACHE_KEY_7_XIEGAN  (1 << 7)
#define HTTP_CACHE_KEY_8_XIEGAN  (1 << 8)
#define HTTP_CACHE_KEY_9_XIEGAN  (1 << 9)
#define HTTP_CACHE_KEY_10_XIEGAN  (1 << 10)
#define HTTP_CACHE_KEY_ALL_XIEGAN  (HTTP_CACHE_KEY_1_XIEGAN | HTTP_CACHE_KEY_2_XIEGAN | \
                    HTTP_CACHE_KEY_3_XIEGAN | HTTP_CACHE_KEY_4_XIEGAN | HTTP_CACHE_KEY_5_XIEGAN | HTTP_CACHE_KEY_6_XIEGAN | \
                    HTTP_CACHE_KEY_7_XIEGAN | HTTP_CACHE_KEY_8_XIEGAN | HTTP_CACHE_KEY_9_XIEGAN | HTTP_CACHE_KEY_10_XIEGAN) 
#define HTTP_CACHE_KEY_PARA  (1 << 11)
#define HTTP_CACHE_KEY_FILE  (1 << 12)

typedef struct http_cache_url_s
{
    ngx_uint_t cache_flg;
    ngx_uint_t key_flg;
    ngx_str_t head_xiegan;
    ngx_str_t mid_xiegan;
    ngx_str_t tail_xiegan;
    ngx_str_t post_file;
    ngx_str_t para;
    void (*url_file_key_parse)(ngx_str_t *para);
    void (*url_para_key_parse)(ngx_str_t *para);
}http_cache_url_t;

typedef struct http_cache_url_avlnode_s
{
    AVL_NODE avl_node;
    http_cache_url_t *cache_url_data;
}http_cache_url_avlnode_t;

#define HTTP_CACHE_HOST_FORBIDEN (1<<0)
typedef struct http_cache_host_url_s
{
    AVL_NODE avl_node;
    ngx_uint_t cache_flg;
    ngx_str_t host;
    http_cache_url_t *url_cache;
    ngx_uint_t url_num;
}http_cache_host_url_t;

typedef struct http_snooping_buf_s
{
    struct list_head res_list;
    u16 buf_len;
    u16 buf_flag;
    uchar data[HTTP_SP_BUF_LEN_MAX];
}http_snooping_buf_t;

#define HTTP_LOCAL_FILE_ROOT_MAX 64
typedef struct http_snooping_url_s
{
    ngx_uint_t url_flags;       //有效参数标识位
    struct list_head res_list;        //空闲链
    struct list_head filter_list;     //安全链
    struct list_head user_list;       //用户行为分析链
    struct list_head url_hash_list;   //资源查找HASH连
    struct list_head cache_buf_list;
    struct list_head data_buf_list;
    unsigned long url_data_lock;
    http_snooping_buf_t *url_data_current;
    u32 server_addr;     //服务器地址
    u32 client_addr;     //服务器地址
    u16 server_port;     //服务器地址
    u16 client_port;     //服务器地址
    u32 redirect_addr;     //重定向服务器地址
    ngx_str_t  server_name;     //服务器名称
    ngx_str_t  res_url;         //资源url，不包含主机名
    ngx_str_t  res_url_para;    //资源url参数
#if 0
    ngx_str_t  local_file_name; //资源对应的本地文件名
    ngx_str_t  local_file_path; //资源对应的本地
    ngx_str_t  redirect_file_name;
#endif
    ngx_str_t  file_key_name; 
    ngx_int_t url_res_len;
    ngx_int_t url_data_len;
    u8 encoding_type;
    u8 data_start;
    u16 url_access_num;
    u16 write_error_num;
    u8 local_down_state;
    u8 url_action;
    ngx_uint_t pkt_send_tick;
    http_mime_type_t *mime_type;
    http_cache_url_t *pcache_rule;
    uchar url_buf[HTTP_SP_URL_LEN_MAX];//URL字符串缓冲区
    uchar key_buf[HTTP_SP_LFILE_LEN_MAX];//URL字符串缓冲区
    //uchar local_file_buf[HTTP_SP_LFILE_LEN_MAX + HTTP_LOCAL_FILE_ROOT_MAX];//本地文件名缓冲区
}http_snooping_url_t;
#define HTTP_LOCAL_DOWN_STATE_INIT 0
#define HTTP_LOCAL_DOWN_STATE_OK 1
#define HTTP_LOCAL_DOWN_STATE_ERROR 2
#define HTTP_LOCAL_DOWN_STATE_ING 3
#define HTTP_LOCAL_DOWN_STATE_SET(url, state) (url->local_down_state = state)
#define HTTP_LOCAL_DOWN_STATE_GET(url) (url->local_down_state)

#define HTTP_ECODING_TYPE_NONE     0
#define HTTP_ECODING_TYPE_GZIP     1
#define HTTP_ECODING_TYPE_DEFLATE  2

typedef struct http_sp_connection_s {
    ngx_uint_t con_flags;       //有效参数标识位
    struct list_head res_list;  //空闲链
    u32 server_addr;     //服务器地址
    u32 client_addr;     //服务器地址
    u16 server_port;     //服务器地址
    u16 client_port;     //服务器地址
    u32 local_server_addr;
    u8 req_method;
    u8 req_state;
    u8 res_state;
    u8 encoding_type;
    u8 trans_encoding_type;
    u8 post_fix;
    u8 res1_pad[2];
    ngx_str_t  server_name;     //服务器名称
    ngx_str_t  res_url;         //资源url，不包含主机名
    ngx_str_t  url_para;         //资源参数
    ef_buf_chain_t request_pkt_chain;
    ef_buf_chain_t response_pkt_chain;    
    ngx_int_t req_data_len; 
    ngx_uint_t rec_req_data_len; 
    ngx_uint_t rec_req_data_tick; 
    ngx_uint_t res_data_len; 
    ngx_uint_t rec_res_data_len; 
    ngx_uint_t rec_res_data_tick; 
    ngx_str_t  content_len; 
    ngx_str_t  content_type;  
    ngx_str_t  response_status;
    ngx_uint_t  response_header_len;
    ngx_uint_t  request_header_len;
    ngx_uint_t data_pos;
    ngx_int_t data_len;            
    ngx_uint_t parse_req_pos;
    ngx_uint_t parse_res_pos;
    void *wan_ta_client_tp;
    http_snooping_url_t *con_url;
    ngx_uint_t req_rec_pkt;
    ngx_uint_t req_snd_pkt;  
    ngx_uint_t res_rec_pkt;
    ngx_uint_t res_snd_pkt;    
    uchar current_request_header_buf[HTTP_SP_REQ_HEAD_LEN_MAX];//URL字符串缓冲区
    uchar current_response_header_buf[HTTP_SP_RES_HEAD_LEN_MAX];//URL字符串缓冲区     
}http_sp_connection_t;

#define HTTP_CACHE_TIME_OUT 20
typedef struct http_snooping_ctx_s {
    ngx_int_t url_max;
    ngx_int_t url_free;
    ushort con_max;
    ushort con_num;
    ushort buf_max;
    ushort buf_num;
    unsigned long con_lock; 
    unsigned long url_lock;
    unsigned long buf_lock; 
    unsigned long url_cache_lock;
    struct list_head url_free_list;
    struct list_head url_use_list;
    struct list_head con_free_list;
    struct list_head con_use_list;
    struct list_head data_buf_free_list;
    struct list_head url_cache_list[HTTP_CACHE_TIME_OUT];
    ushort cache_num;
    ushort cache_pad;
    ngx_log_t *http_sp_log;
    ngx_pool_t *http_sp_url_pool;
    struct list_head url_hash[HTTP_SP_URL_MAX];
    unsigned long url_hash_lock[HTTP_SP_URL_MAX];
    http_snooping_url_t *url_base;
    http_sp_connection_t *con_base;
    http_snooping_buf_t *data_buf_base[2];
}http_snooping_ctx_t;

#define HTTP_MAX_HOST (1024)
#define HTTP_HOMEPAGE_TIMEOUT (10*60*HZ)

#define HS_RECORD_LOCK  (30)
#define HS_HOMEPAGE_SPIN_LOCK(flags) {_local_irqsave(flags); while (test_and_set_bit(HS_RECORD_LOCK, &http_homepage_lock));}
#define HS_HOMEPAGE_SPIN_UNLOCK(flags)  {clear_bit(HS_RECORD_LOCK, &http_homepage_lock); _local_irqrestore(flags);}

#define HS_HOMEPAGE_DATA_SPIN_LOCK() {while (test_and_set_bit(HS_RECORD_LOCK, &http_homepage_lock));}
#define HS_HOMEPAGE_DATA_SPIN_UNLOCK()  {clear_bit(HS_RECORD_LOCK, &http_homepage_lock);}

#define HS_URL_SPIN_LOCK(flags) {_local_irqsave(flags); while (test_and_set_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.url_lock));}
#define HS_URL_SPIN_UNLOCK(flags)  {clear_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.url_lock); _local_irqrestore(flags);}

#define HS_URL_DATA_SPIN_LOCK() {while (test_and_set_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.url_lock));}
#define HS_URL_DATA_SPIN_UNLOCK()  {clear_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.url_lock);}

#define HS_HASH_SPIN_LOCK(key, flags) {_local_irqsave(flags); while (test_and_set_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.url_hash_lock[key]));}
#define HS_HASH_SPIN_UNLOCK(key, flags)  {clear_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.url_hash_lock[key]); _local_irqrestore(flags);}

#define HS_HASH_DATA_SPIN_LOCK(key) {while (test_and_set_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.url_hash_lock[key]));}
#define HS_HASH_DATA_SPIN_UNLOCK(key)  {clear_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.url_hash_lock[key]);}

#define HS_CON_SPIN_LOCK(flags) {_local_irqsave(flags); while (test_and_set_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.con_lock));}
#define HS_CON_SPIN_UNLOCK(flags)  {clear_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.con_lock); _local_irqrestore(flags);}

#define HS_CON_DATA_SPIN_LOCK() {while (test_and_set_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.con_lock));}
#define HS_CON_DATA_SPIN_UNLOCK()  {clear_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.con_lock);}

#define HS_BUF_SPIN_LOCK(flags) {_local_irqsave(flags); while (test_and_set_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.buf_lock));}
#define HS_BUF_SPIN_UNLOCK(flags)  {clear_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.buf_lock); _local_irqrestore(flags);}

#define HS_BUF_DATA_SPIN_LOCK() {while (test_and_set_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.buf_lock));}
#define HS_BUF_DATA_SPIN_UNLOCK()  {clear_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.buf_lock);}

#define HS_URL_BUF_SPIN_LOCK(flags, url) {_local_irqsave(flags); while (test_and_set_bit(HS_RECORD_LOCK, &url->url_data_lock));}
#define HS_URL_BUF_SPIN_UNLOCK(flags, url)  {clear_bit(HS_RECORD_LOCK, &url->url_data_lock); _local_irqrestore(flags);}

#define HS_URL_BUF_DATA_SPIN_LOCK(url) {while (test_and_set_bit(HS_RECORD_LOCK, &url->url_data_lock));}
#define HS_URL_BUF_DATA_SPIN_UNLOCK(url)  {clear_bit(HS_RECORD_LOCK, &url->url_data_lock);}

#define HS_URL_CACHE_SPIN_LOCK(flags) {_local_irqsave(flags); while (test_and_set_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.url_cache_lock));}
#define HS_URL_CACHE_SPIN_UNLOCK(flags)  {clear_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.url_cache_lock); _local_irqrestore(flags);}

#define HS_URL_CACHE_DATA_SPIN_LOCK() {while (test_and_set_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.url_cache_lock));}
#define HS_URL_CACHE_DATA_SPIN_UNLOCK()  {clear_bit(HS_RECORD_LOCK, &g_http_snooping_ctx.url_cache_lock);}

#define ngx_str4cmp(m, c0, c1, c2, c3)                                        \
    (m[0] == c0 && m[1] == c1 && m[2] == c2 && m[3] == c3)
#define ngx_str3_cmp(m, c0, c1, c2, c3)                                       \
    (m[0] == c0 && m[1] == c1 && m[2] == c2)
#define ngx_str5cmp(m, c0, c1, c2, c3, c4)                                    \
    m[0] == c0 && m[1] == c1 && m[2] == c2 && m[3] == c3 && m[4] == c4

void ngx_http_youku_url_add(ngx_str_t *url_name);
void ngx_http_snooping_init(ngx_log_t *log);
//extern long tick;
extern unsigned long time32(void);
extern unsigned long volatile jiffies;

#define HTTP_SP_NGX_SERVER_MAX (4)
#define HTTP_SP2C_PORT (9999)
#define HTTP_C2SP_PORT (10000)
#define HTTP_C2SP_ACTION_GET (1)
#define HTTP_C2SP_ACTION_GETNEXT (2)
#define HTTP_C2SP_ACTION_ADD (3)
#define HTTP_C2SP_ACTION_DELETE (4)
typedef struct http_c2sp_req_pkt_s {
    u32 session_id;
    u8 c2sp_action;
    u8 pad[3];
    u16 url_len;
    u8 usr_data[HTTP_SP_URL_LEN_MAX];
}http_c2sp_req_pkt_t;

typedef struct http_c2sp_res_pkt_s{
    u32 session_id;
    u8 status;
    u8 pad[3];
}http_c2sp_res_pkt_t;

#define HTTP_SP_STATUS_OK 0
#define HTTP_SP_STATUS_DEFAULT_ERROR 1

#define HTTP_SP2C_ACTION_PARSE 1
#define HTTP_SP2C_ACTION_DOWN 2
#define HTTP_SP2C_ACTION_GETNEXT 3
typedef struct http_sp2c_req_pkt_s
{
    u32 session_id;
    u8 sp2c_action;
    u8 pad[3];
    u16 url_len;
    u8 url_data[HTTP_SP_URL_LEN_MAX];
}http_sp2c_req_pkt_t;

typedef struct http_sp2c_res_pkt_s {
    u32 session_id;
    u8 status;
    u8 pad[3];
    u16 url_len;
    u8 url_data[HTTP_SP_URL_LEN_MAX];
}http_sp2c_res_pkt_t;

void ngx_http_snooping_url_forbidden(http_sp_connection_t *con);
void ngx_http_url2local_file(ngx_str_t *url_str, ngx_str_t *local_file, http_cache_url_t *pcache_rule);
http_cache_url_t *ngx_http_url_rule_get(ngx_str_t *url_str);
void ngx_http_snooping_url2local_key(ngx_str_t *url_name, http_cache_url_t *url_cache);
#define HTTPSP_IS_HEX(c) ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))

ngx_int_t ngx_http_sp_url_handle(ngx_str_t *url_name, u8 action);
ngx_int_t ngx_http_server_url_handle(ngx_str_t *url_name, u8 action);
void ngx_http_sp_url_state_setby_name(ngx_str_t *url_name, u8 state);
void ngx_http_sp_url_state_setby_id(ngx_int_t url_id, u8 state);
ngx_int_t ngx_http_sp_urlid_get(ngx_str_t *url_name);
ngx_int_t ngx_http_sp_id2fpath(ngx_int_t url_id, ngx_str_t *pFileBuf);
#define NGX_HTTP_URLID2NODE(id) (g_http_snooping_ctx.url_base + id)
#define NGX_HTTP_URLNODE2ID(node) (node - g_http_snooping_ctx.url_base)
#define NGX_HTTP_URLID_VALID(id) (((id) >=0) && ((id) < HTTP_SP_URL_MAX))
//void ngx_http_sp_url_state_get(ngx_str_t *url_name, u8 *pstate);
#endif /*__HTTPSNOOPING_H__*/

