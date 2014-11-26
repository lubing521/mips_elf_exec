#include <net/networklayer/inet_lib.h>
#include <sys/socket.h>
#include <sys/slab.h>
#include <net/networklayer/in.h>
#include <net/networklayer/socket.h>
#include <app/fs/app_fs_lib.h>
#include <sys/errno.h>
#include <asm/string.h>
#include <sys/string.h>
#include <sys/support.h>
#include <sys/kernel.h>

#include "http_download.h"

int http_dl_log_level = 6;

static char *http_dl_agent_string = "Mozilla/5.0 (Windows NT 6.1; WOW64) " \
                                    "AppleWebKit/537.36 (KHTML, like Gecko) " \
                                    "Chrome/35.0.1916.153 Safari/537.36";
static char *http_dl_agent_string_genuine = "Wget/1.5.3";

static http_dl_list_t http_dl_list_initial;     /* �����ѳ�ʼ������������ */
static http_dl_list_t http_dl_list_requesting;  /* ���õȴ�����HTTP������������� */
static http_dl_list_t http_dl_list_downloading; /* �������ڽ���HTTP�Ự���������� */
static http_dl_list_t http_dl_list_finished;    /* ����������ɻ��޷����ص����� */

static int g_http_dl_ctrl_sockfd = -1;          /* ��������������׽���fd, UDP�׽��� */
static int g_http_dl_exit = 0;                  /* �˳�select()ѭ���Ŀ����ֶ� */

static task_t g_http_download_task = NULL;
static task_t g_http_write_test_task = NULL;

/* Count the digits in a (long) integer.  */
static int http_dl_numdigit(long a)
{
    int res = 1;

    while ((a /= 10) != 0) {
        ++res;
    }

    return res;
}

static int http_dl_conn(char *hostname, unsigned short port)
{
    int ret;
    struct sockaddr_in sa;

    if (hostname == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    bzero(&sa, sizeof(sa));
    ret = inet_pton(AF_INET, hostname, &sa.sin_addr);
    if (ret != 1) {
        return -HTTP_DL_ERR_INVALID;
    }

    /* Set port and protocol */
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);

    /* Make an internet socket, stream type.  */
    if ((ret = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return -HTTP_DL_ERR_SOCK;
    }

    /* Connect the socket to the remote host.  */
    if (connect(ret, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
        close(ret);
        return -HTTP_DL_ERR_CONN;
    }

    http_dl_log_debug("Created and connected socket fd %d.", ret);

    return ret;
}

static int http_dl_iwrite(int fd, char *buf, int len)
{
    int res = 0;

    if (buf == NULL || len <= 0) {
        return -HTTP_DL_ERR_INVALID;
    }

    /* 'write' may write less than LEN bytes, thus the outward loop
    keeps trying it until all was written, or an error occurred.  The
    inner loop is reserved for the usual EINTR f*kage, and the
    innermost loop deals with the same during select().  */
    while (len > 0) {
        do {
            res = write(fd, buf, len);
        } while (res == -1 && errno == EINTR);
        if (res <= 0) {
            return -HTTP_DL_ERR_WRITE;
        }
        buf += res;
        len -= res;
    }
    return len;
}

static int http_dl_write(int fd, char *buf, int len)
{
    int res = 0, already_write = 0;

    if (fd < 0 || buf == NULL || len <= 0) {
        return already_write;
    }

    while (len > 0) {
        do {
            res = write(fd, buf, len);
            if (res > 0) {
                already_write += res;
            }
        } while (res == -1 && errno == EINTR);
        if (res <= 0) {
            return already_write;
        }
        buf += res;
        len -= res;
    }
    return already_write;
}

static void *http_dl_xrealloc(void *obj, size_t size)
{
    void *res;

    /* Not all Un*xes have the feature of realloc() that calling it with
    a NULL-pointer is the same as malloc(), but it is easy to
    simulate.  */
    if (obj) {
        res = realloc(obj, size);
    } else {
        res = malloc(size);
    }

    if (res == NULL) {
        http_dl_log_debug("allocate %d failed", size);
    }

    return res;
}

static void http_dl_reset_elapsed_time(http_dl_info_t *di)
{
    if (di == NULL) {
        return;
    }

    gettimeofday(&di->start_time, NULL);
    di->elapsed_time = -1;  /* for unsigned long, -1 means maximum time */
}

/* unit is msecs */
static unsigned long http_dl_update_elapsed_time(http_dl_info_t *di)
{
    struct timeval t;
    unsigned long ret;

    if (di == NULL) {
        return -1;
    }

    if (di->elapsed_time == 0) {
        http_dl_log_debug("Never ever start download content: %s.", di->url);
        return -1;
    }

    gettimeofday(&t, NULL);
    ret = (t.tv_sec - di->start_time.tv_sec) * 1000
           + (t.tv_usec - di->start_time.tv_usec) / 1000;
    if (ret == 0) {
        ret = 100;  /* ������㲻��delta time����ôǿ����Ϊ100ms����������ٶ� */
    }
    di->elapsed_time = ret;

    return ret;
}

static int http_dl_init_filefd(http_dl_info_t *info)
{
    int fd = -1, ret, restart_len;
    struct stat file_stat;

    if (info == NULL) {
        http_dl_log_debug("Invalid input, info == NULL");
        return -HTTP_DL_ERR_INVALID;
    }

    bzero(&file_stat, sizeof(file_stat));
    ret = stat(info->local, &file_stat);

    if ((ret == 0) && (S_ISREG(file_stat.st_mode))) {
        /* File already exist, and it is regular file. */
        http_dl_log_debug("File %s is exist, and is regular file.", info->local);
        restart_len = file_stat.st_size;
        fd = open(info->local, O_RDWR | O_APPEND, 0644);
    } else  {
        http_dl_log_debug("%s status fail or non-regular file, create it.", info->local);
        restart_len = 0;
        fd = open(info->local, O_RDWR | O_CREAT, 0644);
    }

    if (fd < 0) {
        http_dl_log_debug("Open or create %s failed.", info->local);
        return -HTTP_DL_ERR_FOPEN;
    }

    info->filefd = fd;
    info->restart_len = restart_len;
    if (restart_len != 0) {
        info->flags |= HTTP_DL_F_RESTART_FILE;
    }

    http_dl_log_debug("Open file %s success, fd[%d], restart_len[%ld].",
                        info->local, info->filefd, info->restart_len);

    return HTTP_DL_OK;
}

static http_dl_info_t *http_dl_create_info(char *url)
{
    http_dl_info_t *di;
    int url_len;
    char *p, *host, *path, *local;
    int host_len, path_len, local_len;
    int port = 0;

    if (url == NULL) {
        http_dl_log_debug("invalid input url");
        return NULL;
    }

    url_len = strlen(url);
    if (url_len >= HTTP_DL_URL_LEN) {
        http_dl_log_debug("url is longer than %d: %s", HTTP_DL_URL_LEN - 1, url);
        return NULL;
    }

    if ((p = strstr(url, HTTP_URL_PREFIX)) != NULL) {
        p = p + HTTP_URL_PRE_LEN;
    } else {
        p = url;
    }

    /* ����host���Լ����ܴ��ڵ�port */
    for (host = p, host_len = 0; *p != '/' && *p != ':' && *p != '\0'; p++, host_len++) {
        (void)0;
    }
    if (*p == ':') {
        p++;
        while (isdigit(*p)) {
            port = port * 10 + (*p - '0');
            if (port > 0xFFFF) {
                http_dl_log_debug("invalid port: %s", url);
                return NULL;
            }
            p++;
        }
        if (*p != '/') {
            http_dl_log_debug("invalid port: %s", url);
            return NULL;
        }
    } else if (*p == '\0') {
        http_dl_log_debug("invalid host: %s", host);
        return NULL;
    }
    if (host_len <= 0 || host_len >= HTTP_DL_HOST_LEN) {
        http_dl_log_debug("invalid host length: %s", host);
        return NULL;
    }

    /* ����path */
    for (path = p, path_len = 0; *p != '\0' && *p != ' ' && *p != '\n'; p++, path_len++) {
        (void)0;
    }
    if (path_len <= 0 || path_len >= HTTP_DL_PATH_LEN) {
        http_dl_log_debug("invalid path length: %s", path);
        return NULL;
    }

    /* �������ر����ļ���local */
    p--;
    for (local_len = 0; *p != '/'; p--, local_len++) {
        (void)0;
    }
    local = p + 1;
    if (local_len <= 0 || local_len >= HTTP_DL_LOCAL_LEN) {
        http_dl_log_debug("invalid local file name: %s", local);
        return NULL;
    }

    di = http_dl_xrealloc(NULL, sizeof(http_dl_info_t));
    if (di == NULL) {
        http_dl_log_debug("allocate failed");
        return NULL;
    }

    bzero(di, sizeof(http_dl_info_t));
    memcpy(di->url, url, url_len);
    memcpy(di->host, host, host_len);
    memcpy(di->path, path, path_len);
    memcpy(di->local, local, local_len);
    if (port != 0) {
        di->port = port;
    } else {
        di->port = 80;  /* Ĭ�ϵ�http����˿� */
    }

    di->stage = HTTP_DL_STAGE_INIT;
    INIT_LIST_HEAD(&di->list);

    di->recv_len = 0;
    di->content_len = 0;
    di->total_len = 0;
    di->status_code = HTTP_DL_OK;   /* ��ʼֵΪ0����HTTP�Ựʧ��ʱΪ-1������ֵ��ʾHTTP״̬�� */
    di->elapsed_time = 0;           /* 0Ϊ��ʼֵ����ʾ��δ��ʼ���ع�content */
    di->sockfd = -1;
    if (http_dl_init_filefd(di) != HTTP_DL_OK) {
        http_dl_log_error("Initialize file fd failed: %s.", di->local);
        goto err_out;
    }

    di->buf_data = di->buf;
    di->buf_tail = di->buf;

    return di;

err_out:
    http_dl_free(di);

    return NULL;
}

static void http_dl_add_info_to_list(http_dl_info_t *info, http_dl_list_t *list)
{
    if (info == NULL || list == NULL) {
        return;
    }

    list_add_tail(&info->list, &list->list);
    list->count++;
}

static void http_dl_add_info_to_download_list(http_dl_info_t *info)
{
    http_dl_add_info_to_list(info, &http_dl_list_downloading);
}

static void http_dl_add_info_to_initial_list(http_dl_info_t *info)
{
    if (info == NULL) {
        return;
    }

    if (info->stage > HTTP_DL_STAGE_INIT) {
        http_dl_log_error("info->stage [%d] is larger than initial stage [%d].",
                            info->stage, HTTP_DL_STAGE_INIT);
        return;
    }

    http_dl_add_info_to_list(info, &http_dl_list_initial);
}

static void http_dl_add_info_to_request_list(http_dl_info_t *info)
{
    http_dl_add_info_to_list(info, &http_dl_list_requesting);
}

static void http_dl_add_info_to_finish_list(http_dl_info_t *info)
{
    http_dl_add_info_to_list(info, &http_dl_list_finished);
}

static void http_dl_del_info_from_list(http_dl_info_t *info, http_dl_list_t *list)
{
    if (info == NULL || list == NULL) {
        return;
    }

    /* XXX TODO: ��� */
    list_del_init(&info->list);
    list->count--;
}

static void http_dl_del_info_from_initial_list(http_dl_info_t *info)
{
    if (info == NULL) {
        return;
    }

    if (info->stage != HTTP_DL_STAGE_INIT) {
        http_dl_log_error("info->stage [%d] not equals to initial stage [%d].",
                            info->stage, HTTP_DL_STAGE_INIT);
        return;
    }
    http_dl_del_info_from_list(info, &http_dl_list_initial);
}
static void http_dl_del_info_from_request_list(http_dl_info_t *info)
{
    http_dl_del_info_from_list(info, &http_dl_list_requesting);
}
static void http_dl_del_info_from_download_list(http_dl_info_t *info)
{
    http_dl_del_info_from_list(info, &http_dl_list_downloading);
}

static void http_dl_del_info_from_finish_list(http_dl_info_t *info)
{
    http_dl_del_info_from_list(info, &http_dl_list_finished);
}

static void http_dl_dump_info(http_dl_info_t *info)
{
    if (info == NULL) {
        return;
    }

    http_dl_print_raw("Task: %s\n", info->url);
    http_dl_print_raw("\tStatus code: %d\n"
                      "\tMessage: %s\n"
                      "\tLocal file: %s\n"
                      "\tLength: %ld/%ld\n"
                      "\tRestart: %ld\n"
                      "\tTotal: %ld\n",
                      info->status_code,
                      info->err_msg,
                      info->local,
                      info->recv_len, info->content_len,
                      info->restart_len,
                      info->total_len);
    if (info->elapsed_time != 0) {
        http_dl_print_raw("\tElapse: %ld sec\n"
                          "\tSpeed: %ld KB/s\n",
                          info->elapsed_time / 1000,
                          info->recv_len / info->elapsed_time);
    }
    http_dl_print_raw("----------------------------------------------------------\n");
}

/* ������������ */
static int http_dl_finish_info(http_dl_info_t *info)
{
    if (info == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    if (info->stage != HTTP_DL_STAGE_FINISH) {
        http_dl_log_error("info (stage[%d]) should not in here.", info->stage);
        return -HTTP_DL_ERR_INVALID;
    }

    if (info->filefd >= 0) {
        http_dl_log_debug("Close opened file fd %d: %s.", info->filefd, info->local);
        close(info->filefd);
        info->filefd = -1;
    }

    if (info->sockfd >= 0) {
        http_dl_log_info("Close opened socket fd %d: %s:%u.", info->sockfd, info->host, info->port);
        shutdown(info->sockfd, SHUT_RDWR);
        http_dl_log_info("Shutdown %d success: %s.", info->sockfd, info->url);
//        close(info->sockfd);
//        http_dl_log_info("Close %d success: %s.", info->sockfd, info->url);

        info->sockfd = -1;
    }

    http_dl_log_debug("Before dumping info.");

    http_dl_dump_info(info);

    http_dl_log_debug("After dumping info.");

    http_dl_free(info);

    return HTTP_DL_OK;
}

static void http_dl_init()
{
    http_dl_list_initial.count = 0;
    INIT_LIST_HEAD(&http_dl_list_initial.list);
    sprintf(http_dl_list_initial.name, "Initial list");

    http_dl_list_requesting.count = 0;
    INIT_LIST_HEAD(&http_dl_list_requesting.list);
    sprintf(http_dl_list_requesting.name, "Requesting list");

    http_dl_list_downloading.count = 0;
    INIT_LIST_HEAD(&http_dl_list_downloading.list);
    sprintf(http_dl_list_downloading.name, "Downloading list");

    http_dl_list_finished.count = 0;
    INIT_LIST_HEAD(&http_dl_list_finished.list);
    sprintf(http_dl_list_finished.name, "Finished list");
}

static void http_dl_list_destroy(http_dl_list_t *list)
{
    http_dl_info_t *info, *next_info;

    if (list == NULL) {
        return;
    }

    list_for_each_entry_safe(info, next_info, &list->list, list, http_dl_info_t) {
        http_dl_log_debug("[%s] delete %s", list->name, info->url);
        list_del_init(&info->list);
        /* XXX FIXME: ����ֱ��ʹ��http_dl_finish_info()���ͷ�ÿ��info��̬����ʹ򿪵���Դ�� */
        info->stage = HTTP_DL_STAGE_FINISH; /* Ҫ����http_dl_finish_info()��������FINISH״̬ */
        http_dl_finish_info(info);
        list->count--;
    }

    if (list->count != 0) {
        http_dl_log_error("[%s] FATAL error, after destroy list->count %d (should 0).",
                                list->name, list->count);
    } else {
        http_dl_log_debug("[%s] destroy success.", list->name);
    }
}

static void http_dl_destroy()
{
    http_dl_list_destroy(&http_dl_list_initial);
    http_dl_list_destroy(&http_dl_list_requesting);
    http_dl_list_destroy(&http_dl_list_downloading);
    http_dl_list_destroy(&http_dl_list_finished);
}

/* ����HTTP���� */
static int http_dl_send_req(http_dl_info_t *info)
{
    int ret, nwrite;
    char range[HTTP_DL_BUF_LEN], *useragent;
    char *request;
    int request_len;
    char *command = "GET";

    if (info == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    bzero(range, sizeof(range));
    if (info->restart_len != 0) { /* �ϵ����� */
        if (sizeof(range) < (http_dl_numdigit(info->restart_len) + 17)) {
            http_dl_log_error("Range string is longer than %d", sizeof(range) - 17);
            return -HTTP_DL_ERR_INVALID;
        }
        sprintf(range, "Range: bytes=%ld-\r\n", info->restart_len);
    }

    if (info->flags & HTTP_DL_F_GENUINE_AGENT) {
        useragent = http_dl_agent_string_genuine;
    } else {
        useragent = http_dl_agent_string;
    }

    request_len = strlen(command) + strlen(info->path)
                + strlen(useragent)
                + strlen(info->host) + http_dl_numdigit(info->port)
                + strlen(HTTP_ACCEPT)
                + strlen(range)
                + 64;
    request = http_dl_xrealloc(NULL, request_len);
    if (request == NULL) {
        http_dl_log_error("Allocate request buffer %d failed.", request_len);
        return -HTTP_DL_ERR_RESOURCE;
    }

    bzero(request, request_len);
    sprintf(request, "%s %s HTTP/1.0\r\n"
                     "User-Agent: %s\r\n"
                     "Host: %s:%u\r\n"
                     "Accept: %s\r\n"
                     "%s\r\n",
                     command, info->path,
                     useragent,
                     info->host, info->port,
                     HTTP_ACCEPT,
                     range);
    http_dl_log_debug("\n--- request begin ---\n%s--- request end ---\n", request);

    nwrite = http_dl_iwrite(info->sockfd, request, strlen(request));
    if (nwrite < 0) {
        http_dl_log_debug("Write HTTP request failed.");
        ret = -HTTP_DL_ERR_WRITE;
        goto err_out;
    }

    http_dl_log_debug("HTTP request sent: %s.", info->url);
    ret = HTTP_DL_OK;

err_out:
    http_dl_free(request);

    return ret;
}

static int http_dl_check_status_code(http_dl_info_t *info)
{
    if (info == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    if (info->status_code == HTTP_STATUS_OK
        || info->status_code == HTTP_STATUS_PARTIAL_CONTENTS) {
        return HTTP_DL_OK;
    }

    return -HTTP_DL_ERR_INVALID;
}

static int http_dl_parse_status_line(http_dl_info_t *info)
{
    int reason_nbytes;
    int mjr, mnr, statcode;
    char *line_end, *p;

    if (info == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    if (info->stage != HTTP_DL_STAGE_PARSE_STATUS_LINE) {
        http_dl_log_debug("Wrong stage %d.", info->stage);
        return -HTTP_DL_ERR_INTERNAL;
    }

    *(info->buf_tail) = '\0';   /* �ַ�������ʱ��ȷ����Խ�� */

    line_end = strstr(info->buf_data, "\r\n");
    if (line_end == NULL) {
        /* status line��û��������������... */
        http_dl_log_debug("Incompleted status line: %s", info->buf_data);
        return HTTP_DL_OK;
    }

    /* ����HTTP״̬�н���֮ǰ����״̬����ΪĬ��Invalidֵ */
    info->status_code = -HTTP_DL_ERR_INVALID;

    /* The standard format of HTTP-Version is: `HTTP/X.Y', where X is
     major version, and Y is minor version.  */
    if (strncmp(info->buf_data, "HTTP/", 5) != 0) {
        http_dl_log_debug("Invalid status line: %s", info->buf_data);
        sprintf(info->err_msg, "Invalid status line: not begin with \"HTTP/\".");
        return -HTTP_DL_ERR_HTTP;
    }
    info->buf_data += 5;

    /* Calculate major HTTP version.  */
    p = info->buf_data;
    for (mjr = 0; isdigit(*p); p++) {
        mjr = 10 * mjr + (*p - '0');
    }
    if (*p != '.' || p == info->buf_data) {
        http_dl_log_debug("Invalid status line: %s", info->buf_data);
        sprintf(info->err_msg, "Invalid status line: get major HTTP version failed.");
        return -HTTP_DL_ERR_HTTP;
    }
    p++;
    info->buf_data = p;

    /* Calculate minor HTTP version.  */
    for (mnr = 0; isdigit(*p); p++) {
        mnr = 10 * mnr + (*p - '0');
    }
    if (*p != ' ' || p == info->buf_data) {
        http_dl_log_debug("Invalid status line: %s", info->buf_data);
        sprintf(info->err_msg, "Invalid status line: get minor HTTP version failed.");
        return -HTTP_DL_ERR_HTTP;
    }

    http_dl_log_debug("HTTP version is HTTP/%d.%d", mjr, mnr);

    p++;
    info->buf_data = p;

    /* Calculate status code.  */
    if (!(isdigit(p[0]) && isdigit(p[1]) && isdigit(p[2]))) {
        http_dl_log_debug("Invalid status line: %s", info->buf_data);
        sprintf(info->err_msg, "Invalid status line: get status code failed.");
        return -HTTP_DL_ERR_HTTP;
    }
    statcode = 100 * (p[0] - '0') + 10 * (p[1] - '0') + (p[2] - '0');
    info->status_code = statcode;
    http_dl_log_debug("HTTP status code is %d", statcode);

    /* Set up the reason phrase pointer.  */
    p += 3;
    info->buf_data = p;
    if (*p != ' ') {
        http_dl_log_debug("Invalid status line: %s", info->buf_data);
        sprintf(info->err_msg, "Invalid status line: get reason phrase failed.");
        return -HTTP_DL_ERR_HTTP;
    }
    info->buf_data++;
    reason_nbytes = line_end - info->buf_data;
    bzero(info->err_msg, sizeof(info->err_msg));
    memcpy(info->err_msg, info->buf_data, MINVAL((sizeof(info->err_msg) - 1), reason_nbytes));

    http_dl_log_debug("Finish parse HTTP status line: %d %s", info->status_code, info->err_msg);

    if (http_dl_check_status_code(info) != HTTP_DL_OK) {
        http_dl_log_info("Status code %d illegal, give up downloading: %s.",
                            info->status_code, info->url);
        return -HTTP_DL_ERR_HTTP;
    }

    info->stage = HTTP_DL_STAGE_PARSE_HEADER;

    info->buf_data = line_end + 2;  /* �Թ�"\r\n" */
    if (info->buf_data < info->buf_tail) {
        /* ��������δ�����꣬ת����һ��stage���������� */
        return -HTTP_DL_ERR_AGAIN;
    }

    return HTTP_DL_OK;
}

/* Skip LWS (linear white space), if present.  Returns number of
   characters to skip.  */
static int http_dl_clac_lws(const char *string)
{
    const char *p = string;

    if (string == NULL) {
        return 0;
    }

    while (*p == ' ' || *p == '\t') {
        ++p;
    }

    return (p - string);
}

static int http_dl_header_extract_long_num(const char *val, void *closure)
{
    const char *p = val;
    long result;

    if (val == NULL || closure == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    for (result = 0; isdigit(*p); p++) {
        result = 10 * result + (*p - '0');
    }
    if (*p != '\r') {
        return -HTTP_DL_ERR_INVALID;
    }

    *(long *)closure = result;

    return HTTP_DL_OK;
}

static int http_dl_header_dup_str_to_buf(const char *val, void *buf)
{
    int len;
    char *val_end;

    if (val == NULL || buf == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    val_end = strstr(val, "\r\n");
    if (val_end == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    len = val_end - val;
    if (len <= 0) {
        return -HTTP_DL_ERR_INVALID;
    }

    bzero(buf, HTTP_DL_BUF_LEN);
    memcpy(buf, val, MINVAL(len, HTTP_DL_BUF_LEN - 1));

    return HTTP_DL_OK;
}

/*
 * Content-Range: bytes 1113952-1296411/9570351
 * Content-Range: bytes 0-12903171/12903172
 */
static int http_dl_header_parse_range(const char *hdr, void *arg)
{
    http_dl_range_t *closure = (http_dl_range_t *)arg;
    long num;

    if (closure == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    /* Certain versions of Nutscape proxy server send out
    `Content-Length' without "bytes" specifier, which is a breach of
    RFC2068 (as well as the HTTP/1.1 draft which was current at the
    time).  But hell, I must support it...  */
    if (strncasecmp(hdr, "bytes", 5) == 0) {
        hdr += 5;
        hdr += http_dl_clac_lws(hdr);
        if (!*hdr) {
            return -HTTP_DL_ERR_INVALID;
        }
    }

    if (!isdigit(*hdr)) {
        return -HTTP_DL_ERR_INVALID;
    }

    for (num = 0; isdigit(*hdr); hdr++) {
        num = 10 * num + (*hdr - '0');
    }

    if (*hdr != '-' || !isdigit(*(hdr + 1))) {
        return -HTTP_DL_ERR_INVALID;
    }

    closure->first_byte_pos = num;
    ++hdr;

    for (num = 0; isdigit(*hdr); hdr++) {
        num = 10 * num + (*hdr - '0');
    }

    if (*hdr != '/' || !isdigit(*(hdr + 1))) {
        return -HTTP_DL_ERR_INVALID;
    }

    closure->last_byte_pos = num;
    ++hdr;

    for (num = 0; isdigit(*hdr); hdr++) {
        num = 10 * num + (*hdr - '0');
    }

    closure->entity_length = num;
    return HTTP_DL_OK;
}

/*
 * ����-HTTP_DL_ERR_INVALID��ʾ��������Ӧֱ��������һ��;
 * ����-HTTP_DL_ERR_NOTFOUND��ʾname�뵱ǰ�в�ƥ�䣬������һ�в���;
 * ����HTTP_DL_OK����ʾ��ǰ�����������ɴ�����һ���ˡ�
 */
static int http_dl_header_process(const char *header,
                                  const char *name,
                                  int (*procfun)(const char *, void *),
                                  void *arg)
{
    const char *val;
    int gap;

    if (header == NULL
        || name == NULL
        || procfun == NULL
        || arg == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    /* Check whether HEADER matches NAME.  */
    while (*name && (tolower(*name) == tolower(*header))) {
        ++name, ++header;
    }

    if (*name || *header++ != ':') {
        return -HTTP_DL_ERR_NOTFOUND;
    }

    gap = http_dl_clac_lws(header);
    val = header;
    val += gap;

    /* ����HTTP_DL_OK��-HTTP_DL_ERR_INVALID */
    return ((*procfun)(val, arg));
}

static int http_dl_parse_header(http_dl_info_t *info)
{
    int ret;
    char *line_end;
    int hlen;
    char print_buf[HTTP_DL_BUF_LEN];
    http_dl_range_t range;

    if (info == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    if (info->stage != HTTP_DL_STAGE_PARSE_HEADER) {
        http_dl_log_debug("Wrong stage %d.", info->stage);
        return -HTTP_DL_ERR_INTERNAL;
    }

    *(info->buf_tail) = '\0';   /* �ַ�������ʱ��ȷ����Խ�� */

    while (1) {
        bzero(print_buf, HTTP_DL_BUF_LEN);
        line_end = strstr(info->buf_data, "\r\n");
        if (line_end == NULL) {
            /* header��û���յ�������һ�У�����... */
            http_dl_log_debug("Incompleted header line: %s", info->buf_data);
            break;
        }

        if (info->buf_data == line_end) {
            /* header����������޸�stageΪRECV_CONTENT������ERR_AGAIN�������½׶δ��� */
            /* XXX TODO: ���status code��reason phrase������û�к���content�ģ���ֱ��finish */
            info->stage = HTTP_DL_STAGE_RECV_CONTENT;
            info->buf_data += 2;
            return -HTTP_DL_ERR_AGAIN;
        }

        ret = http_dl_header_process(info->buf_data,
                                     "Content-Length",
                                     http_dl_header_extract_long_num,
                                     &info->content_len);
        if (ret == HTTP_DL_OK || ret == -HTTP_DL_ERR_INVALID) {
            if (info->restart_len == 0 && info->total_len == 0) {
                /* �Ƕϵ�����ʱ��total_len����content_len */
                info->total_len = info->content_len;
            }
            goto header_line_done;
        }

        ret = http_dl_header_process(info->buf_data,
                                     "Content-Type",
                                     http_dl_header_dup_str_to_buf,
                                     print_buf);
        if (ret == HTTP_DL_OK || ret == -HTTP_DL_ERR_INVALID) {
            if (strlen(print_buf) > 0) {
                http_dl_log_debug("Content-Type: %s", print_buf);
            }
            goto header_line_done;
        }

        ret = http_dl_header_process(info->buf_data,
                                     "Accept-Ranges",
                                     http_dl_header_dup_str_to_buf,
                                     print_buf);
        if (ret == HTTP_DL_OK || ret == -HTTP_DL_ERR_INVALID) {
            if (strlen(print_buf) > 0) {
                http_dl_log_debug("Accept-Ranges: %s", print_buf);
            }
            goto header_line_done;
        }

        bzero(&range, sizeof(range));
        ret = http_dl_header_process(info->buf_data,
                                     "Content-Range",
                                     http_dl_header_parse_range,
                                     &range);
        if (ret == HTTP_DL_OK) {
            /* ����range�ɹ�����鷶Χ������������xxx_len */
            if (info->restart_len != range.first_byte_pos) {
                /* XXX TODO: ��μ���??? */
                http_dl_log_error("File %s restart<%ld>, but range<%ld-%ld/%ld>",
                                    info->local,
                                    info->restart_len,
                                    range.first_byte_pos,
                                    range.last_byte_pos,
                                    range.entity_length);
            } else {
                info->total_len = range.entity_length;
            }
            http_dl_log_debug("File %s restart<%ld>, range<%ld-%ld/%ld>",
                                info->local,
                                info->restart_len,
                                range.first_byte_pos,
                                range.last_byte_pos,
                                range.entity_length);
            goto header_line_done;
        } else if (ret == -HTTP_DL_ERR_INVALID) {
            /* ����rangeʧ�� */
            /* XXX TODO */
            http_dl_log_error("Parse range failed: %s.", info->local);
            goto header_line_done;
        }

        ret = http_dl_header_process(info->buf_data,
                                     "Last-Modified",
                                     http_dl_header_dup_str_to_buf,
                                     print_buf);
        if (ret == HTTP_DL_OK || ret == -HTTP_DL_ERR_INVALID) {
            if (strlen(print_buf) > 0) {
                http_dl_log_debug("Last-Modified: %s", print_buf);
            }
            goto header_line_done;
        }

        hlen = line_end - info->buf_data;
        if (hlen > 0){
            memcpy(print_buf, info->buf_data, MINVAL(hlen, (HTTP_DL_BUF_LEN - 1)));
            http_dl_log_debug("Unsupported header: %s", print_buf);
        }

header_line_done:
        /* ��Ӧ������ɻ��������� */
        if (ret == -HTTP_DL_ERR_INVALID) {
            http_dl_log_error("Invalid header line: %s", info->buf_data);
        }
        info->buf_data = line_end + 2;
    }

    return HTTP_DL_OK;
}

static inline void http_dl_move_data(char *dst, char *src, char *end)
{
    while (src < end) {
        *dst = *src;
        dst++;
        src++;
    }
}

static void http_dl_adjust_info_buf(http_dl_info_t *info)
{
    int data_len, free_space;

    if (info == NULL) {
        http_dl_log_debug("ERROR argument is NULL.");
        return;
    }

    if (info->buf_data == info->buf_tail) {
        /* info->buf�������Ѿ�������� */
        bzero(info->buf, HTTP_DL_READBUF_LEN);
        info->buf_data = info->buf;
        info->buf_tail = info->buf;
        return;
    }

    data_len = info->buf_tail - info->buf_data;

    free_space = info->buf + HTTP_DL_READBUF_LEN - info->buf_tail;
    if (free_space < (HTTP_DL_READBUF_LEN >> 2)) {
        http_dl_log_debug("buf_tail<%p> reaching buffer end<%p>, adjust buffer...",
                            info->buf_tail, info->buf + HTTP_DL_READBUF_LEN);
        http_dl_move_data(info->buf, info->buf_data, info->buf_tail);
        info->buf_data = info->buf;
        info->buf_tail = info->buf_data + data_len;

        return;
    } else if ((free_space < (HTTP_DL_READBUF_LEN >> 1))
                && (data_len < (HTTP_DL_READBUF_LEN >> 2))) {
        http_dl_log_debug("free space [%d], and data length [%d], adjust buffer...",
                            free_space, data_len);
        http_dl_move_data(info->buf, info->buf_data, info->buf_tail);
        info->buf_data = info->buf;
        info->buf_tail = info->buf_data + data_len;

        return;
    } else {
        http_dl_log_debug("no adjustment, free[%d], data[%d], buf<%p>, tail<%p>",
                            free_space, data_len, info->buf, info->buf_tail);

        return;
    }
}

static int http_dl_flush_buf_data(http_dl_info_t *info)
{
    int data_len, ret;

    if (info == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    if (info->stage != HTTP_DL_STAGE_RECV_CONTENT) {
        http_dl_log_debug("flush buf data to file only permitted in HTTP_DL_STAGE_RECV_CONTENT.");
        info->buf_data = info->buf;
        info->buf_tail = info->buf;
        return HTTP_DL_OK;
    }

    /* ��content���ս׶Σ�ÿ��flush buf����ζ�������ݴ�������˸������غ�ʱ�� */
    http_dl_update_elapsed_time(info);

    data_len = info->buf_tail - info->buf_data;
    if (data_len == 0) {
        /* ����Ϊ�� */
        info->buf_data = info->buf;
        info->buf_tail = info->buf;
        return HTTP_DL_OK;
    } else if (data_len < 0) {
        http_dl_log_error("FATAL error, buf_tail<%p> is before buf_data<%p>, diff %d.",
                            info->buf_tail, info->buf_data, data_len);
        info->buf_tail = info->buf_data;
        return -HTTP_DL_ERR_INTERNAL;
    }

    ret = http_dl_write(info->filefd, info->buf_data, data_len);
    info->recv_len += ret;
    if (ret < data_len) {
        /* δд�� */
        info->buf_data += ret;
        return -HTTP_DL_ERR_WRITE;
    }

    /* �������ݶ�д���� */
    info->buf_data = info->buf;
    info->buf_tail = info->buf;

    return HTTP_DL_OK;
}

static int http_dl_sync_file_data(http_dl_info_t *info)
{
    int ret;

    if (info == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    ret = fsync(info->filefd);

    if (ret < 0) {
        return -HTTP_DL_ERR_FSYNC;
    } else {
        return HTTP_DL_OK;
    }
}

static int http_dl_recv_content(http_dl_info_t *info)
{
    int ret;

    if (info == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    if (info->stage != HTTP_DL_STAGE_RECV_CONTENT) {
        http_dl_log_debug("Wrong stage %d.", info->stage);
        return -HTTP_DL_ERR_INTERNAL;
    }

    if (info->buf_data == info->buf_tail) {
        http_dl_log_debug("No data in buffer.");
        return HTTP_DL_OK;
    }

    /* �����յ�content�����Ǵ����￪ʼ�ģ���ˣ���ʱ��ʼ */
    if (info->elapsed_time == 0) {
        http_dl_reset_elapsed_time(info);
    }

    ret = http_dl_flush_buf_data(info);
    if (ret != HTTP_DL_OK) {
        /* XXX TODO: flushʧ�ܣ��������������ʧ��ô??? */
        http_dl_log_error("Flush buffer data to file failed, %s.", info->local);
        http_dl_sync_file_data(info);   /* XXX TODO: ������Ҫô??? */
    }

    return HTTP_DL_OK;
}

/*
 * ����HTTP��������Ӧ��������
 * ����ֵ: -HTTP_DL_ERR_AGAIN ��ʾ�������ݻ�δ��;
 *         HTTP_DL_OK ��ʾHTTP�Ự����(��һ����ʾ��Դ���سɹ�������Ŀǰ��֧���ض����ģ���˿���ʧ��);
 *         ��������ֵ��ʾ�ײ�ʧ�ܣ�����readʧ�ܻ�ʱ������������ȡ�
 */
static int http_dl_recv_resp(http_dl_info_t *info)
{
    int ret;
    int nread, free_space;

    if (info == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    free_space = info->buf + HTTP_DL_READBUF_LEN - info->buf_tail;
    if (free_space < (HTTP_DL_READBUF_LEN >> 1)) {
        http_dl_log_info("WARNING: info buffer free space %d too small, (total %d)",
                            free_space, HTTP_DL_READBUF_LEN);
    }

    nread = read(info->sockfd, info->buf_tail, free_space);
    if (nread == 0) {
        /* HTTP�Ự��������info buffer���������ȫ��flush���ļ��У���ͬ��sync�ļ� */
        if (http_dl_flush_buf_data(info) != HTTP_DL_OK) {
            http_dl_log_debug("Flush buffer data to %s failed.", info->local);
        }
        if (http_dl_sync_file_data(info) != HTTP_DL_OK) {
            http_dl_log_debug("Sync file %s failed.", info->local);
        }

        return HTTP_DL_OK;
    } else if (nread < 0) {
        http_dl_log_error("Read failed, %d", nread);
        return -HTTP_DL_ERR_READ;
    }

    info->buf_tail += nread;

again:
    switch (info->stage) {
    case HTTP_DL_STAGE_PARSE_STATUS_LINE:
        ret = http_dl_parse_status_line(info);
        break;
    case HTTP_DL_STAGE_PARSE_HEADER:
        ret = http_dl_parse_header(info);
        break;
    case HTTP_DL_STAGE_RECV_CONTENT:
        ret = http_dl_recv_content(info);
        break;
    default:
        http_dl_log_error("Incorrect stage %d in here.", info->stage);
        return -HTTP_DL_ERR_INTERNAL;
        break;
    }

    if (ret == -HTTP_DL_ERR_AGAIN) {
        /* buffer�л����¸�stage������δ������ */
        http_dl_log_debug("Continue next stage process.");
        goto again;
    } else if (ret == HTTP_DL_OK) {
        /* XXX: ����buffer�д˴�δ����������� */
        (void)http_dl_adjust_info_buf(info);
        /* �ֽ׶λ�δ�����꣬�ȴ��´ε��������ݣ��������� */
        return -HTTP_DL_ERR_AGAIN;
    } else if (ret == -HTTP_DL_ERR_HTTP) {
        /* XXX: HTTP�Ự�������⣬ֱ�ӷ���OK������HTTP�Ự */
        return HTTP_DL_OK;
    } else {
        http_dl_log_debug("Process response failed %d.", ret);
        /* XXX TODO: ��Ҫflush buffer�е�����ô? */
        return ret;
    }
}

static int http_dl_init_ctrl_sockfd(unsigned short ctrl_port)
{
    int sockfd;
    struct sockaddr_in sa;
    socklen_t salen;

    bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(ctrl_port);
    salen = sizeof(sa);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        http_dl_log_debug("Socket failed.");
        return -HTTP_DL_ERR_SOCK;
    }

    if (bind(sockfd, (struct sockaddr *)&sa, salen) < 0) {
        http_dl_log_debug("Bind failed.");
        goto err_out;
    }

    g_http_dl_ctrl_sockfd = sockfd;
    http_dl_log_debug("http download control socket fd is %d.", sockfd);

    return HTTP_DL_OK;

err_out:
    close(sockfd);

    return -HTTP_DL_ERR_SOCK;
}

static int http_dl_is_valid_url(const char *url)
{
    if (url == NULL) {
        return 0;
    }

    /* XXX TODO */

    return 1;
}

/* Ϊinfo����TCP���� */
static int http_dl_init_sockfd(http_dl_info_t *info)
{
    int sockfd;

    if (info == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    sockfd = http_dl_conn(info->host, info->port);
    if (sockfd < 0) {
        http_dl_log_debug("Connect failed: %s:%u.", info->host, info->port);
        return sockfd;
    }
    info->sockfd = sockfd;

    return HTTP_DL_OK;
}

/*
 * ����initial���У�Ϊÿ����������info����TCP���ӣ��ɹ��Ļ�����request���У���
 * ����һ���׶�HTTP_DL_STAGE_SEND_REQUEST��
 * ʧ�ܵĻ���ֱ�ӽ���������info����finished���У�����HTTP_DL_STAGE_FINISH�׶Ρ�
 * ����finish���У����ɹ���infoɾ��������ʧ�ܵ�info���ţ��ȴ��û��鿴��
 */
static int http_dl_handle_timeout_event()
{
    http_dl_info_t *info, *next_info;
    int result;

    /* ���ȴ���initial�����е������� */
    list_for_each_entry_safe(info, next_info, &http_dl_list_initial.list, list, http_dl_info_t) {
        http_dl_del_info_from_initial_list(info);
        result = http_dl_init_sockfd(info);
        if (result == HTTP_DL_OK) {
            /* TCP�׽������ӳɹ���������һ���׶� */
            info->stage = HTTP_DL_STAGE_SEND_REQUEST;
            http_dl_add_info_to_request_list(info);
            http_dl_log_debug("Move to HTTP_DL_STAGE_SEND_REQUEST: %s.", info->url);
        } else {
            /* TCP�׽�������ʧ�ܣ�ֱ������Finish���� (����Finish����ʱ���������ԭ��err_msg) */
            info->stage = HTTP_DL_STAGE_FINISH;
            sprintf(info->err_msg, "TCP connect failed.");
            http_dl_add_info_to_finish_list(info);
            http_dl_log_debug("TCP connect failed, move to HTTP_DL_STAGE_FINISH: %s.", info->url);
        }
    }

    /* ���Ŵ���request�����г�ʱ������ */
    list_for_each_entry_safe(info, next_info, &http_dl_list_requesting.list, list, http_dl_info_t) {
        info->timeout_times++;
        if (info->timeout_times < HTTP_DL_TIMEOUT_RETRIES) {
            continue;
        }

        http_dl_del_info_from_request_list(info);
        http_dl_log_debug("Write timeout: %s.", info->url);
        info->stage = HTTP_DL_STAGE_FINISH;
        sprintf(info->err_msg, "Write timeout %d secs.", HTTP_DL_TIMEOUT_DURATION);
        http_dl_add_info_to_finish_list(info);
    }

    /* Ȼ����download�����г�ʱ������ */
    list_for_each_entry_safe(info, next_info, &http_dl_list_downloading.list, list, http_dl_info_t) {
        info->timeout_times++;
        if (info->timeout_times < HTTP_DL_TIMEOUT_RETRIES) {
            continue;
        }

        http_dl_del_info_from_download_list(info);
        http_dl_log_debug("Read timeout: %s.", info->url);
        info->stage = HTTP_DL_STAGE_FINISH;
        sprintf(info->err_msg, "Read timeout %d secs.", HTTP_DL_TIMEOUT_DURATION);
        http_dl_add_info_to_finish_list(info);
    }

    /* �����finish�������Ѿ���ɻ��������񣬿�չ��β���� */
    list_for_each_entry_safe(info, next_info, &http_dl_list_finished.list, list, http_dl_info_t) {
        http_dl_del_info_from_finish_list(info);
        result = http_dl_finish_info(info);
        /* XXX: Ŀǰ�����ǲ���ע���ս�������Ҫ�������أ������ڴ˼�����stage */
    }

    return HTTP_DL_OK;
}

/*
 * ����http_download����action��Ϣ��Ŀǰ���������������action��
 * �����յ���������ʱ����ʼ����������info�����������initial���У�
 * �׶���ΪHTTP_DL_STAGE_INIT��
 */
static int http_dl_handle_ctrl_event()
{
    struct sockaddr src_sa;
    socklen_t src_salen;
    char recv_buf[HTTP_DL_URL_LEN];
    int nrecv;
    http_dl_info_t *info;

    src_salen = sizeof(src_sa);
    bzero(&src_sa, src_salen);
    bzero(recv_buf, sizeof(recv_buf));

    http_dl_log_debug("Control event occured.");

    nrecv = recvfrom(g_http_dl_ctrl_sockfd, recv_buf, HTTP_DL_URL_LEN - 1, 0, &src_sa, &src_salen);
    if (nrecv < 0) {
        http_dl_log_debug("Recvfrom failed, return %d.", nrecv);
        return -HTTP_DL_ERR_READ;
    }

    if (!http_dl_is_valid_url(recv_buf)) {
        http_dl_log_debug("Receive invalid url: %s.", recv_buf);
        return -HTTP_DL_ERR_INVALID;
    }

    info = http_dl_create_info(recv_buf);
    if (info == NULL) {
        http_dl_log_info("Create download task failed: %s.", recv_buf);
        return -HTTP_DL_ERR_INVALID;
    }

    http_dl_add_info_to_initial_list(info);

    http_dl_log_info("Create download task %s success.", recv_buf);

    return HTTP_DL_OK;
}

/*
 * ����http��Ӧ���ɹ���ʧ�ܺ󶼽�info����finished���У�����HTTP_DL_STAGE_FINISH�׶Ρ�
 */
static int http_dl_handle_read_event(http_dl_info_t *info)
{
    int result;

    if (info == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    if (info->stage < HTTP_DL_STAGE_PARSE_STATUS_LINE || info->stage > HTTP_DL_STAGE_RECV_CONTENT) {
        http_dl_log_error("info->stage [%d] not in [%d, %d]\n", info->stage,
                                HTTP_DL_STAGE_PARSE_STATUS_LINE, HTTP_DL_STAGE_RECV_CONTENT);
        return -HTTP_DL_ERR_INTERNAL;
    }

    result = http_dl_recv_resp(info);
    if (result == -HTTP_DL_ERR_AGAIN) {
        /* XXX: ���ݻ�δ��ȡ��ϣ���������download���У�stage��http_dl_recv_resp�Լ�������� */
        return HTTP_DL_OK;
    }

    /* �����������download�������Ƴ�info */
    http_dl_del_info_from_download_list(info);

    if (result == HTTP_DL_OK) {
        /*
         * HTTP�Ự��ϣ��������سɹ���������Դ���⣬���²������أ�����ͬ������HTTP�Ự���
         * ��ʱ�ɲμ�info->status_code��info->err_msg���鿴��ϸ��Ϣ��
         */
        http_dl_log_debug("HTTP session completed %s, [%d] : %s.",
                                info->url, info->status_code, info->err_msg);
    } else {
        /* �ײ����ʧ�ܣ������ʧ�ܡ���ʱ������HTTPʧ�� */
        sprintf(info->err_msg, "Receive HTTP response failed.");
        http_dl_log_debug("Receive HTTP response failed [%d]: %s.", result, info->url);
    }

    info->stage = HTTP_DL_STAGE_FINISH;
    http_dl_add_info_to_finish_list(info);
    http_dl_log_debug("Move to HTTP_DL_STAGE_FINISH.");

    return HTTP_DL_OK;
}

/*
 * ����HTTP_DL_STAGE_SEND_REQUEST�׶ε�info��
 * ����http���󣬳ɹ���info����downloading���У�������һ��stage����ʼ����http��Ӧ��
 * ʧ�ܺ�infoֱ������finished���У�����HTTP_DL_STAGE_FINISH�׶Ρ�
 */
static int http_dl_handle_write_event(http_dl_info_t *info)
{
    int result;

    if (info == NULL) {
        return -HTTP_DL_ERR_INVALID;
    }

    if (info->stage != HTTP_DL_STAGE_SEND_REQUEST) {
        http_dl_log_error("info->stage [%d] not equals to HTTP_DL_STAGE_SEND_REQUEST [%d].",
                            info->stage, HTTP_DL_STAGE_SEND_REQUEST);
        return -HTTP_DL_ERR_INTERNAL;
    }

    http_dl_del_info_from_request_list(info);

    result = http_dl_send_req(info);
    if (result == HTTP_DL_OK) {
        /* �ɹ�����HTTP���󣬽�����һ���׶� */
        info->stage = HTTP_DL_STAGE_PARSE_STATUS_LINE;
        http_dl_add_info_to_download_list(info);
        http_dl_log_debug("Move to HTTP_DL_STAGE_PARSE_STATUS_LINE: %s.", info->url);
    } else {
        /* ����HTTP����ʧ�ܣ���info����finish���� */
        info->stage = HTTP_DL_STAGE_FINISH;
        sprintf(info->err_msg, "Send HTTP request failed.");
        http_dl_add_info_to_finish_list(info);
        http_dl_log_debug("Send HTTP request failed, move to HTTP_DL_STAGE_FINISH: %s.", info->url);
    }

    return HTTP_DL_OK;
}

/* ��Core Procedure��ֻ������ȣ����ᴦ����󣬸���handlerӦ��Ϊ�˸��� */
static int http_dl_exec_core_proc()
{
    http_dl_info_t *info, *next_info;
    struct timeval tv;
    fd_set read_set, write_set;
    int max_readfd, max_writefd, maxfd;
    int result, ret = HTTP_DL_OK;

    while (1) {
        if (g_http_dl_exit) {
            http_dl_log_info("HTTP download exit...");
            break;
        }

        /* ��ʼ����fd�� */
        max_readfd = -1;
        FD_ZERO(&read_set);
        list_for_each_entry(info, &http_dl_list_downloading.list, list, http_dl_info_t) {
            if (info->sockfd > max_readfd) {
                max_readfd = info->sockfd;
            }
            FD_SET(info->sockfd, &read_set);
        }
        /* ��http_download���ƶ˿ڼ����fd�� */
        if (g_http_dl_ctrl_sockfd > max_readfd) {
            max_readfd = g_http_dl_ctrl_sockfd;
        }
        FD_SET(g_http_dl_ctrl_sockfd, &read_set);

        /* ��ʼ��дfd�� */
        max_writefd = -1;
        FD_ZERO(&write_set);
        list_for_each_entry(info, &http_dl_list_requesting.list, list, http_dl_info_t) {
            if (info->sockfd > max_writefd) {
                max_writefd = info->sockfd;
            }
            FD_SET(info->sockfd, &write_set);
        }

        maxfd = (max_writefd > max_readfd) ? max_writefd : max_readfd;

        /* ��ʼ��select��ʱʱ�䣬ע��: ����ʱ�󣬽�ȥ����������ܱ�fd�����Ĳ��� */
        bzero(&tv, sizeof(tv));
        /* XXX TODO: timeout���ݸ�list���������̬���� */
        tv.tv_sec = HTTP_DL_SELECT_TIMEOUT;
        tv.tv_usec = 0;

        http_dl_log_debug("Select: maxfd[%d].", maxfd);
        result = select(maxfd + 1, &read_set, &write_set, NULL, &tv);
        if (result == 0) {
            /* ��ʱ��������������������� */
            http_dl_handle_timeout_event();
            continue;
        } else if (result == -1 && errno == EINTR) {
            /* ���ж� */
            http_dl_log_debug("select() interrupted by signal.");
            continue;
        } else if (result < 0) {
            http_dl_log_error("select() failed, return %d.", result);
            ret = -HTTP_DL_ERR_SELECT;
            break;
        }

        /* ���ȴ�������¼� */
        if (FD_ISSET(g_http_dl_ctrl_sockfd, &read_set)) {
            http_dl_handle_ctrl_event();
        }

        /* ���Ŵ�����¼� */
        list_for_each_entry_safe(info, next_info, &http_dl_list_downloading.list, list, http_dl_info_t) {
            if (!FD_ISSET(info->sockfd, &read_set)) {
                continue;
            }

            /* ���¼�����������info�ĳ�ʱ��¼ֵ */
            info->timeout_times = 0;
            http_dl_handle_read_event(info);
        }

        /* �����д�¼� */
        list_for_each_entry_safe(info, next_info, &http_dl_list_requesting.list, list, http_dl_info_t) {
            if (!FD_ISSET(info->sockfd, &write_set)) {
                continue;
            }

            /* д�¼�����������info�ĳ�ʱ��¼ֵ */
            info->timeout_times = 0;
            http_dl_handle_write_event(info);
        }

        /* ������һ�ε��� */
    }

    return ret;
}

#if 0
static void http_download_task(ulong argc, void *argv)
{
    http_dl_info_t *di;
    int fd, foffset, len;
    char *fname, url_buf[HTTP_DL_URL_LEN + 1], *p;

    fname = (char *)argv;
    if (fname == NULL) {
        http_dl_log_error("NULL filename ptr.");
        return;
    }

    fd = open(fname, O_RDONLY, 0644);
    if (fd < 0) {
        http_dl_log_error("Open file %s failed.", fname);
        goto err_out1;
    }
    http_dl_log_info("Open file %s, fd %d", fname, fd);

    http_dl_init();

    foffset = 0;
    while (1) {
        if (lseek(fd, foffset, SEEK_SET) == -1) {
            http_dl_log_debug("lseek cause break.");
            break;
        }

        bzero(url_buf, sizeof(url_buf));
        len = read(fd, url_buf, HTTP_DL_URL_LEN);
        if (len <= 0) {
            http_dl_log_debug("read %d <= 0 cause break.", len);
            break;
        }

        http_dl_log_debug("Read[%d/%d]:\n%s\n", len, HTTP_DL_URL_LEN, url_buf);

        p = strchr(url_buf, '\n');
        if (p == NULL) {
            if (len == HTTP_DL_URL_LEN) {
                /* ��һ��url�Ѿ�������read������ */
                http_dl_log_error("Invalid url (longer than %d, or read %d is incorrect).",
                                        HTTP_DL_URL_LEN, len);
                break;
            }
            /* �����ļ���β�ˣ�����򵥲����£��������ͳһ���� */
            p = &url_buf[len];
        }
        *p = '\0';
        foffset = foffset + p - url_buf + 1;

        di = http_dl_create_info(url_buf);
        if (di == NULL) {
            http_dl_log_info("Create download task %s failed.", url_buf);
            continue;
        }
        http_dl_add_info_to_list(di, &http_dl_list_initial);

        http_dl_log_info("Create download task %s success.", url_buf);
    }

    http_dl_debug_show();

    http_dl_list_proc_initial();

    http_dl_debug_show();

    http_dl_list_proc_downloading();

    http_dl_debug_show();

    http_dl_list_proc_finished();

    http_dl_debug_show();

    http_dl_destroy();

err_out2:
    close(fd);
err_out1:
    http_dl_free(fname);

    return;
}
#endif

static void http_download_task(ulong argc, void *argv)
{
    int ret;
    unsigned short port;

    port = HTTP_DL_CTRL_PORT_DEFAULT;

    http_dl_init();
    ret = http_dl_init_ctrl_sockfd(port);
    if (ret != HTTP_DL_OK) {
        http_dl_log_debug("Initialize control socket fd failed.");
        goto err_out;
    }

    ret = http_dl_exec_core_proc();

err_out:
    close(g_http_dl_ctrl_sockfd);
    http_dl_destroy();

    return;
}

static void http_download()
{
    if (g_http_download_task != NULL) {
        http_dl_log_error("An http download task entity is already running...");
        return;
    }

    g_http_download_task = create_task("http_download_task",
                                        http_download_task,
                                        0,
                                        NULL,
                                        16 * 1024,
                                        APP_TASK);
    if (g_http_download_task == NULL) {
        http_dl_log_error("create http download task failed");
    }

    return;
}
DEFINE_DEBUG_FUNC(http_download, http_download);

static void http_download_destroy()
{
    if (g_http_download_task == NULL) {
        return;
    }

    g_http_dl_exit = 1;

    sleep(10 * HZ);

    delete_task(g_http_download_task);
    g_http_download_task = NULL;

    g_http_dl_exit = 0;

    return;
}
DEFINE_DEBUG_FUNC(http_download_destroy, http_download_destroy);

static void http_download_log_level(int level)
{
    if (level <= 7 && level >= 0) {
        http_dl_log_level = level;
    }
}
DEFINE_DEBUG_FUNC_WITH_ARG(http_download_log_level, http_download_log_level, SUPPORT_FUNC_INT);






















#if 0
static void http_download_file_write_test(ulong argc, void *argv)
{
    int fd, ret;
    long delta_time = 0;
    int blocks, i, fail_times = 0, avrspd;
    char buf[1024];
    struct timeval tv_start, tv_end;

    if (argc != 1 || argv == NULL) {
        return;
    }

    fd = open(argv, O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        http_dl_log_error("Open file %s failed.", argv);
        goto err_out;
    }

    gettimeofday(&tv_start, NULL);

    blocks = 0x1 << 15; /* 32K blocks */
    for (i = 0; i < blocks; i++) {
        ret = write(fd, buf, sizeof(buf));
        if (ret < 0) {
            fail_times++;
        }
    }

    gettimeofday(&tv_end, NULL);

    delta_time = (tv_end.tv_sec - tv_start.tv_sec) * 1000
                 + (tv_end.tv_usec - tv_start.tv_usec) / 1000;
    avrspd = (blocks - fail_times) * sizeof(buf) / delta_time;

    http_dl_log_info("Finish: size(%d KB), time(%d secs), speed(%d KB/s), fail(%d)",
                            blocks, delta_time / 1000, avrspd, fail_times);

    close(fd);

err_out:
    http_dl_free(argv);

    return;
}

static void http_download_tcp_conn_test(ulong argc, void *argv)
{
    int sockfd = 0;

    sockfd = http_dl_conn(argv, 80);

    if (sockfd < 0) {
        http_dl_log_info("Connect failed.");
        goto err_out;
    }

    sleep(1 * HZ);

    close(sockfd);

    http_dl_log_info("Connect test finished.");

err_out:
    http_dl_free(argv);

    return;
}

static void http_write_test(char *arg)
{
    char *url;
    int url_len;

    if (g_http_write_test_task != NULL) {
        http_dl_log_error("An http write test task entity is already running...");
        return;
    }

    url_len = strlen(arg);
    if (url_len == 0) {
        return;
    }
    url_len++;

    url = http_dl_xrealloc(NULL, url_len);
    if (url == NULL) {
        http_dl_log_debug("alloc %d failed", url_len);
        return;
    }
    bzero(url, url_len);
    strcpy(url, arg);
    /* �����ڿ�ʼurl������˽�е������� */

//    g_http_write_test_task = create_task("http_download_file_write_test",
//                                          http_download_file_write_test,
    g_http_write_test_task = create_task("http_download_tcp_conn_test",
                                          http_download_tcp_conn_test,
                                          1,
                                          url,
                                          16 * 1024,
                                          APP_TASK);
    if (g_http_write_test_task == NULL) {
        http_dl_log_error("create http download task failed");
        http_dl_free(url);
        return;
    }

    return;
}
DEFINE_DEBUG_FUNC_WITH_ARG(http_write_test, http_write_test, SUPPORT_FUNC_STRING);

static void http_write_test_destroy()
{
    if (g_http_write_test_task == NULL) {
        return;
    }

    delete_task(g_http_write_test_task);
    g_http_write_test_task = NULL;

    return;
}
DEFINE_DEBUG_FUNC(http_write_test_destroy, http_write_test_destroy);
#endif

