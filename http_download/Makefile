include ../.compile_flag_file

default: gen_sym_stub compile convert link

gen_sym_stub:
	../tools/gen_sym_stub

compile:
	$(CC) $(CFLAGS) -c http_download.c -o http_download.o
	$(CC) $(CFLAGS) -c sym_stub.c -o sym_stub.o
	mv sym_stub.c sym_stub.c.debug

convert:
	../tools/converter sym_stub.o

link:
	$(LD) $(LINKFLAGS) http_download.o sym_stub.o -o http_download

probe:
	$(CC) $(CFLAGS) -c http_download.c -o http_download.o
	$(LD) $(LINKFLAGS) http_download.o -o http_download

install:
	cp http_download $(INS)

clean:
	rm -rf http_download.o http_download sym_stub.o sym_stub.c*
