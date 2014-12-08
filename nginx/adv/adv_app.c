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

#define APP_ADV_DROP_DATA   (0x1<<0)  /* 丢弃收到的数据 */
#define APP_ADV_PROXY_DATA  (0x1<<1)  /* 代理回应收到的数据 */
#define VALID_AD_STR_LEN  (128)
#define BIG_VF_BUF_MAX   (1024 * 512)
char big_vf_buf[BIG_VF_BUF_MAX];
int bif_vf_len;
char response_buf[BIG_VF_BUF_MAX];

char vf_response[] = 
    "HTTP/1.1 200 OK\r\n"
    "Server: adserver\r\n"
    "Cache-Control: private\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: private\r\n"
    "Expires: 0\r\n"
    //"Content-Encoding: gzip\r\n"
    "Connection: close\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "Date: Fri, 04 Apr 2014 01:39:59 GMT\r\n"
    "Content-Length: ";

char youku_m3u8_response_1[] = 
    "HTTP/1.1 206 Partial Content\r\n"
    "Server: nginx\r\n"
    "Date: Tue, 22 Apr 2014 03:16:11 GMT\r\n"
    "Content-Type: application/vnd.apple.mpegurl\r\n"
    "Connection: keep-alive\r\n"
    "X-Powered-By: PHP/5.2.17\r\n"
    "Accept-Ranges: bytes\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "Content-Range: bytes ";

char youku_m3u8_response_2[] = "Content-Length: "; 

char get_s_response[] = 
    "HTTP/1.1 200 OK\r\n"
    "Server: nginx/1.2.1\r\n"
    "Date: Thu, 17 Apr 2014 02:21:40 GMT\r\n"
    "Vary: Accept-Encoding\r\n"
    "Cache-Control: no-cache,no-store\r\n"
    "Expires: -1\r\n"
    "Connection: keep-alive\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: ";

char get_m_response[] = 
    "HTTP/1.1 200 OK\r\n"
    "Server: nginx/1.2.1\r\n"
    "Date: Thu, 17 Apr 2014 02:21:40 GMT\r\n"
    "Vary: Accept-Encoding\r\n"
    "Cache-Control: no-cache,no-store\r\n"
    "Expires: -1\r\n"
    "Connection: keep-alive\r\n"
    "Content-Type: text/xml; charset=UTF-8\r\n"
    "Content-Length: ";
    
#define ngx_str8cmp_vf(m, c0, c1, c2, c3, c4, c5, c6, c7)                        \
    m[0] == c0 && m[1] == c1 && m[2] == c2 && m[3] == c3                      \
        && m[4] == c4 && m[5] == c5 && m[6] == c6 && m[7] == c7

#define ngx_str9cmp_vf(m, c0, c1, c2, c3, c4, c5, c6, c7, c8)                        \
    m[0] == c0 && m[1] == c1 && m[2] == c2 && m[3] == c3                      \
        && m[4] == c4 && m[5] == c5 && m[6] == c6 && m[7] == c7 && m[8] == c8

#define ngx_str10cmp_vf(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9)                        \
    m[0] == c0 && m[1] == c1 && m[2] == c2 && m[3] == c3                      \
        && m[4] == c4 && m[5] == c5 && m[6] == c6 && m[7] == c7 && m[8] == c8 && m[9] == c9

#define ngx_str11cmp_vf(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10)                        \
    m[0] == c0 && m[1] == c1 && m[2] == c2 && m[3] == c3                      \
        && m[4] == c4 && m[5] == c5 && m[6] == c6 && m[7] == c7 && m[8] == c8  && m[9] == c9  \
        && m[10] == c10

#define ngx_str12cmp_vf(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11)                        \
    m[0] == c0 && m[1] == c1 && m[2] == c2 && m[3] == c3                      \
        && m[4] == c4 && m[5] == c5 && m[6] == c6 && m[7] == c7 && m[8] == c8  && m[9] == c9  \
        && m[10] == c10 && m[11] == c11
        
#define ngx_str15cmp_vf(m, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14)                        \
    m[0] == c0 && m[1] == c1 && m[2] == c2 && m[3] == c3                      \
        && m[4] == c4 && m[5] == c5 && m[6] == c6 && m[7] == c7 && m[8] == c8  && m[9] == c9  \
        && m[10] == c10 && m[11] == c11  && m[12] == c12 && m[13] == c13 && m[14] == c14

int  ngx_http_make_vf_response(char * resp)
{
    int len;
    int i;
    i = sizeof(vf_response) -1;
    len = 0;
    memcpy(resp, vf_response, i);
    resp += i;
    len += i;
    i = sprintf(resp, "%d\r\n\r\n", bif_vf_len);
    printk_rt("i:%d, %s - %d\n", i, __FILE__, __LINE__);    
    resp += i;
    len += i;
    printk_rt("copy adv txt:%d\n", bif_vf_len);
    memcpy(resp, big_vf_buf, bif_vf_len);
    len += bif_vf_len;
    return len;
}

int  ngx_http_make_youku_m3u8_response(char * resp, int is_range)
{
    int len;
    int i;

    i = sizeof(youku_m3u8_response_1) -1;
    len = 0;
    memcpy(resp, youku_m3u8_response_1, i);
    resp += i;
    len += i;
    if (is_range) {
        i = sprintf(resp, "0-1/%d\r\n", bif_vf_len);
        resp += i;
        len += i;
        i = sprintf(resp, "%s%d\r\n\r\n#E", youku_m3u8_response_2, 2);
        resp += i;
        len += i;
        return len;        
    }

    i = sprintf(resp, "0-%d/%d\r\n", bif_vf_len - 1 , bif_vf_len);
    resp += i;
    len += i;
    i = sprintf(resp, "%s%d\r\n\r\n", youku_m3u8_response_2, bif_vf_len);
    resp += i;
    len += i;

    memcpy(resp, big_vf_buf, bif_vf_len);
    len += bif_vf_len;    
    return len;
}


int  ngx_http_make_get_s_response(char * resp)
{
    int len;
    int i;
    i = sizeof(get_s_response) -1;
    len = 0;
    memcpy(resp, get_s_response, i);
    resp += i;
    len += i;
    i = sprintf(resp, "%d\r\n\r\n", bif_vf_len);
    printk_rt("i:%d, %s - %d\n", i, __FILE__, __LINE__);    
    resp += i;
    len += i;
    printk_rt("copy vf txt:%d\n", bif_vf_len);
    memcpy(resp, big_vf_buf, bif_vf_len);
    len += bif_vf_len;
    return len;
}


int  ngx_http_make_get_m_response(char * resp)
{
    int len;
    int i;
    i = sizeof(get_m_response) -1;
    len = 0;
    memcpy(resp, get_m_response, i);
    resp += i;
    len += i;
    i = sprintf(resp, "%d\r\n\r\n", bif_vf_len);
    printk_rt("i:%d, %s - %d\n", i, __FILE__, __LINE__);    
    resp += i;
    len += i;
    printk_rt("copy vf txt:%d\n", bif_vf_len);
    memcpy(resp, big_vf_buf, bif_vf_len);
    len += bif_vf_len;
    return len;
}

int ngx_http_make_get_dc_amdt_response(char * resp, char * old, int old_ad_len)
{
    char *str_first_other;
    char *str_last_other, *str, *prv_str;
    char * ad_start, *ad_end;
    int lf_cnt;
    int len;
    int i;
  //  i = sizeof(get_m_response) -1;
    len = 0;
   // memcpy(resp, get_m_response, i);
   // resp += i;
   // len += i;
   
    //printf("old:%p\n", old);
    str = strstr(old, "videos/other/");

    if (str == NULL) {
        printk_rt("no adv..............................\n");
        return;
    }
    ad_start = strstr(old, "#EXTINF");
    str_first_other = NULL;
    str_last_other = NULL;
    
    str += 13;
    prv_str = str;
    while(str) {
        str = strstr(prv_str, "videos/other/");
        //printf("str:%p\n", str);
        if (str == NULL) {
            break;
        }
        str += 13;
        prv_str = str;
    }
    ad_end = strstr(prv_str, "#EXTINF");


    printk_rt("old:%p, %c, offset:%d, %c\n", old, old[0], ad_start - old, ad_start[0]);
    memcpy(resp, old, ad_start - old);
    i = ad_start - old;
    resp += i;
    len += i;
    printk_rt("old:%p, %c, offset:%d, %c\n", old, old[0], old_ad_len - (ad_end - old), ad_end[0]);
    memcpy(resp, ad_end, old_ad_len - (ad_end - old));
    len += old_ad_len - (ad_end - old);
    return len;
}

char adv_str[100]={0};

void ngx_http_vf_deal1(tcp_proxy_cb_t *tp_entry)
{
    tcp_proxy_cb_t *tp_cb;
    ngx_uint_t token;
    ef_buf_t *efb, *efb_nxt;
    tcp_app_data_t http_data;
    char * reg_buf;
    char tem_buf[64];
    int i, rsp_len;
    char * str;
    tp_cb = tp_entry;
    token = tp_cb->tp_socket->write_queue_free_space(tp_cb->pair->sk);
    efb = tp_cb->tp_socket->recvmsg(tp_cb->sk, token);

    if (efb == NULL) {
        return;
    }

    if (tp_cb->app_flags == APP_ADV_PROXY_DATA) { /* 处理iqiyi服务器回应的m3u8 */
        while(efb) {
            efb_nxt = efb->nextPkt;
            tcp_data_get(&http_data, efb);
            memcpy(big_vf_buf + bif_vf_len, http_data.buf, http_data.buf_len);
            bif_vf_len += http_data.buf_len;
            printk_rt("nsock(%d),get str:\n",tp_cb->session_id);
            str = strstr(big_vf_buf, "videos/v");
            printk_rt("nsock(%d),str is :[%p, %p, %d]\n",tp_cb->session_id,
                big_vf_buf, str, str ? (str - big_vf_buf) : -1);
            r_ef_buf_free(efb); 
            efb = efb_nxt;
            if (str) {
                memset(response_buf, 0 , sizeof(response_buf));
                rsp_len =  ngx_http_make_get_dc_amdt_response(response_buf, big_vf_buf, bif_vf_len);
                printk_rt("\n\nsock(%d),send vf_response,%d ytes\n",tp_cb->session_id,rsp_len);

                i = wan_ta_app_send_byte(tp_cb->pair, response_buf, rsp_len, WAN_TA_SEND_ALL);
                printk_rt("\nsock(%d),send vf:%d bytes\n", tp_cb->session_id,i);
                tp_cb->app_flags = 0;
                tp_cb->pair->app_flags = APP_ADV_DROP_DATA;
                goto send;
            }
        }
        
        return;
    }
    
    
    tcp_data_get(&http_data, efb);

    reg_buf = http_data.buf;

    if (http_data.buf_len > 8) {
        /* adv_str 模式 */
        if (adv_str[0] && strstr(reg_buf,adv_str)) {
#if 1       
            printk_rt("adv_str is :%s\n", adv_str);
            memset(tem_buf, 0 , sizeof(tem_buf));
            snprintf(tem_buf,30, "%s",reg_buf);
            printk_rt("sock(#%d), get_adv request:%s\n",tp_cb->session_id, tem_buf);
            
            memset(response_buf, 0 , sizeof(response_buf));
            rsp_len =  ngx_http_make_vf_response(response_buf);
            printk_rt("\n\nsock(%d),send vf_response,%d ytes\n",tp_cb->session_id,rsp_len);

            i = wan_ta_app_send_byte(tp_cb, response_buf, rsp_len, WAN_TA_SEND_ALL);
            printk_rt("\nsock(%d),send vf:%d bytes\n", tp_cb->session_id,i);            
#endif            
            tp_cb->app_flags = APP_ADV_DROP_DATA;
            tp_cb->pair->app_flags = APP_ADV_DROP_DATA;
            goto free_and_out;
        }
        
        if (adv_str[0]) {
             goto finish_ad;
        }
        /* youku */
        if (ngx_str8cmp_vf(reg_buf,'G', 'E', 'T', ' ', '/', 'v', 'f', '?')||
            ngx_str9cmp_vf(reg_buf,'G', 'E', 'T', ' ', '/', 'a', 'd','v', '?')) {
           // wan_ta_write_error(tp_cb->sk);
#if 1           
            memset(tem_buf, 0 , sizeof(tem_buf));
            snprintf(tem_buf,10, "%s",reg_buf);
            printk_rt("sock(#%d), get_vf,reg:%s\n",tp_cb->session_id, tem_buf);

            memset(response_buf, 0 , sizeof(response_buf));
            rsp_len =  ngx_http_make_vf_response(response_buf);
            printk_rt("\n\nsock(%d),send vf_response,%d ytes\n",tp_cb->session_id,rsp_len);

            i = wan_ta_app_send_byte(tp_cb, response_buf, rsp_len, WAN_TA_SEND_ALL);
            printk_rt("\nsock(%d),send vf:%d bytes\n", tp_cb->session_id,i);
#endif            
            tp_cb->app_flags = APP_ADV_DROP_DATA;
            tp_cb->pair->app_flags = APP_ADV_DROP_DATA;
            goto free_and_out;
        }

 /* youku apple http://pl.youku.com*/
        if (strstr(reg_buf,"playlist/m3u8?keyframe=0")) {
            //wan_ta_write_error(tp_cb->sk);
          
            memset(tem_buf, 0 , sizeof(tem_buf));
            snprintf(tem_buf,32, "%s",reg_buf);
            printk_rt("sock(#%d), get_vf,reg:%s\n",tp_cb->session_id, tem_buf);
            
#if 1
            memset(response_buf, 0 , sizeof(response_buf));

            rsp_len =  ngx_http_make_youku_m3u8_response(response_buf, strstr(reg_buf ,"Range: bytes=0-1\r\n"));
            printk_rt("\n\nsock(%d),send vf_response,%d ytes\n",tp_cb->session_id,rsp_len);

            i = wan_ta_app_send_byte(tp_cb, response_buf, rsp_len, WAN_TA_SEND_ALL);
            printk_rt("\nsock(%d),send vf:%d bytes\n", tp_cb->session_id,i);
#endif            
            tp_cb->app_flags = 0;
            tp_cb->pair->app_flags = APP_ADV_DROP_DATA;
            goto free_and_out;
        }        

        /* letv */
        if (ngx_str15cmp_vf(reg_buf,'G', 'E', 'T', ' ', '/', 's', '?', 'a' ,'r', 'k', '=', '2','1','3','&')
            || ngx_str15cmp_vf(reg_buf,'G', 'E', 'T', ' ', '/', 's', '?', 'a' ,'r', 'k', '=', '2','3','2','&')) {
           // wan_ta_write_error(tp_cb->sk);
#if 1           
            memset(tem_buf, 0 , sizeof(tem_buf));
            snprintf(tem_buf,24, "%s",reg_buf);
            printk_rt("sock(#%d), get_vf,reg:%s\n",tp_cb->session_id, tem_buf);

            memset(response_buf, 0 , sizeof(response_buf));
            rsp_len =  ngx_http_make_get_s_response(response_buf);
            printk_rt("\n\nsock(%d),send vf_response,%d ytes\n",tp_cb->session_id,rsp_len);

            i = wan_ta_app_send_byte(tp_cb, response_buf, rsp_len, WAN_TA_SEND_ALL);
            printk_rt("\nsock(%d),send vf:%d bytes\n", tp_cb->session_id,i);
#endif            
            tp_cb->app_flags = APP_ADV_DROP_DATA;
            tp_cb->pair->app_flags = APP_ADV_DROP_DATA;
            goto free_and_out;
        }

        if (ngx_str11cmp_vf(reg_buf,'G', 'E', 'T', ' ', '/', 't', '?', 'm' ,'i', 'd', '=')) {
            tp_cb->app_flags = 0;
            tp_cb->pair->app_flags = 0;            
        }
        /* sohu */
        if (ngx_str10cmp_vf(reg_buf,'G', 'E', 'T', ' ', '/', 'm', '?', 'd' ,'u', '=')) {
           // wan_ta_write_error(tp_cb->sk);
#if 1           
            memset(tem_buf, 0 , sizeof(tem_buf));
            snprintf(tem_buf,24, "%s",reg_buf);
            printk_rt("sock(#%d), get_vf,reg:%s\n",tp_cb->session_id, tem_buf);

            memset(response_buf, 0 , sizeof(response_buf));
            rsp_len =  ngx_http_make_get_s_response(response_buf);
            printk_rt("\n\nsock(%d),send vf_response,%d ytes\n",tp_cb->session_id,rsp_len);

            i = wan_ta_app_send_byte(tp_cb, response_buf, rsp_len, WAN_TA_SEND_ALL);
            printk_rt("\nsock(%d),send vf:%d bytes\n", tp_cb->session_id,i);
#endif            
            tp_cb->app_flags = APP_ADV_DROP_DATA;
            tp_cb->pair->app_flags = APP_ADV_PROXY_DATA;
            goto free_and_out;
        }

        /* sohu apple /m?pt=oad*/
        if (ngx_str10cmp_vf(reg_buf,'G', 'E', 'T', ' ', '/', 'm', '?', 'p' ,'t', '=')) {
           // wan_ta_write_error(tp_cb->sk);
#if 1           
            memset(tem_buf, 0 , sizeof(tem_buf));
            snprintf(tem_buf,24, "%s",reg_buf);
            printk_rt("sock(#%d), get_vf,reg:%s\n",tp_cb->session_id, tem_buf);

            memset(response_buf, 0 , sizeof(response_buf));
            rsp_len =  ngx_http_make_get_s_response(response_buf);
            printk_rt("\n\nsock(%d),send vf_response,%d ytes\n",tp_cb->session_id,rsp_len);

            i = wan_ta_app_send_byte(tp_cb, response_buf, rsp_len, WAN_TA_SEND_ALL);
            printk_rt("\nsock(%d),send vf:%d bytes\n", tp_cb->session_id,i);
#endif            
            tp_cb->app_flags = APP_ADV_DROP_DATA;
            tp_cb->pair->app_flags = APP_ADV_PROXY_DATA;
            goto free_and_out;
        }        

#if 0        
        /* iqiyi  GET /dc/amdt*/
        if (ngx_str12cmp_vf(reg_buf,'G', 'E', 'T', ' ', '/', 'd', 'c', '/' ,'a', 'm', 'd', 't')) {
#if 0           
            memset(tem_buf, 0 , sizeof(tem_buf));
            snprintf(tem_buf,24, "%s",reg_buf);
            printk_rt("sock(#%d), get_vf,reg:%s\n",tp_cb->session_id, tem_buf);
            
            memset(big_vf_buf, 0,409600);
            bif_vf_len = 0;
            //tp_cb->app_flags = 1;
            tp_cb->pair->app_flags = 2; /* 接收服务器响应的m3u8 */
            goto send;
#endif       
#if 1           
            memset(tem_buf, 0 , sizeof(tem_buf));
            snprintf(tem_buf,24, "%s",reg_buf);
            printk_rt("sock(#%d), get_vf,reg:%s\n",tp_cb->session_id, tem_buf);

            memset(response_buf, 0 , sizeof(response_buf));
            rsp_len =  ngx_http_make_get_s_response(response_buf);
            printk_rt("\n\nsock(%d),send vf_response,%d ytes\n",tp_cb->session_id,rsp_len);

            i = wan_ta_app_send_byte(tp_cb, response_buf, rsp_len, WAN_TA_SEND_ALL);
            printk_rt("\nsock(%d),send vf:%d bytes\n", tp_cb->session_id,i);
#endif            
            tp_cb->app_flags = 1;
            tp_cb->pair->app_flags = 1;
            goto free_and_out;
        }   

        /* iqiyi  POST /show2*/
        if (ngx_str11cmp_vf(reg_buf,'P', 'O', 'S', 'T',  ' ', '/', 's', 'h', 'o' ,'w', '2')) {
#if 0           
            memset(tem_buf, 0 , sizeof(tem_buf));
            snprintf(tem_buf,24, "%s",reg_buf);
            printk_rt("sock(#%d), get_vf,reg:%s\n",tp_cb->session_id, tem_buf);
            
            memset(big_vf_buf, 0,BIG_VF_BUF_MAX);
            bif_vf_len = 0;
            //tp_cb->app_flags = 1;
            tp_cb->pair->app_flags = 2; /* 接收服务器响应的m3u8 */
            goto send;
#endif       
#if 1           
            memset(tem_buf, 0 , sizeof(tem_buf));
            snprintf(tem_buf,24, "%s",reg_buf);
            printk_rt("sock(#%d), get_vf,reg:%s\n",tp_cb->session_id, tem_buf);

            memset(response_buf, 0 , sizeof(response_buf));
            rsp_len =  ngx_http_make_get_s_response(response_buf);
            printk_rt("\n\nsock(%d),send vf_response,%d ytes\n",tp_cb->session_id,rsp_len);

            i = wan_ta_app_send_byte(tp_cb, response_buf, rsp_len, WAN_TA_SEND_ALL);
            printk_rt("\nsock(%d),send vf:%d bytes\n", tp_cb->session_id,i);
#endif            
            tp_cb->app_flags = 1;
            tp_cb->pair->app_flags = 2;
            goto free_and_out;
        }   

        /* POST /php/xyz/i*/
        if (ngx_str15cmp_vf(reg_buf,'P', 'O', 'S', 'T',  ' ', '/', 'p', 'h', 'p' ,'/', 'x', 'y', 'z', '/', 'i')) {
            memset(tem_buf, 0 , sizeof(tem_buf));
            snprintf(tem_buf,24, "%s",reg_buf);
            printk_rt("sock(#%d), get_vf,reg:%s\n",tp_cb->session_id, tem_buf);            
            wan_ta_write_error(tp_cb->sk);
            goto free_and_out;
        }

#endif

        /* iqiyi adnord GET /php/xyz/entry/nebula */
        if (strstr(reg_buf, "php/xyz/entry/nebula")) {
           // wan_ta_write_error(tp_cb->sk);
#if 1           
            memset(tem_buf, 0 , sizeof(tem_buf));
            snprintf(tem_buf,30, "%s",reg_buf);
            printk_rt("sock(#%d), get_vf,reg:%s\n",tp_cb->session_id, tem_buf);

            memset(response_buf, 0 , sizeof(response_buf));
            rsp_len =  ngx_http_make_get_s_response(response_buf);
            printk_rt("\n\nsock(%d),send vf_response,%d ytes\n",tp_cb->session_id,rsp_len);

            i = wan_ta_app_send_byte(tp_cb, response_buf, rsp_len, WAN_TA_SEND_ALL);
            printk_rt("\nsock(%d),send vf:%d bytes\n", tp_cb->session_id,i);
#endif            
            tp_cb->app_flags = APP_ADV_DROP_DATA;
            tp_cb->pair->app_flags = APP_ADV_PROXY_DATA;
            goto free_and_out;
        } 

        
    }
    
finish_ad: 
    if (tp_cb->app_flags == APP_ADV_DROP_DATA) {
        printk_rt("\nsock(%d), deal vf\n", tp_cb->session_id);
        //wan_ta_show_session_dbg_detail(tp_cb->session_id);
free_and_out:
        r_ef_buf_free(efb);
        return;
    }
    
    
send:    
    tp_cb->tp_socket->sendmsg(tp_cb->pair->sk, efb);
}



void ngx_http_vf_deal(tcp_proxy_cb_t *tp_entry)
{
    tcp_proxy_cb_t *tp_cb;
    ngx_uint_t token;
    ef_buf_t *efb, *efb_nxt;
    tcp_app_data_t http_data;
    char * reg_buf;
    char tem_buf[64];
    char * ad_str = "ad_str\":\"{}";
    int i, rsp_len,ad_len, move_len;;
    char * str;
    tp_cb = tp_entry;
    token = tp_cb->tp_socket->write_queue_free_space(tp_cb->pair->sk);
    if (tp_cb->event_bits & TP_EVENT_SHUTDOWN) {
        token = 200;
    }
    efb = tp_cb->tp_socket->recvmsg(tp_cb->sk, token);

    if (efb == NULL && !(tp_cb->event_bits & TP_EVENT_SHUTDOWN)) {
        return;
    }
    
    tcp_data_get(&http_data, efb);

    reg_buf = http_data.buf;
#if 0
    if (http_data.buf_len > 8) {
        if (adv_str != NULL  && strstr(reg_buf,adv_str)) {
#if 1       
            printk_rt("adv_str is :%s\n", adv_str);
            memset(tem_buf, 0 , sizeof(tem_buf));
            snprintf(tem_buf,30, "%s",reg_buf);
            printk_rt("sock(#%d), get_adv request:%s\n",tp_cb->session_id, tem_buf);

            memset(response_buf, 0 , sizeof(response_buf));
            rsp_len =  ngx_http_make_vf_response(response_buf);
            printk_rt("\n\nsock(%d),send adv_response,%d ytes\n",tp_cb->session_id,rsp_len);

            i = wan_ta_app_send_byte(tp_cb, response_buf, rsp_len, WAN_TA_SEND_ALL);
            printk_rt("\nsock(%d),send adv:%d bytes\n", tp_cb->session_id,i);
#endif            
            tp_cb->app_flags = 1;
            tp_cb->pair->app_flags = 1;
            goto free_and_out;
        }

    }
#endif


#if 1
    if (tp_cb->app_flags == APP_ADV_PROXY_DATA) { /* 处理iqiyi服务器回应的视频综合信息，包含广告综合信息 */
        char * str_adv_str, *str_adv_str_n;
        while(efb) {
            efb_nxt = efb->nextPkt;
            tcp_data_get(&http_data, efb);
            memcpy(big_vf_buf + bif_vf_len, http_data.buf, http_data.buf_len);
            bif_vf_len += http_data.buf_len;
            r_ef_buf_free(efb); 
            efb = efb_nxt;
        }
        str_adv_str = strstr(big_vf_buf, "ad_str");
        if (str_adv_str) {
            str_adv_str_n = strstr(str_adv_str,"\\n");
            if (str_adv_str_n) {
                if (str_adv_str_n - str_adv_str > VALID_AD_STR_LEN) {/* 只对有广告信息的做屏蔽 */
                    printk_rt("sock(#%d),deal valid ad_str\n",tp_cb->pair->session_id);
                    memcpy(str_adv_str, ad_str,11);
                    move_len = &big_vf_buf[bif_vf_len - 1] - str_adv_str_n + 1;
                    memmove(str_adv_str + 11, str_adv_str_n,move_len);
                    bif_vf_len = bif_vf_len - (str_adv_str_n - str_adv_str) + 11;
                } else {
                    printk_rt("sock(#%d),deal invalid ad_str\n",tp_cb->pair->session_id);
                }
                printk_rt("sock(#%d), send %d data\n", tp_cb->pair->session_id, bif_vf_len);
                i = wan_ta_app_send_byte(tp_cb->pair, big_vf_buf, bif_vf_len, WAN_TA_SEND_ALL);
                printk_rt("sock(#%d), send %d data success\n", tp_cb->pair->session_id, i);
                tp_cb->app_flags = 0;
                bif_vf_len = 0;
                
            }
        }

        if ((tp_cb->event_bits & TP_EVENT_SHUTDOWN) && bif_vf_len != 0) {
            printk_rt("sock(#%d),have no ad_str\n",tp_cb->pair->session_id);
            wan_ta_app_send_byte(tp_cb->pair, big_vf_buf, bif_vf_len, WAN_TA_SEND_ALL);
            tp_cb->app_flags = 0;
            bif_vf_len = 0;
        }
        
        return;
    }

    
    if (http_data.buf_len > 8) {
        if (adv_str != NULL  && strstr(reg_buf,adv_str)) {
#if 1       
            printk_rt("adv_str is :%s\n", adv_str);
            memset(tem_buf, 0 , sizeof(tem_buf));
            snprintf(tem_buf,30, "%s",reg_buf);
            printk_rt("sock(#%d), get_adv request:%s\n",tp_cb->session_id, tem_buf);
#endif            
            memset(big_vf_buf, 0,BIG_VF_BUF_MAX);
            bif_vf_len = 0;
            //tp_cb->app_flags = 1;
            tp_cb->pair->app_flags = APP_ADV_PROXY_DATA;
            goto send;
        }

    }
#endif    
    if (tp_cb->app_flags == APP_ADV_DROP_DATA) {
        printk_rt("\nsock(%d), deal adv\n", tp_cb->session_id);
free_and_out:
        r_ef_buf_free(efb);
        return;
    }
    
    
send:    
    tp_cb->tp_socket->sendmsg(tp_cb->pair->sk, efb);
}

#include <sys/support.h>

void make_vf_res_buf(char * file_name)
{
    int fd;
    int i;
    int j = 0;
    printf("file_name:%s\n", file_name);
    fd = ngx_open_file(file_name, NGX_FILE_RDONLY, NGX_FILE_OPEN, 0);
    if (fd == -1) {
        printf("open failed\n");
        return;
    }
    printf("fd is %d\n", fd);
    memset(big_vf_buf, 0,409600);
    bif_vf_len =read(fd, big_vf_buf, 409600);
    printf("read %d:\n",bif_vf_len);

#if 0    
    for(i = 0; i < bif_vf_len; i++) {
        printf("%c", big_vf_buf[i]);
    }
#endif
    close(fd);
}

void set_adv_str(char * adv)
{  
    memset(adv_str,0 , sizeof(adv_str));
    memcpy(adv_str, adv, strlen(adv));
    printf("adv_str is :%s\n", adv_str);
}

void show_adv_response()
{
    int i;
    for(i = 0; i < bif_vf_len; i++) {
        printf("%c", big_vf_buf[i]);
    }
}
DEFINE_DEBUG_FUNC(show_adv_response, show_adv_response);
DEFINE_DEBUG_FUNC_WITH_ARG(make_vf_res_buf, make_vf_res_buf, SUPPORT_FUNC_STRING);
DEFINE_DEBUG_FUNC_WITH_ARG(set_adv_str, set_adv_str, SUPPORT_FUNC_STRING);
