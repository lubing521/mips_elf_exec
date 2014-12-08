/*
 * Copyright(C) 2014 Ruijie Network. All rights reserved.
 */
/*
 * ngx_sc_util.c
 * Original Author: zhaoyao@ruijie.com.cn, 2014-03-11
 *
 * Snooping Client's common utilities.
 *
 * History:
 *  v1.0    zhaoyao@ruijie.com.cn   2014-03-11
 *          �������ļ���
 *
 */

#include <net/networklayer/inet_lib.h>
#include "ngx_snoop_client.h"

/**
 * NAME: sock_conn_retry
 *
 * DESCRIPTION:
 *      �������ӹ��ܵ��׽������ӡ�
 *
 * @sockfd:     -IN �׽����ļ���������
 * @addr:       -IN �׽��ֵ�ֵַ��
 * @alen:       -IN �׽��ֵ�ַ���ȡ�
 *
 * RETURN: -1��ʾ����ʧ�ܣ�0��ʾ���ӳɹ���
 */
int sock_conn_retry(int sockfd, const struct sockaddr *addr, socklen_t alen)
{
    int nsec;

    if (addr == NULL) {
        return -1;
    }

    for (nsec = 1; nsec <= MAX_SLEEP; nsec <<= 1) {
        if (connect(sockfd, addr, alen) == 0) {
            /* ���ӳɹ���ֱ�ӷ��� */
            return 0;
        }
        if (nsec <= MAX_SLEEP/2) {
            sleep(nsec);
        }
    }

    return -1;
}

/* �ж�host�ֶ��Ƿ�ΪIP��ַ */
static int host_is_ip_addr(const char *host)
{
    int len = 0, dot_cnt = 0, digits = 0, i, num;
    char asc[IP_ADDR_DIGITS_MAX + 1] = {0}; /* ���ǽ�����'\0' */

    if (host == NULL) {
        return 0;
    }

    len = strlen(host);
    if (len > IP_ADDR_STR_MAX_LEN || len < IP_ADDR_STR_MIN_LEN) {
        return 0;
    }

    for (i = 0; i < len; i++) {
        if (host[i] == '.') {
            if (digits > IP_ADDR_DIGITS_MAX || digits < 1) {
                return 0;
            }
            asc[digits] = '\0';
            num = atoi(asc);
            if (num < 0 || num > 255) {
                return 0;
            }
            digits = 0;
            memset(asc, 0, sizeof(asc));
            dot_cnt++;
            if (dot_cnt > IP_ADDR_MAX_DOT_CNT) {
                return 0;
            }
            continue;
        }
        if (!isdigit(host[i])) {
            return 0;
        }
        if (digits >= IP_ADDR_DIGITS_MAX) {
            return 0;
        }
        asc[digits++] = host[i];
    }
    
    if (dot_cnt != IP_ADDR_MAX_DOT_CNT) {
        return 0;
    }
    if (digits > IP_ADDR_DIGITS_MAX || digits < 1) {
        return 0;
    }
    asc[digits] = '\0';
    num = atoi(asc);
    if (num < 0 || num > 255) {
        return 0;
    }

    return 1;
}

/**
 * NAME: host_connect
 *
 * DESCRIPTION:
 *      ���������������������ӡ�
 *
 * @hostname:    -IN ����������IP��ַ��
 *
 * RETURN: -1��ʾ����ʧ�ܣ���������ֵ��ʾ���Ӷ�Ӧ���׽���������ֵ��
 */
int host_connect(const char *hostname)
{
    int sockfd, ret;
    struct sockaddr_in sa;
    socklen_t salen;

    if (hostname == NULL) {
        return -1;
    }

    bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((uint16_t)HTTP_PORT_DEFAULT);
    if (host_is_ip_addr(hostname)) {
        if (inet_aton(hostname, (struct in_addr *)&sa.sin_addr) < 0) {
            return -1;
        }
    } else if (strcmp(hostname, "v.youku.com") == 0) {
        /* ���ڲ�֧�������������ڱ���������ʱʹ�þ�̬�Ľ�����ʽ */
        /*
         * Address: 183.61.116.217
         * Address: 183.61.116.215
         * Address: 183.61.116.218
         * Address: 183.61.116.216
         */
        if (inet_aton("183.61.116.218", (struct in_addr *)&sa.sin_addr) < 0) {
            return -1;
        }
    } else if (strcmp(hostname, "f.youku.com") == 0) {
        /*
         * Address: 183.61.116.54
         * Address: 183.61.116.52
         * Address: 183.61.116.55
         * Address: 183.61.116.53
         * Address: 183.61.116.56
         */
        if (inet_aton("183.61.116.52", (struct in_addr *)&sa.sin_addr) < 0) {
            return -1;
        }
    } else if (strcmp(hostname, "hot.vrs.sohu.com") == 0) {
        /*
         * Address: 220.181.118.55
         * Address: 220.181.118.54
         * Address: 220.181.118.53
         * Address: 220.181.118.60
         */
        if (inet_aton("220.181.118.53", (struct in_addr *)&sa.sin_addr) < 0) {
            return -1;
        }
    } else if (strcmp(hostname, "my.tv.sohu.com") == 0) {
        /*
         * Address: 220.181.90.17
         * Address: 220.181.94.201
         * Address: 220.181.94.203
         * Address: 220.181.94.202
         * Address: 220.181.90.14
         * Address: 220.181.90.12
         * Address: 220.181.94.204
         * Address: 220.181.90.15
         * Address: 220.181.90.19
         * Address: 220.181.90.18
         */
        if (inet_aton("220.181.94.202", (struct in_addr *)&sa.sin_addr) < 0) {
            return -1;
        }
    } else {
        hc_log_error("unknown host %s", hostname);
        return -1;
    }

    salen = sizeof(struct sockaddr_in);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        hc_log_error("Socket failed");
        return -1;
    }

    ret = sock_conn_retry(sockfd, (struct sockaddr *)&sa, salen);
    if (ret < 0) {
        close(sockfd);
        hc_log_error("Connect failed");
        return -1;
    }

    return sockfd;
}

/**
 * NAME: http_parse_status_line
 *
 * DESCRIPTION:
 *      ����HTTPӦ���״̬�����ݡ�
 *
 * @buf:    -IN  HTTPӦ�����ݡ�
 * @len:    -IN  HTTPӦ�𳤶ȡ�
 * @status: -OUT ״̬�롣
 *
 * RETURN: -1��ʾ����ʧ�ܣ���������ֵ��ʾ���Ӷ�Ӧ���׽���������ֵ��
 */
int http_parse_status_line(char *buf, int len, int *status)
{
    char ch;
    char *p;
    int status_digits = 0;
    enum {
        sw_start = 0,
        sw_H,
        sw_HT,
        sw_HTT,
        sw_HTTP,
        sw_first_major_digit,
        sw_major_digit,
        sw_first_minor_digit,
        sw_minor_digit,
        sw_status,
        sw_space_after_status,
        sw_status_text,
        sw_almost_done
    } state;

    if (buf == NULL || status == NULL || len < 0) {
        return -1;
    }

    state = sw_start;
    *status = 0;

    for (p = buf; p < buf + len; p++) {
        ch = *p;

        switch (state) {

        /* "HTTP/" */
        case sw_start:
            switch (ch) {
            case 'H':
                state = sw_H;
                break;
            default:
                return -1;
            }
            break;

        case sw_H:
            switch (ch) {
            case 'T':
                state = sw_HT;
                break;
            default:
                return -1;
            }
            break;

        case sw_HT:
            switch (ch) {
            case 'T':
                state = sw_HTT;
                break;
            default:
                return -1;
            }
            break;

        case sw_HTT:
            switch (ch) {
            case 'P':
                state = sw_HTTP;
                break;
            default:
                return -1;
            }
            break;

        case sw_HTTP:
            switch (ch) {
            case '/':
                state = sw_first_major_digit;
                break;
            default:
                return -1;
            }
            break;

        /* the first digit of major HTTP version */
        case sw_first_major_digit:
            if (ch < '1' || ch > '9') {
                return -1;
            }

            state = sw_major_digit;
            break;

        /* the major HTTP version or dot */
        case sw_major_digit:
            if (ch == '.') {
                state = sw_first_minor_digit;
                break;
            }

            if (ch < '0' || ch > '9') {
                return -1;
            }
            break;

        /* the first digit of minor HTTP version */
        case sw_first_minor_digit:
            if (ch < '0' || ch > '9') {
                return -1;
            }

            state = sw_minor_digit;
            break;

        /* the minor HTTP version or the end of the request line */
        case sw_minor_digit:
            if (ch == ' ') {
                state = sw_status;
                break;
            }

            if (ch < '0' || ch > '9') {
                return -1;
            }
            break;

        /* HTTP status code */
        case sw_status:
            if (ch == ' ') {
                break;
            }

            if (ch < '0' || ch > '9') {
                return -1;
            }

            *status = *status * 10 + ch - '0';
            
            if (++status_digits == 3) {
                state = sw_space_after_status;
            }
            break;

        /* space or end of line */
        case sw_space_after_status:
            switch (ch) {
            case ' ':
                state = sw_status_text;
                break;
            case '.':                    /* IIS may send 403.1, 403.2, etc */
                state = sw_status_text;
                break;
            case CR:
                state = sw_almost_done;
                break;
            case LF:
                goto done;
            default:
                return -1;
            }
            break;

        /* any text until end of line */
        case sw_status_text:
            switch (ch) {
            case CR:
                state = sw_almost_done;

                break;
            case LF:
                goto done;
            }
            break;

        /* end of status line */
        case sw_almost_done:
            switch (ch) {
            case LF:
                goto done;
            default:
                return -1;
            }
        }
    }

    return -1;

done:

    return 0;
}

/**
 * NAME: sc_copy_url
 *
 * DESCRIPTION:
 *      ����url��
 *
 * @url:        -OUT ����url��
 * @o_url:      -IN  ԭʼurl��
 * @len:        -IN  ����url����ռ䳤�ȡ�
 * @with_para:  -IN  0��ʾ�����Ʋ����������ʾ���Ʋ�����
 *
 * RETURN: �޷���ֵ��
 */
void sc_copy_url(char *url, char *o_url, unsigned int len, char with_para)
{
    char *start, *p, *q;
    int cnt = 1; /* ����������'\0' */

    if (url == NULL || o_url == NULL || len > HTTP_SP_URL_LEN_MAX) {
        return;
    }

    start = o_url;
    if (strncmp(start, HTTP_URL_PREFIX, HTTP_URL_PRE_LEN) == 0) {
        start = start + HTTP_URL_PRE_LEN;
    }

    if (with_para) {
        for (p = url, q = start; *q != '\0' && cnt < len; p++, q++, cnt++) {
            *p = *q;
        }
    } else {
        for (p = url, q = start; *q != '?' && *q != '\0' && cnt < len; p++, q++, cnt++) {
            *p = *q;
        }
    }

    *p = '\0';
}

