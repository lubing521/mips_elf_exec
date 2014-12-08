O_TARGET := _sub_core.o

subdir-y :=

obj-y := nginx.o ngx_cycle.o ngx_open_file_cache.o ngx_shmtx.o \
 ngx_array.o ngx_file.o ngx_output_chain.o ngx_slab.o \
 ngx_buf.o ngx_hash.o ngx_palloc.o ngx_spinlock.o \
 ngx_conf_file.o ngx_inet.o ngx_parse.o ngx_string.o \
 ngx_connection.o ngx_list.o ngx_queue.o ngx_times.o \
 ngx_cpuinfo.o ngx_log.o ngx_radix_tree.o ngx_crc32.o \
 ngx_md5.o ngx_rbtree.o ngx_crypt.o ngx_murmurhash.o ngx_resolver.o ngx_regex.o
override EXTRA_INCLUDE += -I$(TOPMK)/include \
 -I$(TOPMK)/app/nginx \
 -I$(TOPMK)/app/nginx/std \
 -I$(TOPMK)/app/nginx/src/event \
 -I$(TOPMK)/app/nginx/src/event/modules \
 -I$(TOPMK)/app/nginx/src/os/unix \
 -I$(TOPMK)/app/nginx/src/core \
 -I$(TOPMK)/app/nginx/src/http \
 -I$(TOPMK)/app/nginx/src/http/modules
include $(TOPDIR)/Rules.make