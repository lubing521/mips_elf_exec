O_TARGET := _sub_src.o

subdir-y += core event http os

obj-y := 
override EXTRA_INCLUDE += -I$(TOPMK)/include \
 -I$(TOPMK)/app/nginx \
 -I$(TOPMK)/app/nginx/zlib \
 -I$(TOPMK)/app/nginx/std \
 -I$(TOPMK)/app/nginx/src/event \
 -I$(TOPMK)/app/nginx/src/event/modules \
 -I$(TOPMK)/app/nginx/src/os/unix \
 -I$(TOPMK)/app/nginx/src/core \
 -I$(TOPMK)/app/nginx/src/http \
 -I$(TOPMK)/app/nginx/src/http/modules
 
include $(TOPDIR)/Rules.make

