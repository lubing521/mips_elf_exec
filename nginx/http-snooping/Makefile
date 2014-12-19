O_TARGET := _sub_http-snooping.o

subdir-y :=

obj-y := http-snooping.o

extra_include := -I$(DYNLD_TARGET_ROOT) \
		 		 -I$(DYNLD_TARGET_ROOT)/std \
		 		 -I$(DYNLD_TARGET_ROOT)/src/core \
		 		 -I$(DYNLD_TARGET_ROOT)/src/event \
		 		 -I$(DYNLD_TARGET_ROOT)/src/event/modules \
		 		 -I$(DYNLD_TARGET_ROOT)/src/os/unix \
				 -I$(DYNLD_TARGET_ROOT)/src/http \
				 -I$(DYNLD_TARGET_ROOT)/src/http/modules \
				 -I$(RGOS_ROOT_PATH)/app/wtot/r_tcp/include


include $(ROOT)/Rules.make

