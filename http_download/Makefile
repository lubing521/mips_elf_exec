cflags := -mlong-calls -G 0 -fno-builtin  -nostdinc -pipe -Wall -O2 -fno-delete-null-pointer-checks -fno-common -fno-schedule-insns2 -fdata-sections -ffunction-sections -EB -mabi=eabi -mlong32 -msoft-float -march=octeon -mno-octeon-useun -D_MIPS_SZLONG=32 -D_MIPS_SIM=5 -D_MIPS_SZINT=32 -D_MIPS_SZPTR=32 -DUSE_RUNTIME_MODEL_CHECKS=1 -DCVMX_ENABLE_PARAMETER_CHECKING=0  -DCVMX_ENABLE_CSR_ADDRESS_CHECKING=0 -DCVMX_ENABLE_POW_CHECKS=0

cflags += -I ../../10_4_3b13_cache/include -I ../../10_4_3b13_cache/project/main/rsr1002e-rsr2004e/conf 

default: gen_sym_stub compile convert link

gen_sym_stub:
	../tools/gen_sym_stub

compile:
	mipsisa64-octeon-elf-gcc $(cflags) -c http_download.c -o http_download.o
	mipsisa64-octeon-elf-gcc $(cflags) -c sym_stub.c -o sym_stub.o
	mv sym_stub.c sym_stub.c.debug

convert:
	../tools/converter sym_stub.o

link:
	mipsisa64-octeon-elf-ld --gc-sections -T ../elf.ld http_download.o sym_stub.o -o http_download

probe:
	mipsisa64-octeon-elf-gcc $(cflags) -c http_download.c -o http_download.o
	mipsisa64-octeon-elf-ld --gc-sections -T ../elf.ld http_download.o -o http_download

clean:
	rm -rf http_download.o http_download sym_stub.o sym_stub.c*
