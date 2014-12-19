O_TARGET := _sub_modules.o

subdir-y :=

obj-y := ngx_http_log_module.o \
		 ngx_http_static_module.o \
		 ngx_http_index_module.o \
		 ngx_http_chunked_filter_module.o \
		 ngx_http_range_filter_module.o \
		 ngx_http_headers_filter_module.o \
		 ngx_http_not_modified_filter_module.o \
		 ngx_http_gzip_filter_module.o \
		 ngx_http_ssi_filter_module.o \
		 ngx_http_charset_filter_module.o \
		 ngx_http_userid_filter_module.o \
		 ngx_http_autoindex_module.o \
		 ngx_http_access_module.o \
		 ngx_http_limit_req_module.o \
		 ngx_http_geo_module.o \
		 ngx_http_map_module.o \
		 ngx_http_split_clients_module.o \
		 ngx_http_referer_module.o \
		 ngx_http_proxy_module.o \
		 ngx_http_fastcgi_module.o \
		 ngx_http_uwsgi_module.o \
		 ngx_http_scgi_module.o \
		 ngx_http_memcached_module.o \
		 ngx_http_empty_gif_module.o \
		 ngx_http_browser_module.o \
		 ngx_http_flv_module.o \
		 ngx_http_mp4_module.o \
		 ngx_http_upstream_ip_hash_module.o \
		 ngx_http_getfile_module.o

extra_include := -I$(DYNLD_TARGET_ROOT) \
		         -I$(DYNLD_TARGET_ROOT)/std \
		         -I$(DYNLD_TARGET_ROOT)/src/core \
		         -I$(DYNLD_TARGET_ROOT)/src/event \
		         -I$(DYNLD_TARGET_ROOT)/src/event/modules \
		         -I$(DYNLD_TARGET_ROOT)/src/os/unix \
		         -I$(DYNLD_TARGET_ROOT)/src/http \
		         -I$(DYNLD_TARGET_ROOT)/src/http/modules \


include $(ROOT)/Rules.make

