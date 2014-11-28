#ifndef __HTTP_DOWNLOAD_H__
#define __HTTP_DOWNLOAD_H__

#include <sys/time.h>
#include <sys/list.h>

#define HTTP_DL_CTRL_PORT_DEFAULT   10001

#define HTTP_DL_BUF_LEN             128
#define HTTP_DL_HOST_LEN            HTTP_DL_BUF_LEN
#define HTTP_DL_PATH_LEN            (HTTP_DL_BUF_LEN * 4)
#define HTTP_DL_URL_LEN             (HTTP_DL_HOST_LEN + HTTP_DL_PATH_LEN)
#define HTTP_DL_LOCAL_LEN           HTTP_DL_BUF_LEN
#define HTTP_DL_READBUF_LEN         4096

#define HTTP_DL_SELECT_TIMEOUT      3   /* ��λ�� */
#define HTTP_DL_TIMEOUT_RETRIES     9   /* ��info��select����9�γ�ʱ����δwrite��read����finish */
#define HTTP_DL_TIMEOUT_DURATION    (HTTP_DL_SELECT_TIMEOUT * HTTP_DL_TIMEOUT_RETRIES)

typedef enum http_dl_stage_e {
    HTTP_DL_STAGE_INIT = 0,         /* ����������ʱ��ʼ״̬ */
    HTTP_DL_STAGE_SEND_REQUEST,     /* �����������󵽷������������ӳɹ��������� */
    HTTP_DL_STAGE_PARSE_STATUS_LINE,/* ����״̬�� */
    HTTP_DL_STAGE_PARSE_HEADER,     /* ����ͷ�� */
    HTTP_DL_STAGE_RECV_CONTENT,     /* ���հ������� */
    HTTP_DL_STAGE_FINISH,           /* ������ɣ�����쳣��ɣ��򽫴�����Ϣ��¼��err_msg�� */
} http_dl_stage_t;

#define HTTP_DL_F_GENUINE_AGENT 0x00000001UL
#define HTTP_DL_F_RESTART_FILE  0x00000002UL

typedef struct http_dl_info_s {
    http_dl_stage_t stage;
    unsigned long flags;

    struct list_head list;

    char url[HTTP_DL_URL_LEN];      /* Unchanged URL */
    char host[HTTP_DL_HOST_LEN];    /* Extracted hostname */
    char path[HTTP_DL_PATH_LEN];    /* Path, as well as dir and file (properly decoded) */
    char local[HTTP_DL_LOCAL_LEN];  /* The local filename of the URL document */
    unsigned short port;

    int sockfd;
    int filefd;

    char buf[HTTP_DL_READBUF_LEN];
    void *dont_touch;               /* ����buf_tailָ��buf����һ������λ�ã�
                                     * ��buf����ʱ��buf_tail�㳬��buf��ָ��dont_touch��
                                     * ��ˣ�dont_touch�𵽻�������ã���ʵ������.
                                     */
    char *buf_data;
    char *buf_tail;

    long recv_len;                  /* ���ղ��ɹ�write��file�е����ݳ��� */
    long content_len;               /* ����http�Ự���͵����ݳ��ȣ�ע����total_len���� */
    long restart_len;               /* �ϵ������У���ʼ���յ�λ�ã�Ŀǰ��֧��range��ʽ */
    long total_len;                 /* �����ļ�����ʵ���� */
    int status_code;                /* HTTP�Ự״̬�룬���HTTP�Ự�����⣬��Ϊ-1 */

    char err_msg[HTTP_DL_BUF_LEN];  /* HTTP�Ự����ʱ�İ�����Ϣ��XXX TODO:������ֱ��sprintf�����ֹ��� */

    struct timeval start_time;      /* ����Content��ʼ��ʱ�� */
    unsigned long elapsed_time;     /* ����Content��ʱ����λ: ���� */

    unsigned int timeout_times;     /* info��select������timeout_times����δ��event���� */
} http_dl_info_t;

typedef struct http_dl_list_s {
    char name[HTTP_DL_BUF_LEN];
    struct list_head list;
    int count;
} http_dl_list_t;

typedef struct http_dl_range_s {
    long first_byte_pos;
    long last_byte_pos;
    long entity_length;
} http_dl_range_t;

typedef enum http_dl_err_e {
    HTTP_DL_OK = 0,
    HTTP_DL_ERR_INVALID = 1,        /* 0 stands for success, so errors begin with 1 */
    HTTP_DL_ERR_INTERNAL,
    HTTP_DL_ERR_SOCK,
    HTTP_DL_ERR_CONN,
    HTTP_DL_ERR_FOPEN,
    HTTP_DL_ERR_FSYNC,
    HTTP_DL_ERR_WRITE,
    HTTP_DL_ERR_READ,
    HTTP_DL_ERR_EOF,
    HTTP_DL_ERR_RESOURCE,
    HTTP_DL_ERR_AGAIN,
    HTTP_DL_ERR_NOTFOUND,
    HTTP_DL_ERR_SELECT,
    HTTP_DL_ERR_HTTP,
} http_dl_err_t;

#define HTTP_URL_PREFIX    "http://"
#define HTTP_URL_PRE_LEN    7       /* strlen("http://") */

#define HTTP_ACCEPT "*/*"
/* HTTP/1.0 status codes from RFC1945, provided for reference.  */
/* Successful 2xx.  */
#define HTTP_STATUS_OK			        200
#define HTTP_STATUS_CREATED		        201
#define HTTP_STATUS_ACCEPTED		    202
#define HTTP_STATUS_NO_CONTENT		    204
#define HTTP_STATUS_PARTIAL_CONTENTS	206

/* Redirection 3xx.  */
#define HTTP_STATUS_MULTIPLE_CHOICES	300
#define HTTP_STATUS_MOVED_PERMANENTLY	301
#define HTTP_STATUS_MOVED_TEMPORARILY	302
#define HTTP_STATUS_NOT_MODIFIED	    304

/* Client error 4xx.  */
#define HTTP_STATUS_BAD_REQUEST		    400
#define HTTP_STATUS_UNAUTHORIZED	    401
#define HTTP_STATUS_FORBIDDEN		    403
#define HTTP_STATUS_NOT_FOUND		    404

/* Server errors 5xx.  */
#define HTTP_STATUS_INTERNAL		    500
#define HTTP_STATUS_NOT_IMPLEMENTED	    501
#define HTTP_STATUS_BAD_GATEWAY		    502
#define HTTP_STATUS_UNAVAILABLE		    503
#define H_20X(x)        (((x) >= 200) && ((x) < 300))
#define H_PARTIAL(x)    ((x) == HTTP_STATUS_PARTIAL_CONTENTS)
#define H_REDIRECTED(x) (((x) == HTTP_STATUS_MOVED_PERMANENTLY)	\
			 || ((x) == HTTP_STATUS_MOVED_TEMPORARILY))
			 
/* The smaller value of the two.  */
#define MINVAL(x, y) ((x) < (y) ? (x) : (y))

#define RBUF_FD(rbuf) ((rbuf)->fd)
#define TEXTHTML_S "text/html"

#define http_dl_free(foo) \
    do {    \
        if (foo) {  \
            free(foo);  \
        }   \
    } while (0)

extern int http_dl_log_level;

#define http_dl_log_info(fmt, arg...) \
    do { \
        if (http_dl_log_level >= 6) { \
            printk("*INFO*  %s: " fmt "\n", __func__, ##arg); \
        } else { \
            printk(fmt "\n", ##arg); \
        } \
    } while (0)

#define http_dl_log_debug(fmt, arg...) \
    do { \
        if (http_dl_log_level >= 7) { \
            printk("*DEBUG* %s[%d]: " fmt "\n", __func__, __LINE__, ##arg); \
        } \
    } while (0)
        
#define http_dl_log_error(fmt, arg...) \
    do { \
        if (http_dl_log_level >= 3) { \
            printk("*ERROR* %s[%d]: " fmt "\n", __func__, __LINE__, ##arg); \
        } \
    } while (0)

#define http_dl_print_raw(fmt, arg...) \
    do { \
        printk(fmt, ##arg); \
    } while(0)

#ifndef malloc
#define	malloc(size)        kmalloc((size), 0)
#endif
#ifndef free
#define	free(ptr)           kfree(ptr)
#endif
#ifndef realloc
#define realloc(ptr, size)  krealloc((ptr), (size), 0)
#endif

#endif /* __HTTP_DOWNLOAD_H__ */

