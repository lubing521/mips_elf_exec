O_TARGET := _sub_zlib.o

subdir-y :=

obj-y := adler32.o \
		 compress.o \
		 crc32.o \
		 deflate.o \
		 gzclose.o \
		 gzlib.o \
		 gzread.o \
		 gzwrite.o \
		 infback.o \
		 inffast.o \
		 inflate.o \
		 inftrees.o \
		 trees.o \
		 uncompr.o \
		 minigzip.o \
		 zutil.o

extra_include := -I$(DYNLD_TARGET_ROOT) \
				 -I$(DYNLD_TARGET_ROOT)/std \
				 -I$(DYNLD_TARGET_ROOT)/src/core \
				 -I$(DYNLD_TARGET_ROOT)/src/event \
				 -I$(DYNLD_TARGET_ROOT)/src/event/modules \
				 -I$(DYNLD_TARGET_ROOT)/src/os/unix \


include $(ROOT)/Rules.make

