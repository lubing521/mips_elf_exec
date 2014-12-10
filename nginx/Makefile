CFLAGS := -mlong-calls -G 0 -fno-builtin  -nostdinc -pipe -Wall -O2 -fno-delete-null-pointer-checks -fno-common -fno-schedule-insns2 -fdata-sections -ffunction-sections -EB -mabi=eabi -mlong32 -msoft-float -march=octeon -mno-octeon-useun -D_MIPS_SZLONG=32 -D_MIPS_SIM=5 -D_MIPS_SZINT=32 -D_MIPS_SZPTR=32 -DUSE_RUNTIME_MODEL_CHECKS=1 -DCVMX_ENABLE_PARAMETER_CHECKING=0 -DCVMX_ENABLE_CSR_ADDRESS_CHECKING=0 -DCVMX_ENABLE_POW_CHECKS=0

CFLAGS += -I ../../10_4_3b13_cache/include -I ../../10_4_3b13_cache/project/main/rsr1002e-rsr2004e/conf

CC = mipsisa64-octeon-elf-gcc
LINK = mipsisa64-octeon-elf-ld --undefined=dynload_exit --gc-sections

default: gen_sym_stub parse compile convert link

gen_sym_stub:
	../tools/gen_sym_stub

parse:
	../tools/parser ../symbol_table.txt
	$(CC) $(CFLAGS) -c sym_stub.c -o sym_stub.o
	mv sym_stub.c sym_stub.c.debug

convert:
	../tools/converter sym_stub.o

probe: compile fake_link

clean:
	rm -f `find . -name "*.o"`
	rm -f nginx
	rm -f sym_stub.c*

ALL_INCS = -I ./ \
		   -I std \
		   -I src/core \
		   -I src/event \
		   -I src/event/modules \
		   -I src/os/unix \
		   -I src/http \
		   -I src/http/modules \

CORE_DEPS = src/core/nginx.h \
			src/core/ngx_array.h \
			src/core/ngx_buf.h \
			src/core/ngx_conf_file.h \
			src/core/ngx_connection.h \
            src/core/ngx_core.h \
            src/core/ngx_crc32.h \
            src/core/ngx_crypt.h \
            src/core/ngx_cycle.h \
            src/core/ngx_file.h \
            src/core/ngx_hash.h \
            src/core/ngx_inet.h \
            src/core/ngx_list.h \
            src/core/ngx_log.h \
            src/core/ngx_md5.h \
            src/core/ngx_murmurhash.h \
            src/core/ngx_open_file_cache.h \
            src/core/ngx_palloc.h \
            src/core/ngx_parse.h \
            src/core/ngx_queue.h \
            src/core/ngx_radix_tree.h \
            src/core/ngx_rbtree.h \
            src/core/ngx_regex.h \
            src/core/ngx_resolver.h \
            src/core/ngx_shmtx.h \
            src/core/ngx_slab.h \
            src/core/ngx_string.h \
            src/core/ngx_times.h \
            src/event/ngx_event_busy_lock.h \
            src/event/ngx_event_connect.h \
            src/event/ngx_event.h \
            src/event/ngx_event_pipe.h \
            src/event/ngx_event_posted.h \
            src/event/ngx_event_timer.h \
			src/os/unix/ngx_alloc.h \
            src/os/unix/ngx_channel.h \
            src/os/unix/ngx_errno.h \
            src/os/unix/ngx_files.h \
            src/os/unix/ngx_process_cycle.h \
            src/os/unix/ngx_process.h \
            src/os/unix/ngx_setproctitle.h \
            src/os/unix/ngx_shmem.h \
            src/os/unix/ngx_socket.h \
            src/os/unix/ngx_time.h \
            src/os/unix/ngx_user.h \
			./ngx_auto_config.h \
			./ngx_auto_headers.h \
			zlib/crc32.h \
			zlib/deflate.h \
			zlib/gzguts.h \
			zlib/inffast.h \
			zlib/inffixed.h \
			zlib/inflate.h \
			zlib/inftrees.h \
			zlib/trees.h \
			zlib/zconf.h \
			zlib/zlib.h \
			zlib/zutil.h

CORE_INCS = -I ./ \
		    -I std \
			-I src/core \
			-I src/event \
			-I src/event/modules \
			-I src/os/unix

HTTP_DEPS = src/http/ngx_http.h \
			src/http/ngx_http_busy_lock.h \
			src/http/ngx_http_core_module.h \
			src/http/ngx_http_request.h \
			src/http/ngx_http_script.h \
			src/http/ngx_http_upstream.h \
			src/http/ngx_http_upstream_round_robin.h \
			src/http/ngx_http_variables.h \
			src/http/modules/ngx_http_ssi_filter_module.h

HTTP_INCS = -I src/http \
			-I src/http/modules

all_objects :=	src/core/nginx.o \
	src/core/ngx_log.o \
	src/core/ngx_palloc.o \
	src/core/ngx_array.o \
	src/core/ngx_list.o \
	src/core/ngx_hash.o \
	src/core/ngx_buf.o \
	src/core/ngx_queue.o \
	src/core/ngx_output_chain.o \
	src/core/ngx_string.o \
	src/core/ngx_parse.o \
	src/core/ngx_inet.o \
	src/core/ngx_file.o \
	src/core/ngx_crc32.o \
	src/core/ngx_murmurhash.o \
	src/core/ngx_md5.o \
	src/core/ngx_rbtree.o \
	src/core/ngx_radix_tree.o \
	src/core/ngx_slab.o \
	src/core/ngx_times.o \
	src/core/ngx_shmtx.o \
	src/core/ngx_connection.o \
	src/core/ngx_cycle.o \
	src/core/ngx_spinlock.o \
	src/core/ngx_cpuinfo.o \
	src/core/ngx_conf_file.o \
	src/core/ngx_resolver.o \
	src/core/ngx_open_file_cache.o \
	src/core/ngx_crypt.o \
	src/core/ngx_regex.o \
	src/event/ngx_event.o \
	src/event/ngx_event_timer.o \
	src/event/ngx_event_posted.o \
	src/event/ngx_event_busy_lock.o \
	src/event/ngx_event_accept.o \
	src/event/ngx_event_connect.o \
	src/event/ngx_event_pipe.o \
	src/event/modules/ngx_select_module.o \
	src/os/unix/ngx_time.o \
	src/os/unix/ngx_errno.o \
	src/os/unix/ngx_alloc.o \
	src/os/unix/ngx_files.o \
	src/os/unix/ngx_socket.o \
	src/os/unix/ngx_recv.o \
	src/os/unix/ngx_readv_chain.o \
	src/os/unix/ngx_udp_recv.o \
	src/os/unix/ngx_send.o \
	src/os/unix/ngx_writev_chain.o \
	src/os/unix/ngx_channel.o \
	src/os/unix/ngx_shmem.o \
	src/os/unix/ngx_process.o \
	src/os/unix/ngx_daemon.o \
	src/os/unix/ngx_setproctitle.o \
	src/os/unix/ngx_posix_init.o \
	src/os/unix/ngx_user.o \
	src/os/unix/ngx_process_cycle.o \
	src/os/unix/ngx_linux_sendfile_chain.o \
	src/http/ngx_http.o \
	src/http/ngx_http_core_module.o \
	src/http/ngx_http_special_response.o \
	src/http/ngx_http_request.o \
	src/http/ngx_http_parse.o \
	src/http/ngx_http_header_filter_module.o \
	src/http/ngx_http_write_filter_module.o \
	src/http/ngx_http_copy_filter_module.o \
	src/http/ngx_http_request_body.o \
	src/http/ngx_http_variables.o \
	src/http/ngx_http_script.o \
	src/http/ngx_http_upstream.o \
	src/http/ngx_http_upstream_round_robin.o \
	src/http/ngx_http_parse_time.o \
	src/http/ngx_http_busy_lock.o \
	src/http/ngx_http_postpone_filter_module.o \
	src/http/ngx_http_file_cache.o \
	src/http/modules/ngx_http_log_module.o \
	src/http/modules/ngx_http_static_module.o \
	src/http/modules/ngx_http_index_module.o \
	src/http/modules/ngx_http_chunked_filter_module.o \
	src/http/modules/ngx_http_range_filter_module.o \
	src/http/modules/ngx_http_headers_filter_module.o \
	src/http/modules/ngx_http_not_modified_filter_module.o \
	src/http/modules/ngx_http_gzip_filter_module.o \
	src/http/modules/ngx_http_ssi_filter_module.o \
	src/http/modules/ngx_http_charset_filter_module.o \
	src/http/modules/ngx_http_userid_filter_module.o \
	src/http/modules/ngx_http_autoindex_module.o \
	src/http/modules/ngx_http_access_module.o \
	src/http/modules/ngx_http_limit_req_module.o \
	src/http/modules/ngx_http_geo_module.o \
	src/http/modules/ngx_http_map_module.o \
	src/http/modules/ngx_http_split_clients_module.o \
	src/http/modules/ngx_http_referer_module.o \
	src/http/modules/ngx_http_proxy_module.o \
	src/http/modules/ngx_http_fastcgi_module.o \
	src/http/modules/ngx_http_uwsgi_module.o \
	src/http/modules/ngx_http_scgi_module.o \
	src/http/modules/ngx_http_memcached_module.o \
	src/http/modules/ngx_http_empty_gif_module.o \
	src/http/modules/ngx_http_browser_module.o \
	src/http/modules/ngx_http_flv_module.o \
	src/http/modules/ngx_http_mp4_module.o \
	src/http/modules/ngx_http_upstream_ip_hash_module.o \
	src/http/modules/ngx_http_getfile_module.o \
	ngx_modules.o \
	zlib/adler32.o \
	zlib/compress.o \
	zlib/crc32.o \
	zlib/deflate.o \
	zlib/example.o \
	zlib/gzclose.o \
	zlib/gzlib.o \
	zlib/gzread.o \
	zlib/gzwrite.o \
	zlib/infback.o \
	zlib/inffast.o \
	zlib/inflate.o \
	zlib/inftrees.o \
	zlib/trees.o \
	zlib/uncompr.o \
	zlib/minigzip.o \
	zlib/zutil.o \
	http-snooping/http-snooping.o \
	avllib/avlLib.o \
	avllib/strtod.o \
	snoop_client/ngx_sc_sohu.o \
	snoop_client/ngx_sc_util.o \
	snoop_client/ngx_sc_youku.o \
	snoop_client/ngx_snoop_client.o \
	snoop_client/sohu_lib.o \
	snoop_client/yk_player_plugin.o \
	snoop_client/yk_lib.o

compile: $(all_objects)

link:
	$(LINK) -o nginx \
	-T ../elf.ld \
	sym_stub.o \
	$(all_objects)

fake_link:
	$(LINK) -o nginx \
	-T ../elf.ld \
	$(all_objects)

ngx_modules.o:	$(CORE_DEPS) \
	ngx_modules.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o ngx_modules.o \
		ngx_modules.c


src/core/nginx.o:	$(CORE_DEPS) \
	src/core/nginx.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/nginx.o \
		src/core/nginx.c


src/core/ngx_log.o:	$(CORE_DEPS) \
	src/core/ngx_log.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_log.o \
		src/core/ngx_log.c


src/core/ngx_palloc.o:	$(CORE_DEPS) \
	src/core/ngx_palloc.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_palloc.o \
		src/core/ngx_palloc.c


src/core/ngx_array.o:	$(CORE_DEPS) \
	src/core/ngx_array.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_array.o \
		src/core/ngx_array.c


src/core/ngx_list.o:	$(CORE_DEPS) \
	src/core/ngx_list.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_list.o \
		src/core/ngx_list.c


src/core/ngx_hash.o:	$(CORE_DEPS) \
	src/core/ngx_hash.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_hash.o \
		src/core/ngx_hash.c


src/core/ngx_buf.o:	$(CORE_DEPS) \
	src/core/ngx_buf.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_buf.o \
		src/core/ngx_buf.c


src/core/ngx_queue.o:	$(CORE_DEPS) \
	src/core/ngx_queue.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_queue.o \
		src/core/ngx_queue.c


src/core/ngx_output_chain.o:	$(CORE_DEPS) \
	src/core/ngx_output_chain.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_output_chain.o \
		src/core/ngx_output_chain.c


src/core/ngx_string.o:	$(CORE_DEPS) \
	src/core/ngx_string.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_string.o \
		src/core/ngx_string.c


src/core/ngx_parse.o:	$(CORE_DEPS) \
	src/core/ngx_parse.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_parse.o \
		src/core/ngx_parse.c


src/core/ngx_inet.o:	$(CORE_DEPS) \
	src/core/ngx_inet.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_inet.o \
		src/core/ngx_inet.c


src/core/ngx_file.o:	$(CORE_DEPS) \
	src/core/ngx_file.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_file.o \
		src/core/ngx_file.c


src/core/ngx_crc32.o:	$(CORE_DEPS) \
	src/core/ngx_crc32.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_crc32.o \
		src/core/ngx_crc32.c


src/core/ngx_murmurhash.o:	$(CORE_DEPS) \
	src/core/ngx_murmurhash.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_murmurhash.o \
		src/core/ngx_murmurhash.c


src/core/ngx_md5.o:	$(CORE_DEPS) \
	src/core/ngx_md5.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_md5.o \
		src/core/ngx_md5.c


src/core/ngx_rbtree.o:	$(CORE_DEPS) \
	src/core/ngx_rbtree.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_rbtree.o \
		src/core/ngx_rbtree.c


src/core/ngx_radix_tree.o:	$(CORE_DEPS) \
	src/core/ngx_radix_tree.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_radix_tree.o \
		src/core/ngx_radix_tree.c


src/core/ngx_slab.o:	$(CORE_DEPS) \
	src/core/ngx_slab.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_slab.o \
		src/core/ngx_slab.c


src/core/ngx_times.o:	$(CORE_DEPS) \
	src/core/ngx_times.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_times.o \
		src/core/ngx_times.c


src/core/ngx_shmtx.o:	$(CORE_DEPS) \
	src/core/ngx_shmtx.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_shmtx.o \
		src/core/ngx_shmtx.c


src/core/ngx_connection.o:	$(CORE_DEPS) \
	src/core/ngx_connection.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_connection.o \
		src/core/ngx_connection.c


src/core/ngx_cycle.o:	$(CORE_DEPS) \
	src/core/ngx_cycle.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_cycle.o \
		src/core/ngx_cycle.c


src/core/ngx_spinlock.o:	$(CORE_DEPS) \
	src/core/ngx_spinlock.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_spinlock.o \
		src/core/ngx_spinlock.c


src/core/ngx_cpuinfo.o:	$(CORE_DEPS) \
	src/core/ngx_cpuinfo.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_cpuinfo.o \
		src/core/ngx_cpuinfo.c


src/core/ngx_conf_file.o:	$(CORE_DEPS) \
	src/core/ngx_conf_file.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_conf_file.o \
		src/core/ngx_conf_file.c


src/core/ngx_resolver.o:	$(CORE_DEPS) \
	src/core/ngx_resolver.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_resolver.o \
		src/core/ngx_resolver.c


src/core/ngx_open_file_cache.o:	$(CORE_DEPS) \
	src/core/ngx_open_file_cache.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_open_file_cache.o \
		src/core/ngx_open_file_cache.c


src/core/ngx_crypt.o:	$(CORE_DEPS) \
	src/core/ngx_crypt.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_crypt.o \
		src/core/ngx_crypt.c


src/event/ngx_event.o:	$(CORE_DEPS) \
	src/event/ngx_event.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/event/ngx_event.o \
		src/event/ngx_event.c


src/event/ngx_event_timer.o:	$(CORE_DEPS) \
	src/event/ngx_event_timer.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/event/ngx_event_timer.o \
		src/event/ngx_event_timer.c


src/event/ngx_event_posted.o:	$(CORE_DEPS) \
	src/event/ngx_event_posted.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/event/ngx_event_posted.o \
		src/event/ngx_event_posted.c


src/event/ngx_event_busy_lock.o:	$(CORE_DEPS) \
	src/event/ngx_event_busy_lock.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/event/ngx_event_busy_lock.o \
		src/event/ngx_event_busy_lock.c


src/event/ngx_event_accept.o:	$(CORE_DEPS) \
	src/event/ngx_event_accept.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/event/ngx_event_accept.o \
		src/event/ngx_event_accept.c


src/event/ngx_event_connect.o:	$(CORE_DEPS) \
	src/event/ngx_event_connect.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/event/ngx_event_connect.o \
		src/event/ngx_event_connect.c


src/event/ngx_event_pipe.o:	$(CORE_DEPS) \
	src/event/ngx_event_pipe.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/event/ngx_event_pipe.o \
		src/event/ngx_event_pipe.c


src/os/unix/ngx_time.o:	$(CORE_DEPS) \
	src/os/unix/ngx_time.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_time.o \
		src/os/unix/ngx_time.c


src/os/unix/ngx_errno.o:	$(CORE_DEPS) \
	src/os/unix/ngx_errno.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_errno.o \
		src/os/unix/ngx_errno.c


src/os/unix/ngx_alloc.o:	$(CORE_DEPS) \
	src/os/unix/ngx_alloc.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_alloc.o \
		src/os/unix/ngx_alloc.c


src/os/unix/ngx_files.o:	$(CORE_DEPS) \
	src/os/unix/ngx_files.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_files.o \
		src/os/unix/ngx_files.c


src/os/unix/ngx_socket.o:	$(CORE_DEPS) \
	src/os/unix/ngx_socket.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_socket.o \
		src/os/unix/ngx_socket.c


src/os/unix/ngx_recv.o:	$(CORE_DEPS) \
	src/os/unix/ngx_recv.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_recv.o \
		src/os/unix/ngx_recv.c


src/os/unix/ngx_readv_chain.o:	$(CORE_DEPS) \
	src/os/unix/ngx_readv_chain.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_readv_chain.o \
		src/os/unix/ngx_readv_chain.c


src/os/unix/ngx_udp_recv.o:	$(CORE_DEPS) \
	src/os/unix/ngx_udp_recv.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_udp_recv.o \
		src/os/unix/ngx_udp_recv.c


src/os/unix/ngx_send.o:	$(CORE_DEPS) \
	src/os/unix/ngx_send.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_send.o \
		src/os/unix/ngx_send.c


src/os/unix/ngx_writev_chain.o:	$(CORE_DEPS) \
	src/os/unix/ngx_writev_chain.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_writev_chain.o \
		src/os/unix/ngx_writev_chain.c


src/os/unix/ngx_channel.o:	$(CORE_DEPS) \
	src/os/unix/ngx_channel.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_channel.o \
		src/os/unix/ngx_channel.c


src/os/unix/ngx_shmem.o:	$(CORE_DEPS) \
	src/os/unix/ngx_shmem.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_shmem.o \
		src/os/unix/ngx_shmem.c


src/os/unix/ngx_process.o:	$(CORE_DEPS) \
	src/os/unix/ngx_process.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_process.o \
		src/os/unix/ngx_process.c


src/os/unix/ngx_daemon.o:	$(CORE_DEPS) \
	src/os/unix/ngx_daemon.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_daemon.o \
		src/os/unix/ngx_daemon.c


src/os/unix/ngx_setaffinity.o:	$(CORE_DEPS) \
	src/os/unix/ngx_setaffinity.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_setaffinity.o \
		src/os/unix/ngx_setaffinity.c


src/os/unix/ngx_setproctitle.o:	$(CORE_DEPS) \
	src/os/unix/ngx_setproctitle.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_setproctitle.o \
		src/os/unix/ngx_setproctitle.c


src/os/unix/ngx_posix_init.o:	$(CORE_DEPS) \
	src/os/unix/ngx_posix_init.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_posix_init.o \
		src/os/unix/ngx_posix_init.c


src/os/unix/ngx_user.o:	$(CORE_DEPS) \
	src/os/unix/ngx_user.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_user.o \
		src/os/unix/ngx_user.c


src/os/unix/ngx_process_cycle.o:	$(CORE_DEPS) \
	src/os/unix/ngx_process_cycle.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_process_cycle.o \
		src/os/unix/ngx_process_cycle.c


src/os/unix/ngx_linux_init.o:	$(CORE_DEPS) \
	src/os/unix/ngx_linux_init.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_linux_init.o \
		src/os/unix/ngx_linux_init.c


src/event/modules/ngx_select_module.o:	$(CORE_DEPS) \
	src/event/modules/ngx_select_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/event/modules/ngx_select_module.o \
		src/event/modules/ngx_select_module.c


src/os/unix/ngx_linux_sendfile_chain.o:	$(CORE_DEPS) \
	src/os/unix/ngx_linux_sendfile_chain.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/os/unix/ngx_linux_sendfile_chain.o \
		src/os/unix/ngx_linux_sendfile_chain.c


src/core/ngx_regex.o:	$(CORE_DEPS) \
	src/core/ngx_regex.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) \
		-o src/core/ngx_regex.o \
		src/core/ngx_regex.c


src/http/ngx_http.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http.o \
		src/http/ngx_http.c


src/http/ngx_http_core_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http_core_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http_core_module.o \
		src/http/ngx_http_core_module.c


src/http/ngx_http_special_response.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http_special_response.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http_special_response.o \
		src/http/ngx_http_special_response.c


src/http/ngx_http_request.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http_request.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http_request.o \
		src/http/ngx_http_request.c


src/http/ngx_http_parse.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http_parse.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http_parse.o \
		src/http/ngx_http_parse.c


src/http/ngx_http_header_filter_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http_header_filter_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http_header_filter_module.o \
		src/http/ngx_http_header_filter_module.c


src/http/ngx_http_write_filter_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http_write_filter_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http_write_filter_module.o \
		src/http/ngx_http_write_filter_module.c


src/http/ngx_http_copy_filter_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http_copy_filter_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http_copy_filter_module.o \
		src/http/ngx_http_copy_filter_module.c


src/http/modules/ngx_http_log_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_log_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_log_module.o \
		src/http/modules/ngx_http_log_module.c


src/http/ngx_http_request_body.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http_request_body.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http_request_body.o \
		src/http/ngx_http_request_body.c


src/http/ngx_http_variables.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http_variables.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http_variables.o \
		src/http/ngx_http_variables.c


src/http/ngx_http_script.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http_script.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http_script.o \
		src/http/ngx_http_script.c


src/http/ngx_http_upstream.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http_upstream.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http_upstream.o \
		src/http/ngx_http_upstream.c


src/http/ngx_http_upstream_round_robin.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http_upstream_round_robin.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http_upstream_round_robin.o \
		src/http/ngx_http_upstream_round_robin.c


src/http/ngx_http_parse_time.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http_parse_time.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http_parse_time.o \
		src/http/ngx_http_parse_time.c


src/http/ngx_http_file_cache.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http_file_cache.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http_file_cache.o \
		src/http/ngx_http_file_cache.c


src/http/modules/ngx_http_static_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_static_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_static_module.o \
		src/http/modules/ngx_http_static_module.c


src/http/modules/ngx_http_index_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_index_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_index_module.o \
		src/http/modules/ngx_http_index_module.c


src/http/modules/ngx_http_chunked_filter_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_chunked_filter_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_chunked_filter_module.o \
		src/http/modules/ngx_http_chunked_filter_module.c


src/http/modules/ngx_http_range_filter_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_range_filter_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_range_filter_module.o \
		src/http/modules/ngx_http_range_filter_module.c


src/http/modules/ngx_http_headers_filter_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_headers_filter_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_headers_filter_module.o \
		src/http/modules/ngx_http_headers_filter_module.c


src/http/modules/ngx_http_not_modified_filter_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_not_modified_filter_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_not_modified_filter_module.o \
		src/http/modules/ngx_http_not_modified_filter_module.c


src/http/ngx_http_busy_lock.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http_busy_lock.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http_busy_lock.o \
		src/http/ngx_http_busy_lock.c


src/http/modules/ngx_http_gzip_filter_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_gzip_filter_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_gzip_filter_module.o \
		src/http/modules/ngx_http_gzip_filter_module.c


src/http/ngx_http_postpone_filter_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/ngx_http_postpone_filter_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/ngx_http_postpone_filter_module.o \
		src/http/ngx_http_postpone_filter_module.c


src/http/modules/ngx_http_ssi_filter_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_ssi_filter_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_ssi_filter_module.o \
		src/http/modules/ngx_http_ssi_filter_module.c


src/http/modules/ngx_http_charset_filter_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_charset_filter_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_charset_filter_module.o \
		src/http/modules/ngx_http_charset_filter_module.c


src/http/modules/ngx_http_userid_filter_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_userid_filter_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_userid_filter_module.o \
		src/http/modules/ngx_http_userid_filter_module.c


src/http/modules/ngx_http_autoindex_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_autoindex_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_autoindex_module.o \
		src/http/modules/ngx_http_autoindex_module.c


src/http/modules/ngx_http_auth_basic_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_auth_basic_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_auth_basic_module.o \
		src/http/modules/ngx_http_auth_basic_module.c


src/http/modules/ngx_http_access_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_access_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_access_module.o \
		src/http/modules/ngx_http_access_module.c


src/http/modules/ngx_http_limit_conn_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_limit_conn_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_limit_conn_module.o \
		src/http/modules/ngx_http_limit_conn_module.c


src/http/modules/ngx_http_limit_req_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_limit_req_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_limit_req_module.o \
		src/http/modules/ngx_http_limit_req_module.c


src/http/modules/ngx_http_geo_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_geo_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_geo_module.o \
		src/http/modules/ngx_http_geo_module.c


src/http/modules/ngx_http_map_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_map_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_map_module.o \
		src/http/modules/ngx_http_map_module.c


src/http/modules/ngx_http_split_clients_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_split_clients_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_split_clients_module.o \
		src/http/modules/ngx_http_split_clients_module.c


src/http/modules/ngx_http_referer_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_referer_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_referer_module.o \
		src/http/modules/ngx_http_referer_module.c


src/http/modules/ngx_http_rewrite_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_rewrite_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_rewrite_module.o \
		src/http/modules/ngx_http_rewrite_module.c


src/http/modules/ngx_http_proxy_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_proxy_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_proxy_module.o \
		src/http/modules/ngx_http_proxy_module.c


src/http/modules/ngx_http_fastcgi_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_fastcgi_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_fastcgi_module.o \
		src/http/modules/ngx_http_fastcgi_module.c


src/http/modules/ngx_http_uwsgi_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_uwsgi_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_uwsgi_module.o \
		src/http/modules/ngx_http_uwsgi_module.c


src/http/modules/ngx_http_scgi_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_scgi_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_scgi_module.o \
		src/http/modules/ngx_http_scgi_module.c


src/http/modules/ngx_http_memcached_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_memcached_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_memcached_module.o \
		src/http/modules/ngx_http_memcached_module.c


src/http/modules/ngx_http_empty_gif_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_empty_gif_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_empty_gif_module.o \
		src/http/modules/ngx_http_empty_gif_module.c


src/http/modules/ngx_http_browser_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_browser_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_browser_module.o \
		src/http/modules/ngx_http_browser_module.c


src/http/modules/ngx_http_flv_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_flv_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_flv_module.o \
		src/http/modules/ngx_http_flv_module.c


src/http/modules/ngx_http_mp4_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_mp4_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_mp4_module.o \
		src/http/modules/ngx_http_mp4_module.c


src/http/modules/ngx_http_upstream_ip_hash_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_upstream_ip_hash_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_upstream_ip_hash_module.o \
		src/http/modules/ngx_http_upstream_ip_hash_module.c


src/http/modules/ngx_http_upstream_least_conn_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_upstream_least_conn_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_upstream_least_conn_module.o \
		src/http/modules/ngx_http_upstream_least_conn_module.c


src/http/modules/ngx_http_upstream_keepalive_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_upstream_keepalive_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_upstream_keepalive_module.o \
		src/http/modules/ngx_http_upstream_keepalive_module.c


src/http/modules/ngx_http_getfile_module.o:	$(CORE_DEPS) $(HTTP_DEPS) \
	src/http/modules/ngx_http_getfile_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(HTTP_INCS) \
		-o src/http/modules/ngx_http_getfile_module.o \
		src/http/modules/ngx_http_getfile_module.c


zlib/adler32.o: zlib/adler32.c
	$(CC) -c $(CFLAGS) -o zlib/adler32.o zlib/adler32.c

zlib/compress.o: zlib/compress.c
	$(CC) -c $(CFLAGS) -o zlib/compress.o zlib/compress.c

zlib/zutil.o: zlib/zutil.c
	$(CC) -c $(CFLAGS) -o zlib/zutil.o zlib/zutil.c

zlib/uncompr.o: zlib/uncompr.c
	$(CC) -c $(CFLAGS) -o zlib/uncompr.o zlib/uncompr.c


zlib/trees.o: zlib/trees.c
	$(CC) -c $(CFLAGS) -o zlib/trees.o zlib/trees.c


zlib/inftrees.o: zlib/inftrees.c
	$(CC) -c $(CFLAGS) -o zlib/inftrees.o zlib/inftrees.c


zlib/minigzip.o: zlib/minigzip.c
	$(CC) -c $(CFLAGS) $(ALL_INCS) -o zlib/minigzip.o zlib/minigzip.c


zlib/inflate.o: zlib/inflate.c
	$(CC) -c $(CFLAGS) -o zlib/inflate.o zlib/inflate.c


zlib/inffast.o: zlib/inffast.c
	$(CC) -c $(CFLAGS) -o zlib/inffast.o zlib/inffast.c


zlib/infback.o: zlib/infback.c
	$(CC) -c $(CFLAGS) -o zlib/infback.o zlib/infback.c


zlib/gzwrite.o: zlib/gzwrite.c
	$(CC) -c $(CFLAGS) -o zlib/gzwrite.o zlib/gzwrite.c


zlib/gzread.o: zlib/gzread.c
	$(CC) -c $(CFLAGS) -o zlib/gzread.o zlib/gzread.c


zlib/gzlib.o: zlib/gzlib.c
	$(CC) -c $(CFLAGS) -o zlib/gzlib.o zlib/gzlib.c


zlib/gzclose.o: zlib/gzclose.c
	$(CC) -c $(CFLAGS) -o zlib/gzclose.o zlib/gzclose.c


zlib/example.o: zlib/example.c
	$(CC) -c $(CFLAGS) -o zlib/example.o zlib/example.c


zlib/deflate.o: zlib/deflate.c
	$(CC) -c $(CFLAGS) -o zlib/deflate.o zlib/deflate.c


zlib/crc32.o: zlib/crc32.c
	$(CC) -c $(CFLAGS) -o zlib/crc32.o zlib/crc32.c


http-snooping/http-snooping.o: http-snooping/http-snooping.c
	$(CC) -c $(CFLAGS) -I ../../10_4_3b13_cache/app/wtot/r_tcp/include $(ALL_INCS) \
	-o http-snooping/http-snooping.o http-snooping/http-snooping.c


avllib/avlLib.o: avllib/avlLib.c
	$(CC) -c $(CFLAGS) $(ALL_INCS) -o avllib/avlLib.o avllib/avlLib.c


avllib/strtod.o: avllib/strtod.c
	$(CC) -c $(CFLAGS) $(ALL_INCS) -o avllib/strtod.o avllib/strtod.c


snoop_client/ngx_sc_sohu.o: snoop_client/ngx_sc_sohu.c
	$(CC) -c $(CFLAGS) $(ALL_INCS) -o snoop_client/ngx_sc_sohu.o snoop_client/ngx_sc_sohu.c


snoop_client/ngx_sc_util.o: snoop_client/ngx_sc_util.c
	$(CC) -c $(CFLAGS) $(ALL_INCS) -o snoop_client/ngx_sc_util.o snoop_client/ngx_sc_util.c


snoop_client/ngx_sc_youku.o: snoop_client/ngx_sc_youku.c
	$(CC) -c $(CFLAGS) $(ALL_INCS) -o snoop_client/ngx_sc_youku.o snoop_client/ngx_sc_youku.c


snoop_client/ngx_snoop_client.o: snoop_client/ngx_snoop_client.c
	$(CC) -c $(CFLAGS) $(ALL_INCS) -o snoop_client/ngx_snoop_client.o snoop_client/ngx_snoop_client.c


snoop_client/sohu_lib.o: snoop_client/sohu_lib.c
	$(CC) -c $(CFLAGS) $(ALL_INCS) -o snoop_client/sohu_lib.o snoop_client/sohu_lib.c


snoop_client/yk_player_plugin.o: snoop_client/yk_player_plugin.c
	$(CC) -c $(CFLAGS) $(ALL_INCS) -o snoop_client/yk_player_plugin.o snoop_client/yk_player_plugin.c


snoop_client/yk_lib.o: snoop_client/yk_lib.c
	$(CC) -c $(CFLAGS) $(ALL_INCS) -o snoop_client/yk_lib.o snoop_client/yk_lib.c
