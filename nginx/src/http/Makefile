O_TARGET := _sub_http.o

subdir-y := modules

obj-y := ngx_http.o \
		 ngx_http_core_module.o \
		 ngx_http_special_response.o \
		 ngx_http_request.o \
		 ngx_http_parse.o \
		 ngx_http_header_filter_module.o \
		 ngx_http_write_filter_module.o \
		 ngx_http_copy_filter_module.o \
		 ngx_http_request_body.o \
		 ngx_http_variables.o \
		 ngx_http_script.o \
		 ngx_http_upstream.o \
		 ngx_http_upstream_round_robin.o \
		 ngx_http_parse_time.o \
		 ngx_http_busy_lock.o \
		 ngx_http_postpone_filter_module.o \
		 ngx_http_file_cache.o

extra_include := -I$(DYNLD_TARGET_ROOT) \
		         -I$(DYNLD_TARGET_ROOT)/std \
		         -I$(DYNLD_TARGET_ROOT)/src/core \
		         -I$(DYNLD_TARGET_ROOT)/src/event \
		         -I$(DYNLD_TARGET_ROOT)/src/event/modules \
		         -I$(DYNLD_TARGET_ROOT)/src/os/unix \
		         -I$(DYNLD_TARGET_ROOT)/src/http \
		         -I$(DYNLD_TARGET_ROOT)/src/http/modules \


include $(ROOT)/Rules.make

