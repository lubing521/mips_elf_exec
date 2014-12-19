O_TARGET := _sub_core.o

subdir-y :=

obj-y := nginx.o \
		 ngx_log.o \
		 ngx_palloc.o \
		 ngx_array.o \
		 ngx_list.o \
		 ngx_hash.o \
		 ngx_buf.o \
		 ngx_queue.o \
		 ngx_output_chain.o \
		 ngx_string.o \
		 ngx_parse.o \
		 ngx_inet.o \
		 ngx_file.o \
		 ngx_crc32.o \
		 ngx_murmurhash.o \
		 ngx_md5.o \
		 ngx_rbtree.o \
		 ngx_radix_tree.o \
		 ngx_slab.o \
		 ngx_times.o \
		 ngx_shmtx.o \
		 ngx_connection.o \
		 ngx_cycle.o \
		 ngx_spinlock.o \
		 ngx_cpuinfo.o \
		 ngx_conf_file.o \
		 ngx_resolver.o \
		 ngx_open_file_cache.o \
		 ngx_crypt.o \
		 ngx_regex.o

extra_include := -I$(DYNLD_TARGET_ROOT) \
		         -I$(DYNLD_TARGET_ROOT)/std \
		         -I$(DYNLD_TARGET_ROOT)/src/core \
		         -I$(DYNLD_TARGET_ROOT)/src/event \
		         -I$(DYNLD_TARGET_ROOT)/src/event/modules \
		         -I$(DYNLD_TARGET_ROOT)/src/os/unix \


include $(ROOT)/Rules.make

