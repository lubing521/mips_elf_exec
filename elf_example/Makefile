cflags := -G 0 -fno-builtin  -nostdinc -pipe -Wall -O2 -fno-delete-null-pointer-checks -fno-common -fno-schedule-insns2 -fdata-sections -ffunction-sections -EB -mabi=eabi -mlong32 -msoft-float -march=octeon -mno-octeon-useun -D_MIPS_SZLONG=32 -D_MIPS_SIM=5 -D_MIPS_SZINT=32 -D_MIPS_SZPTR=32 -DUSE_RUNTIME_MODEL_CHECKS=1 -DCVMX_ENABLE_PARAMETER_CHECKING=0  -DCVMX_ENABLE_CSR_ADDRESS_CHECKING=0 -DCVMX_ENABLE_POW_CHECKS=0

default: compile link

compile:
	mipsisa64-octeon-elf-gcc $(cflags) -c elf.c -o elf.o

link:
	mipsisa64-octeon-elf-ld -T ../elf.ld elf.o -o elf

clean:
	rm -rf elf.o elf
