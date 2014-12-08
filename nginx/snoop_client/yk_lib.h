/*
 * Copyright(C) 2014 Ruijie Network. All rights reserved.
 */
/*
 * yk_lib.h
 * Original Author: zhaoyao@ruijie.com.cn, 2014-03-11
 *
 * Youku related library's header file.
 *
 * History:
 *  v1.0    zhaoyao@ruijie.com.cn   2014-03-11
 *          创建此文件。
 *
 */

#ifndef _YK_LIB_H_
#define _YK_LIB_H_

#include "ngx_snoop_client.h"

#define YK_VID_MAX_LEN          16
#define YK_VID_VALID_LEN        13

#define YK_FILEID_LEN           66   /* 视频文件名解析后的长度 */

#define YK_SEED_STRING_LEN      8

#define YK_STREAM_TYPE_TOTAL    4   /* flv, mp4, hd2, hd3 */
#define YK_STREAM_TYPE_LEN      4
#define YK_STREAM_FILE_IDS_LEN  256
#define YK_STREAM_SEGS_MAX      64

#define YK_SEG_INFO_LEN         256
#define YK_SEGMENT_K_LEN        32
#define YK_SEGMENT_K2_LEN       32

#define YK_PLAYLIST_SID_LEN     25
#define YK_PLAYLIST_TYPE_LEN    6
#define YK_PLAYLIST_KEY_LEN     25

#define YK_VIDEO_SEG_KEY_LEN    25

typedef struct yk_playlist_data_s {
    char sid[YK_PLAYLIST_SID_LEN];
    char fileType[YK_PLAYLIST_TYPE_LEN];    /* flv, mp4, hd2, hd3, flvhd */
    bool drm;                               /* 含义未知 */
    char key1[YK_PLAYLIST_KEY_LEN];
    char key2[YK_PLAYLIST_KEY_LEN];
} yk_playlist_data_t;

typedef struct yk_video_seg_data_s {
    char fileId[YK_FILEID_LEN + 1]; /* 加上结束符'\0' */
    char key[YK_VIDEO_SEG_KEY_LEN];
    int seconds;
} yk_video_seg_data_t;

typedef struct yk_stream_info_s yk_stream_info_t;

typedef struct yk_segment_info_s {
    yk_stream_info_t *stream;       /* Where I am belonging to. */
    int  no;                        /* number */
    int  size;
    int  seconds;
    char k[YK_SEGMENT_K_LEN];
    char k2[YK_SEGMENT_K2_LEN];
} yk_segment_info_t;

struct yk_stream_info_s {
    char type[YK_STREAM_TYPE_LEN];  /* flv, mp4, hd2, hd3 */
    char streamfileids[YK_STREAM_FILE_IDS_LEN];
    int  streamsizes;
    int  seed;
    yk_segment_info_t *segs[YK_STREAM_SEGS_MAX];
};

int yk_url_to_playlist(char *url, char *playlist);
int yk_get_fileid(char *streamfileids, int video_num, int seed, char *fileids);
int yk_get_fileurl(int num, yk_playlist_data_t *play_list, yk_video_seg_data_t *seg_data,
                   bool use_jumptime, int jump_time, char *out_url);
int yk_http_session(char *url, char *referer, char *response, unsigned long resp_len);
int yk_seg_to_flvpath(const yk_segment_info_t *seg, char *fp_url);
int yk_is_tradition_url(char *yk_url);

int yk_parse_playlist(char *data, yk_stream_info_t *streams[]);
int yk_parse_flvpath(char *data, char *real_url);
void yk_destroy_streams_all(yk_stream_info_t *streams[]);
void yk_debug_streams(yk_stream_info_t *streams[]);

#endif /* _YK_LIB_H_ */

