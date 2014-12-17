include ../.compile_flag_file

OBJ_CORE := ngx_modules.o \
	src/core/nginx.o \
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
	src/core/ngx_regex.o

OBJ_EVENT := src/event/ngx_event.o \
	src/event/ngx_event_timer.o \
	src/event/ngx_event_posted.o \
	src/event/ngx_event_busy_lock.o \
	src/event/ngx_event_accept.o \
	src/event/ngx_event_connect.o \
	src/event/ngx_event_pipe.o \
	src/event/modules/ngx_select_module.o

OBJ_OS := src/os/unix/ngx_time.o \
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
	src/os/unix/ngx_linux_sendfile_chain.o

OBJ_HTTP := src/http/ngx_http.o \
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
	src/http/modules/ngx_http_getfile_module.o

OBJ_ZLIB := zlib/adler32.o \
	zlib/compress.o \
	zlib/crc32.o \
	zlib/deflate.o \
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
	zlib/zutil.o

OBJ_SP := http-snooping/http-snooping.o

OBJ_SPC := snoop_client/ngx_sc_sohu.o \
	snoop_client/ngx_sc_util.o \
	snoop_client/ngx_sc_youku.o \
	snoop_client/ngx_snoop_client.o \
	snoop_client/sohu_lib.o \
	snoop_client/yk_player_plugin.o \
	snoop_client/yk_lib.o

OBJ_AVL := avllib/avlLib.o avllib/strtod.o

OBJ_STD := std/stdlib.o

objects := $(OBJ_CORE) $(OBJ_EVENT) $(OBJ_OS) $(OBJ_HTTP) $(OBJ_ZLIB) $(OBJ_SP) $(OBJ_SPC) $(OBJ_AVL) $(OBJ_STD)
target := nginx

default: gen_sym_stub stub link

gen_sym_stub:
	../tools/gen_sym_stub

stub:
	$(CC) $(CFLAGS) -c $(SYM_STUB).c -o $(SYM_STUB).o
	mv $(SYM_STUB).c $(SYM_STUB).c.debug
	../tools/converter $(SYM_STUB).o

link: $(objects)
	$(LD) $(LINKFLAGS) $(objects) $(SYM_STUB).o -o $(target)

probe: $(objects)
	$(LD) $(LINKFLAGS) $(objects) -o $(target)

INCS_BASE := -I ./ -I std
INCS_HTTP := -I src/http -I src/http/modules

INCS_SP := $(INCS_HTTP) -I $(RGOS_ROOT_PATH)/app/wtot/r_tcp/include
INCS_CORE := $(INCS_BASE) \
		   -I src/core \
		   -I src/event \
		   -I src/event/modules \
		   -I src/os/unix

$(OBJ_CORE) $(OBJ_EVENT) $(OBJ_OS) $(OBJ_ZLIB) $(OBJ_AVL): %.o: %.c
	$(CC) $(CFLAGS) $(INCS_CORE) -c $< -o $@

$(OBJ_HTTP): %.o: %.c
	$(CC) $(CFLAGS) $(INCS_CORE) $(INCS_HTTP) -c $< -o $@

$(OBJ_SP) $(OBJ_SPC): %.o: %.c
	$(CC) $(CFLAGS) $(INCS_CORE) $(INCS_SP) -c $< -o $@

$(OBJ_STD): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: install clean

install:
	cp $(target) $(INS)

clean:
	rm -rf $(objects) $(target) $(SYM_STUB).*
