O_TARGET := _sub_nginx.o

subdir-y += src zlib http-snooping avllib adv
subdir-y += snoop_client


obj-y := ngx_modules.o
override EXTRA_INCLUDE += -I$(TOPMK)/include \
 -I$(TOPMK)/app/nginx \
 -I$(TOPMK)/app/nginx/std \
 -I$(TOPMK)/app/nginx/src/event \
 -I$(TOPMK)/app/nginx/src/event/modules \
 -I$(TOPMK)/app/nginx/src/os/unix \
 -I$(TOPMK)/app/nginx/src/core \
 -I$(TOPMK)/app/nginx/src/http \
 -I$(TOPMK)/app/nginx/src/http/modules \
 -I$(TOPMK)/app/wtot/r_tcp/include
include $(TOPDIR)/Rules.make
