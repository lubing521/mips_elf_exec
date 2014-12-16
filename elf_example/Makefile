include ../.compile_flag_file

default: gen_sym_stub compile convert link

gen_sym_stub:
	../tools/gen_sym_stub

compile:
	$(CC) $(CFLAGS) -c elf.c -o elf.o
	$(CC) $(CFLAGS) -c util.c -o util.o
	$(CC) $(CFLAGS) -c sym_stub.c -o sym_stub.o
	mv sym_stub.c sym_stub.c.debug

convert:
	../tools/converter sym_stub.o

link:
	$(LD) $(LINKFLAGS) elf.o sym_stub.o util.o -o elf

probe:
	$(CC) $(CFLAGS) -c elf.c -o elf.o
	$(CC) $(CFLAGS) -c util.c -o util.o
	$(LD) $(LINKFLAGS) elf.o util.o -o elf

install:
	cp elf $(INS)

clean:
	rm -rf elf.o util.o elf sym_stub.o sym_stub.c*

