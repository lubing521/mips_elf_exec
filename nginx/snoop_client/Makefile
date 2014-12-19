O_TARGET := _sub_snoop_client.o

subdir-y :=

obj-y := ngx_sc_sohu.o ngx_sc_util.o ngx_sc_youku.o ngx_snoop_client.o sohu_lib.o yk_lib.o yk_player_plugin.o

extra_include := -I$(DYNLD_TARGET_ROOT) \
		 		 -I$(DYNLD_TARGET_ROOT)/std \
		 		 -I$(DYNLD_TARGET_ROOT)/src/core \
		 		 -I$(DYNLD_TARGET_ROOT)/src/event \
		 		 -I$(DYNLD_TARGET_ROOT)/src/event/modules \
		 		 -I$(DYNLD_TARGET_ROOT)/src/os/unix \
				 -I$(DYNLD_TARGET_ROOT)/src/http \
				 -I$(DYNLD_TARGET_ROOT)/src/http/modules \


include $(ROOT)/Rules.make

