/*
 * Copyright(C) 2014 Ruijie Network. All rights reserved.
 */
/*
 * yk_lib.c
 * Original Author: zhaoyao@ruijie.com.cn, 2014-03-11
 *
 * Youku related library and parsing Youku video protocol data routines.
 *
 * History:
 *  v1.0    zhaoyao@ruijie.com.cn   2014-03-11
 *          创建此文件。
 *
 */

#include <sys/ctype.h>
#include "ngx_snoop_client.h"
#include "yk_lib.h"

#define YK_HTTP_RECV_DELAY    10 /* 秒 */

/* 解析优酷getplaylist步骤返回的http应答内容需要的宏，内部使用，因此直接在c文件中定义 */
#define YK_VALUE_LEN            16
#define YK_TOKEN_LEN_TYPE       6
#define YK_TOKEN_LEN_TYPE_END   3
#define YK_TOKEN_LEN_SEGS       8
#define YK_TOKEN_LEN_SEG_END    2
#define YK_TOKEN_LEN_NO         5
#define YK_TOKEN_LEN_SIZE       7
#define YK_TOKEN_LEN_SECONDS    9
#define YK_TOKEN_LEN_K          4
#define YK_TOKEN_LEN_K2         5
#define YK_TOKEN_LEN_SEED       6
#define YK_STRM_TYPE_CMP_LEN   (YK_STREAM_TYPE_LEN - 1)    /* 不计最后的结束符'\0' */

static char yk_request_pattern[] =
        "GET %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
                    "Chrome/33.0.1750.117 Safari/537.36\r\n"
        "Accept: */*\r\n"
        "Referer: http://%s\r\n"
        "Accept-Encoding:\r\n"
        "Accept-Language: en-US,en;q=0.8,zh-CN;q=0.6,zh;q=0.4\r\n\r\n";

static int yk_build_request(char *host, char *uri, char *referer, char *buf)
{
    int len;
    
    if (buf == NULL || host == NULL || uri == NULL || referer == NULL) {
        return -1;
    }

    /* XXX: 计算时，忽略去掉无关的%s字符 */
    len = strlen(host) + strlen(uri) + strlen(referer) + strlen(yk_request_pattern) - 6;
    if (len >= BUFFER_LEN) {
        hc_log_error( "request length (%d) exceed limit %d", len, BUFFER_LEN);
        return -1;
    }
    sprintf(buf, yk_request_pattern, uri, host, referer);

    return 0;
}

/**
 * NAME: yk_is_tradition_url
 *
 * DESCRIPTION:
 *      判断是否为优酷视频传统的url，例如v.youku.com/v_show/id_XNjgzMjc0MjY4.html。
 *
 * @yk_url: -IN 资源url。
 *
 * RETURN: 1表示是，0表示不是。
 */
int yk_is_tradition_url(char *yk_url)
{
    static char *tag1 = "v.youku.com";
    static char *tag2 = "/id_";
    char *cur;

    if (yk_url == NULL) {
        return 0;
    }

    cur = yk_url;
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
 * NAME: yk_http_session
 *
 * DESCRIPTION:
 *      与优酷服务器的http会话。
 *
 * @url:        -IN  资源url。
 * @referer:    -IN  HTTP头中referer项。
 * @response    -OUT HTTP响应内容。
 * @resp_len    -IN  HTTP响应的缓冲空间长度。
 *
 * RETURN: -1表示失败，0表示成功。
 */
int yk_http_session(char *url, char *referer, char *response, unsigned long resp_len)
{
    int sockfd = -1, err = 0;
    char host[MAX_HOST_NAME_LEN], *uri_start;
    char buffer[BUFFER_LEN];
    int nsend, nrecv, len, i, pre_len = 0, total_recv;
    struct timeval tv;
    fd_set r_set;
    int ret;

    if (url == NULL || referer == NULL || response == NULL) {
        hc_log_error("Input is invalid");
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

    if (memcmp(url, HTTP_URL_PREFIX, HTTP_URL_PRE_LEN) == 0) {
        pre_len = HTTP_URL_PRE_LEN;
    }

    memset(host, 0, MAX_HOST_NAME_LEN);
    for (i = 0; (url[pre_len + i] != '/') && (url[pre_len + i] != '\0'); i++) {
        if (i >= MAX_HOST_NAME_LEN - 1) {
            break;
        }
        host[i] = url[pre_len + i];
    }
    if (url[pre_len + i] == '\0' || i >= MAX_HOST_NAME_LEN - 1) {
        hc_log_error("url is invalid: %s", url);
        return -1;
    }
    host[i] = '\0';
    uri_start = url + pre_len + i;

    sockfd = host_connect(host);
    if (sockfd < 0) {
        hc_log_error("can not connect web(80) to %s", host);
        return -1;
    }

    memset(buffer, 0, BUFFER_LEN);
    if (yk_build_request(host, uri_start, referer, buffer) < 0) {
        hc_log_error("yk_build_request failed");
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
        hc_log_error("Send %d bytes failed", nsend);
        err = -1;
        goto out;
    }

    memset(response, 0, resp_len);
    total_recv = 0;
    bzero(&tv, sizeof(tv));
    tv.tv_sec = YK_HTTP_RECV_DELAY;
    tv.tv_usec = 0;

    while (1) {
        memset(&r_set, 0 , sizeof(r_set));
        FD_SET(sockfd, &r_set);
        ret = select(sockfd + 1, &r_set, NULL, NULL, &tv);
        
        if (ret <= 0) {
            hc_log_error("Recv select failed, ret: %d", ret);
            err = -1;
            goto out;
        }

        nrecv = recv(sockfd, response, resp_len - total_recv, MSG_WAITALL);
        if (nrecv > 0) {
            response += nrecv;
            total_recv += nrecv;
        } else if (nrecv == 0) {
            break;
        } else {
            err = -1;
            goto out;
        }
    }

    if (total_recv <= 0) {
        hc_log_error("Recv failed or meet EOF, %d", total_recv);
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
 * NAME: yk_seg_to_flvpath
 *
 * DESCRIPTION:
 *      获取优酷视频分段信息中的fp_url。
 *
 * @seg:    -IN  优酷视频分段信息。
 * @fp_url: -OUT 取得的优酷视频fp_url。
 *
 * RETURN: -1表示失败，0表示成功。
 */
int yk_seg_to_flvpath(const yk_segment_info_t *seg, char *fp_url)
{
    char fileids[YK_FILEID_LEN + 1];    /* 包含结束符'\0' */
    yk_playlist_data_t play_list;
    yk_video_seg_data_t seg_data;
    yk_stream_info_t *strm;
    
    if (seg == NULL || fp_url == NULL || seg->stream == NULL) {
        return -1;
    }

    strm = seg->stream;

    memset(fileids, 0, sizeof(fileids));
    if (yk_get_fileid(strm->streamfileids, seg->no, strm->seed, fileids) != 0) {
        hc_log_error("yk_get_fileid failed");
        return -1;
    }

    memset(&play_list, 0, sizeof(play_list));
    memcpy(play_list.fileType, strm->type, 3); /* flv, mp4, hd2, hd3 */
    play_list.drm = false;
    play_list.sid[0] = '0';
    play_list.sid[1] = '0';

    memset(&seg_data, 0, sizeof(seg_data));
    memcpy(seg_data.fileId, fileids, strlen(fileids));
    if (strlen(seg->k) >= YK_VIDEO_SEG_KEY_LEN) {
        hc_log_debug("seg->k is longer than %d: %s", YK_VIDEO_SEG_KEY_LEN, seg->k);
        return -1;
    }
    memcpy(seg_data.key, seg->k, strlen(seg->k));

    memset(fp_url, 0, HTTP_SP_URL_LEN_MAX);
    if (yk_get_fileurl(0, &play_list, &seg_data, false, 0, fp_url) != 0) {
        hc_log_error("yk_get_fileurl failed");
        return -1;
    }

    return 0;
}

static yk_stream_info_t *yk_create_stream_info(const char *type)
{
    yk_stream_info_t *p;

    if (type == NULL) {
        hc_log_error("invalid input.");
        return NULL;
    }

    p = malloc(sizeof(yk_stream_info_t));
    if (p == NULL) {
        hc_log_error("memory allocate %d bytes failed", sizeof(yk_stream_info_t));
    } else {
        memset(p, 0, sizeof(yk_stream_info_t));
        memcpy(p->type, type, YK_STREAM_TYPE_LEN - 1);
    }

    return p;
}

static yk_segment_info_t *yk_create_segment_info(char *info)
{
    yk_segment_info_t *p;
    char *cur;
    char val[YK_VALUE_LEN];
    int i;

    if (info == NULL) {
        hc_log_error("invalid input");
        return NULL;
    }

    p = malloc(sizeof(yk_segment_info_t));
    if (p == NULL) {
        hc_log_error("memory allocate %d bytes failed", sizeof(yk_segment_info_t));
        return NULL;
    }

    memset(p, 0, sizeof(yk_segment_info_t));
    cur = info;

    cur = strstr(cur, "no");
    if (cur == NULL) {
        goto err_out;
    }
    memset(val, 0, YK_VALUE_LEN);
    for (cur = cur + YK_TOKEN_LEN_NO, i = 0;
            *cur != '"' && isdigit(*cur) && i < YK_VALUE_LEN - 1;
            cur++, i++) {
        val[i] = (*cur);
    }
    p->no = atoi(val);

    cur = strstr(cur, "size");
    if (cur == NULL) {
        goto err_out;
    }
    memset(val, 0, YK_VALUE_LEN);
    for (cur = cur + YK_TOKEN_LEN_SIZE, i = 0;
            *cur != '"' && isdigit(*cur) && i < YK_VALUE_LEN - 1;
            cur++, i++) {
        val[i] = (*cur);
    }
    p->size = atoi(val);

    /*
     * 注意: 格式可能是如 "seconds":"196" or "seconds":411
     */
    cur = strstr(cur, "seconds");
    if (cur == NULL) {
        goto err_out;
    }
    memset(val, 0, YK_VALUE_LEN);
    cur = cur + YK_TOKEN_LEN_SECONDS;
    if (*cur == '"') {
        cur++;
    }
    for (i = 0; *cur != '"' && isdigit(*cur) && i < YK_VALUE_LEN - 1; cur++, i++) {
        val[i] = (*cur);
    }
    p->seconds = atoi(val);

    cur = strchr(cur, 'k');
    if (cur == NULL) {
        goto err_out;
    }
    for (cur = cur + YK_TOKEN_LEN_K, i = 0;
            *cur != '"' && *cur != '\0' && i < YK_SEGMENT_K_LEN - 1;
            cur++, i++) {
        p->k[i] = (*cur);
    }

    cur = strstr(cur, "k2");
    if (cur == NULL) {
        goto err_out;
    }
    for (cur = cur + YK_TOKEN_LEN_K2, i = 0;
            *cur != '"' && *cur != '\0' && i < YK_SEGMENT_K2_LEN - 1;
            cur++, i++) {
        p->k2[i] = (*cur);
    }

    return p;

err_out:
    free(p);
    hc_log_error("mal-formated segment info.");

    return NULL;
}

static yk_stream_info_t *yk_find_stream_info(yk_stream_info_t *streams[], const char *type)
{
    int i;

    if (streams == NULL || type == NULL) {
        return NULL;
    }

    for (i = 0; i < YK_STREAM_TYPE_TOTAL; i++) {
        if (streams[i] == NULL) {
            continue;
        }
        if (strcmp(streams[i]->type, type) == 0) {
            return streams[i];
        }
    }

    return NULL;
}

static char *yk_parse_pl_seed(const char *cur, char *seed)
{
    char *ret;
    int i;

    if (cur == NULL || seed == NULL) {
        hc_log_error("invalid input.");
        return NULL;
    }

    ret = strstr(cur, "seed");
    if (ret == NULL) {
        return ret;
    }

    for (i = 0, ret = ret + YK_TOKEN_LEN_SEED;
            *ret != ',' && isdigit(*ret) && i < (YK_SEED_STRING_LEN - 1);
            i++, ret++, seed++) {
        *seed = *ret;
    }
    *seed = '\0';

    return ret;
}

/**
 * NAME: yk_destroy_streams_all
 *
 * DESCRIPTION:
 *      销毁保存流信息的数据。
 *
 * @streams:    -IN  优酷视频流信息管理结构体指针。
 *
 * RETURN: 无返回值。
 */
void yk_destroy_streams_all(yk_stream_info_t *streams[])
{
    int i, j;

    if (streams == NULL) {
        return;
    }

    for (i = 0; i < YK_STREAM_TYPE_TOTAL; i++) {
        if (streams[i] == NULL) {
            continue;
        }

        if (streams[i]->segs != NULL) {
            for (j = 0; j < YK_STREAM_SEGS_MAX; j++) {
                if (streams[i]->segs[j] != NULL) {
                    free(streams[i]->segs[j]);
                    streams[i]->segs[j] = NULL;
                }
            }
        }

        free(streams[i]);
        streams[i] = NULL;
    }
}

/**
 * NAME: yk_debug_streams
 *
 * DESCRIPTION:
 *      打印保存流信息的调试内容。
 *
 * @streams:    -IN  优酷视频流信息管理结构体指针。
 *
 * RETURN: 无返回值。
 */
void yk_debug_streams(yk_stream_info_t *streams[])
{
    int i, j;
    yk_stream_info_t *strm;
    yk_segment_info_t *seg;

    if (streams == NULL) {
        return;
    }

    for (i = 0; streams[i] != NULL && i < YK_STREAM_TYPE_TOTAL; i++) {
        strm = streams[i];
        printf("Stream type: %s\n", strm->type);
        printf("       streamfileids: %s\n", strm->streamfileids);
        printf("       seed: %d\n", strm->seed);
        printf("       streamsizes: %d\n", strm->streamsizes);
        if (strm->segs != NULL) {
            for (j = 0; strm->segs[j] != NULL && j < YK_STREAM_SEGS_MAX; j++) {
                seg = strm->segs[j];
                printf("       segs [%2d]:\n", seg->no);
                printf("            size: %d\n", seg->size);
                printf("            seconds: %d\n", seg->seconds);
                printf("            k: %s\n", seg->k);
                printf("            k2: %s\n", seg->k2);
            }
        }
        printf("------------------------------------End\n\n");
    }
}

static char *yk_parse_pl_streamfileids(const char *cur, yk_stream_info_t *streams[])
{
    char *ret, *v;
    int i;
    yk_stream_info_t *strm;

    if (cur == NULL || streams == NULL) {
        hc_log_error("invalid input");
        return NULL;
    }

    ret = strstr(cur, "streamfileids");
    if (ret == NULL) {
        return NULL;
    }

    ret = ret + strlen("streamfileids\":{\"");
    i = 0;
    while (i < YK_STREAM_TYPE_TOTAL) {
        if (memcmp(ret, "hd2", YK_STRM_TYPE_CMP_LEN) == 0) {
            strm = yk_create_stream_info("hd2");
            if (strm == NULL) {
                goto error;
            }
            for (ret = ret + YK_TOKEN_LEN_TYPE, v = strm->streamfileids; *ret != '"'; ret++, v++) {
                *v = *ret;
            }
            *v = '\0';
            ret = ret + YK_TOKEN_LEN_TYPE_END;
            streams[i] = strm;
            i++;
        } else if (memcmp(ret, "mp4", YK_STRM_TYPE_CMP_LEN) == 0) {
            strm = yk_create_stream_info("mp4");
            if (strm == NULL) {
                goto error;
            }
            for (ret = ret + YK_TOKEN_LEN_TYPE, v = strm->streamfileids; *ret != '"'; ret++, v++) {
                *v = *ret;
            }
            *v = '\0';
            ret = ret + YK_TOKEN_LEN_TYPE_END;
            streams[i] = strm;
            i++;
        } else if (memcmp(ret, "hd3", YK_STRM_TYPE_CMP_LEN) == 0) {
            strm = yk_create_stream_info("hd3");
            if (strm == NULL) {
                goto error;
            }
            for (ret = ret + YK_TOKEN_LEN_TYPE, v = strm->streamfileids; *ret != '"'; ret++, v++) {
                *v = *ret;
            }
            *v = '\0';
            ret = ret + YK_TOKEN_LEN_TYPE_END;
            streams[i] = strm;
            i++;
        } else if (memcmp(ret, "flv", YK_STRM_TYPE_CMP_LEN) == 0) {
            strm = yk_create_stream_info("flv");
            if (strm == NULL) {
                goto error;
            }
            for (ret = ret + YK_TOKEN_LEN_TYPE, v = strm->streamfileids; *ret != '"'; ret++, v++) {
                *v = *ret;
            }
            *v = '\0';
            ret = ret + YK_TOKEN_LEN_TYPE_END;
            streams[i] = strm;
            i++;
        } else if (memcmp(ret, "\"se", YK_STRM_TYPE_CMP_LEN) == 0) {   /* End of streamfileids */
            ret++;
            return ret;
        } else {
            goto error;
        }
    }

error:
    hc_log_error("get stream file id failed");

    return NULL;
}

static char *yk_parse_pl_segs_do(char *cur, yk_stream_info_t *stream)
{
    char *ret, *end, info[YK_SEG_INFO_LEN];
    yk_segment_info_t *seg;
    int i, seg_cnt = 0;

    if (cur == NULL || stream == NULL) {
        hc_log_error("invalid input.");
        return NULL;
    }

    ret = cur + YK_TOKEN_LEN_TYPE;
    end = strchr(ret, ']');
    while (ret < end) {
        memset(info, 0, YK_SEG_INFO_LEN);
        for (i = 0; *ret != '}' && i < YK_SEG_INFO_LEN; ret++, i++) {
            info[i] = (*ret);   /* 分段信息的本地副本 */
        }
        seg = yk_create_segment_info(info);
        if (seg == NULL) {
            return NULL;
        }
        seg->stream = stream;
        stream->segs[seg_cnt] = seg;
        seg_cnt++;
        if (seg_cnt > YK_STREAM_SEGS_MAX) {
            return NULL;
        }
        ret = ret + YK_TOKEN_LEN_SEG_END;
    }
    ret = ret + YK_TOKEN_LEN_SEG_END;

    return ret;
}

static char *yk_parse_pl_segs(const char *cur, yk_stream_info_t *streams[])
{
    char *ret;
    yk_stream_info_t *strm;

    if (cur == NULL || streams == NULL) {
        hc_log_error("invalid input.");
        return NULL;
    }

    ret = strstr(cur, "segs");
    if (ret == NULL) {
        return NULL;
    }
    ret = ret + YK_TOKEN_LEN_SEGS;
    while (1) {
        if (memcmp(ret, "hd2", YK_STRM_TYPE_CMP_LEN) == 0) {
            strm = yk_find_stream_info(streams, "hd2");
            if (strm == NULL) {
                goto error;
            }
            ret = yk_parse_pl_segs_do(ret, strm);
            if (ret == NULL) {
                goto error;
            }
        } else if (memcmp(ret, "mp4", YK_STRM_TYPE_CMP_LEN) == 0) {
            strm = yk_find_stream_info(streams, "mp4");
            if (strm == NULL) {
                goto error;
            }
            ret = yk_parse_pl_segs_do(ret, strm);
            if (ret == NULL) {
                goto error;
            }
        } else if (memcmp(ret, "hd3", YK_STRM_TYPE_CMP_LEN) == 0) {
            strm = yk_find_stream_info(streams, "hd3");
            if (strm == NULL) {
                goto error;
            }
            ret = yk_parse_pl_segs_do(ret, strm);
            if (ret == NULL) {
                goto error;
            }
        } else if (memcmp(ret, "flv", YK_STRM_TYPE_CMP_LEN) == 0) {
            strm = yk_find_stream_info(streams, "flv");
            if (strm == NULL) {
                goto error;
            }
            ret = yk_parse_pl_segs_do(ret, strm);
            if (ret == NULL) {
                goto error;
            }
        } else if (memcmp(ret, "\"st", YK_STRM_TYPE_CMP_LEN) == 0) {
            /* End of segs，正常返回 */
            ret++;
            return ret;
        } else {
            goto error;
        }
    }

error:
    hc_log_error("parse segments info failed.");

    return NULL;
}

/**
 * NAME: yk_parse_playlist
 *
 * DESCRIPTION:
 *      解析getplaylist阶段返回的http应答内容。
 *
 * @data:    -IN  http应答的内容。
 * @streams: -OUT 存放解析的各类视屏流信息。
 *
 * RETURN: -1表示失败，0表示成功。
 */
int yk_parse_playlist(char *data, yk_stream_info_t *streams[])
{
    int i, seed;
    char *cur = data;
    char ss[YK_SEED_STRING_LEN];

    if (data == NULL || streams == NULL) {
        hc_log_error("invalid input.");
        return -1;
    }
    for (i = 0; i < YK_STREAM_TYPE_TOTAL; i++) {
        if (streams[i] != NULL) {
            hc_log_error("streams' information has been parsed already.");
            return -1;
        }
    }

    /* Get seed */
    bzero(ss, YK_SEED_STRING_LEN);
    cur = yk_parse_pl_seed(cur, ss);
    if (cur == NULL) {
        hc_log_error("seed does not exist.");
        return -1;
    }
    seed = atoi(ss);

    /* Get streamfileids */
    cur = yk_parse_pl_streamfileids(cur, streams);   /* 它会分配并初始化stream_info */
    if (cur == NULL) {
        return -1;
    }
    for (i = 0; streams[i] != NULL && i < YK_STREAM_TYPE_TOTAL; i++) {
        streams[i]->seed = seed;
    }

    /* Get segs */
    cur = yk_parse_pl_segs(cur, streams);            /* 它会分配并初始化segment_info */
    if (cur == NULL) {
        return -1;
    }

    /* TODO: Get streamsizes */

    return 0;
}

/**
 * NAME: yk_parse_flvpath
 *
 * DESCRIPTION:
 *      解析getflvpath阶段返回的http应答内容。
 *
 * @data:       -IN  http应答的内容。
 * @real_url:   -OUT 存放解析出来的视频分段真实url。
 *
 * RETURN: -1表示失败，0表示成功。
 */
int yk_parse_flvpath(char *data, char *real_url)
{
    char *tag1 = "Location: ";
    char *tag2 = "server\":\"";
    char *cur, *dst;
    int i;

    if (data == NULL || real_url == NULL) {
        hc_log_error("invalid input.");
        return -1;
    }

    memset(real_url, 0, HTTP_SP_URL_LEN_MAX);

    cur = strstr(data, tag1);
    if (cur != NULL) {
        for (cur = cur + strlen(tag1), dst = real_url, i = 0;
                *cur != '\r' && *cur != '\n' && *cur != '\0' && i < (HTTP_SP_URL_LEN_MAX - 1);
                cur++, dst++, i++) {
            *dst = *cur;
        }
        goto end;
    }

    cur = strstr(data, tag2);
    if (cur != NULL) {
        for (cur = cur + strlen(tag2), dst = real_url, i = 0;
                *cur != '"' && *cur != ',' && *cur != '\0' && i < (HTTP_SP_URL_LEN_MAX - 1);
                cur++, dst++) {
            *dst = *cur;
        }
        goto end;
    }

    if (cur == NULL) {
        hc_log_error("do not find %s or %s in getflvpath response", tag1, tag2);
        return -1;
    }

end:
    if (i >= (HTTP_SP_URL_LEN_MAX - 1)) {
        hc_log_error("WARNING: real_url maybe exceed buffer length %d", HTTP_SP_URL_LEN_MAX);
    }

    return 0;
}

