include ../.compile_flag_file

default: gen_sym_stub compile convert link

gen_sym_stub:
	../tools/gen_sym_stub

compile:
	mipsisa64-octeon-elf-gcc $(cflags) -c elf.c -o elf.o
	mipsisa64-octeon-elf-gcc $(cflags) -c util.c -o util.o
	mipsisa64-octeon-elf-gcc $(cflags) -c sym_stub.c -o sym_stub.o
	mv sym_stub.c sym_stub.c.debug

convert:
	../tools/converter sym_stub.o

link:
	mipsisa64-octeon-elf-ld --undefined=dynload_exit --gc-sections -T ../elf.ld elf.o sym_stub.o util.o -o elf

probe:
	mipsisa64-octeon-elf-gcc $(cflags) -c elf.c -o elf.o
	mipsisa64-octeon-elf-gcc $(cflags) -c util.c -o util.o
	mipsisa64-octeon-elf-ld --undefined=dynload_exit --gc-sections -T ../elf.ld elf.o util.o -o elf
	

clean:
	rm -rf elf.o util.o elf sym_stub.o sym_stub.c*
