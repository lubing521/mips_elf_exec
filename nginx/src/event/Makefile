O_TARGET := _sub_event.o

subdir-y := modules

obj-y := ngx_event.o \
		 ngx_event_timer.o \
		 ngx_event_posted.o \
		 ngx_event_busy_lock.o \
		 ngx_event_accept.o \
		 ngx_event_connect.o \
		 ngx_event_pipe.o

extra_include := -I$(DYNLD_TARGET_ROOT) \
		         -I$(DYNLD_TARGET_ROOT)/std \
		         -I$(DYNLD_TARGET_ROOT)/src/core \
		         -I$(DYNLD_TARGET_ROOT)/src/event \
		         -I$(DYNLD_TARGET_ROOT)/src/event/modules \
		         -I$(DYNLD_TARGET_ROOT)/src/os/unix \


include $(ROOT)/Rules.make

