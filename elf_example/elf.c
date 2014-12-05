#include <asm/string.h>
#include <sys/print.h>
#include <sys/socket.h>
#include <sys/filio.h>
#include <net/networklayer/in.h>
#include <net/networklayer/inet_lib.h>

#include "util.h"

char *host = "115.239.211.110";
int sockfd = -1;

void dynload_entry()
{
    int ret;
    int nb;
    struct sockaddr_in sa;

    memset(&sa, 0, sizeof(sa));
    ret = inet_pton(AF_INET, host, &sa.sin_addr);
    if (ret != 1) {
        printk("inet_pton() failed.\r\n");
        return;
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons(80);

    ret = socket(AF_INET, SOCK_STREAM, 0);
    if (ret == -1) {
        printk("Create socket failed.\r\n");
        return;
    }

    nb = 1;
    if (ioctl(ret, FIONBIO, &nb) != 0) {
        close(ret);
        printk("Ioctl set non-blocking fd failed.\r\n");
        return;
    }

    if (connect(ret, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
        close(ret);
        printk("Connect failed.\r\n");
        return;
    }

    sockfd = ret;
    printk("Connect to %s success: %d.\r\n", host, sockfd);

    return;
}

void dynload_exit()
{
    if (sockfd == -1) {
        printk("Invalid sockfd.\r\n");
    } else {
        printk("Closing sockfd %d.\r\n", sockfd);
        close(sockfd);
        printk("Closed.\r\n");
    }

    return;
}

