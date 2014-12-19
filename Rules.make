#!/bin/make

unexport obj-y
unexport subdir-y
unexport O_TARGET
unexport extra_include
unexport extra_cflags
unexport extra_linkflags

subdir_obj := $(join $(subdir-y), $(patsubst %,/_sub_%.o,$(notdir $(subdir-y))))


# 连接成部分.o
$(O_TARGET): $(obj-y) $(subdir_obj)
	$(LD) -r $(obj-y) $(subdir_obj) -o $@
ifeq ($(DYNLD_TARGET_ROOT), $(shell pwd))
	make final
endif

# 编译所有.o
$(obj-y):%.o: %.c
	$(CC) $(CFLAGS) $(extra_include) -c $< -o $@

# 编译所有子文件下的.o
$(subdir_obj):
	make -C $(dir $@)


.PHONY: final gen_sym_stub link probe install clean

final: gen_sym_stub link

gen_sym_stub:
	$(GEN_SYM)
	$(CC) $(CFLAGS) -c $(STUB_CODE) -o $(STUB_OBJECT)
	mv $(STUB_CODE) $(STUB_CODE_DBG)
	$(CONVERT_SYM) $(STUB_OBJECT)

link: $(O_TARGET)
	$(LD) $(LINKFLAGS) $< $(STUB_OBJECT) -o $(DYNLD_TARGET)

# 第一次链接，找出未定义的外部符号
probe: $(O_TARGET)
	$(LD) $(LINKFLAGS) $< -o $(DYNLD_TARGET)

install:
	cp $(DYNLD_TARGET) $(INS)

clean:
	rm -rf $(O_TARGET) $(shell find . -name "*.o") $(DYNLD_TARGET) $(SYM_STUB).*

