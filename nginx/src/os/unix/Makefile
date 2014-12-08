
O_TARGET := _sub_unix.o

subdir-y :=

obj-y := ngx_alloc.o ngx_files.o ngx_process_cycle.o ngx_send.o ngx_time.o \
 ngx_channel.o ngx_posix_init.o ngx_process.o ngx_setproctitle.o ngx_udp_recv.o \
 ngx_daemon.o ngx_linux_sendfile_chain.o ngx_readv_chain.o ngx_shmem.o \
 ngx_errno.o ngx_recv.o ngx_socket.o ngx_writev_chain.o ngx_user.o
#ngx_posix_init.o 
override EXTRA_INCLUDE += -I$(TOPMK)/app/nginx/std \
 -I$(TOPMK)/app/nginx \
 -I$(TOPMK)/app/nginx/src/event \
 -I$(TOPMK)/app/nginx/src/event/modules \
 -I$(TOPMK)/app/nginx/src/os/unix \
 -I$(TOPMK)/app/nginx/src/core \
 -I$(TOPMK)/app/nginx/src/http \
 -I$(TOPMK)/app/nginx/src/http/modules
 
#ADD_CFLAGS += -DNGX_SSLVPN=1

include $(TOPDIR)/Rules.make
