#
# CC
# CFLAGS
# LD
# LINKFLAGS
# INS
# SYM_STUB
#


include ../.compile_flag_file

objects := elf.o util.o
target := elf

default: gen_sym_stub compile convert link

gen_sym_stub:
	../tools/gen_sym_stub

compile:
	$(CC) $(CFLAGS) -c elf.c -o elf.o
	$(CC) $(CFLAGS) -c util.c -o util.o
	$(CC) $(CFLAGS) -c $(SYM_STUB).c -o $(SYM_STUB).o
	mv $(SYM_STUB).c $(SYM_STUB).c.debug

convert:
	../tools/converter $(SYM_STUB).o

link:
	$(LD) $(LINKFLAGS) $(objects) $(SYM_STUB).o -o $(target)

probe:
	$(CC) $(CFLAGS) -c elf.c -o elf.o
	$(CC) $(CFLAGS) -c util.c -o util.o
	$(LD) $(LINKFLAGS) $(objects) -o $(target)

install:
	cp $(target) $(INS)

clean:
	rm -rf $(objects) $(target) $(SYM_STUB).o $(SYM_STUB).c*

