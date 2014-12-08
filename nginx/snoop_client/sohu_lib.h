/*
 * Copyright(C) 2014 Ruijie Network. All rights reserved.
 */
/*
 * sohu_lib.h
 * Original Author: zhaoyao@ruijie.com.cn, 2014-04-28
 *
 * Sohu related library's header file.
 *
 * History:
 *  v1.0    zhaoyao@ruijie.com.cn   2014-04-28
 *          创建此文件。
 *
 */

#ifndef _SOHU_LIB_H_
#define _SOHU_LIB_H_

int sohu_is_m3u8_url(char *sohu_url);
int sohu_http_session(char *url, char *response, unsigned long resp_len);
char *sohu_parse_m3u8_response(char *curr, char *real_url);
int sohu_parse_file_url_response(char *response, char *real_url);

#endif /* _SOHU_LIB_H_ */

