O_TARGET := _sub_unix.o

subdir-y :=

obj-y := ngx_time.o \
		 ngx_errno.o \
		 ngx_alloc.o \
		 ngx_files.o \
		 ngx_socket.o \
		 ngx_recv.o \
		 ngx_readv_chain.o \
		 ngx_udp_recv.o \
		 ngx_send.o \
		 ngx_writev_chain.o \
		 ngx_channel.o \
		 ngx_shmem.o \
		 ngx_process.o \
		 ngx_daemon.o \
		 ngx_setproctitle.o \
		 ngx_posix_init.o \
		 ngx_user.o \
		 ngx_process_cycle.o \
		 ngx_linux_sendfile_chain.o

extra_include := -I$(DYNLD_TARGET_ROOT) \
		         -I$(DYNLD_TARGET_ROOT)/std \
		         -I$(DYNLD_TARGET_ROOT)/src/core \
		         -I$(DYNLD_TARGET_ROOT)/src/event \
		         -I$(DYNLD_TARGET_ROOT)/src/event/modules \
		         -I$(DYNLD_TARGET_ROOT)/src/os/unix


include $(ROOT)/Rules.make

