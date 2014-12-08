#ifndef _SSLVPN_STD_LIB_H_
#define  _SSLVPN_STD_LIB_H_

#ifndef SSL_MALLOC
#define SSL_MALLOC
#define malloc(size)                kmalloc((size),0)
#define free(ptr)                    kfree(ptr)
#define remove(ptr)                        unlink(ptr)
#endif

#endif


