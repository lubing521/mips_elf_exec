unexport DYNLD_TARGET_ROOT
unexport DYNLD_TARGET
export DYNLD_TARGET_ROOT := $(shell pwd)
export DYNLD_TARGET := nginx

O_TARGET := _sub_nginx.o

subdir-y := std src zlib http-snooping avllib snoop_client

obj-y := ngx_modules.o

extra_include := -I$(DYNLD_TARGET_ROOT) \
                 -I$(DYNLD_TARGET_ROOT)/std \
                 -I$(DYNLD_TARGET_ROOT)/src/core \
                 -I$(DYNLD_TARGET_ROOT)/src/http \
                 -I$(DYNLD_TARGET_ROOT)/src/http/modules \
                 -I$(DYNLD_TARGET_ROOT)/src/event \
                 -I$(DYNLD_TARGET_ROOT)/src/event/modules \
                 -I$(DYNLD_TARGET_ROOT)/src/os/unix


include $(ROOT)/Rules.make

