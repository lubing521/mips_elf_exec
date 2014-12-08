O_TARGET := _sub_event.o

subdir-y := modules

obj-y := ngx_event_connect.o ngx_event_pipe.o \
 ngx_event_accept.o ngx_event.o ngx_event_posted.o \
 ngx_event_busy_lock.o ngx_event_timer.o
#delete ngx_event_openssl.o
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