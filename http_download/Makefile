include ../.compile_flag_file

objects := http_download.o
target := http_download

default: gen_sym_stub stub link

gen_sym_stub:
	../tools/gen_sym_stub

stub:
	$(CC) $(CFLAGS) -c $(SYM_STUB).c -o $(SYM_STUB).o
	mv $(SYM_STUB).c $(SYM_STUB).c.debug
	../tools/converter $(SYM_STUB).o

link: $(objects)
	$(LD) $(LINKFLAGS) $(objects) $(SYM_STUB).o -o $(target)

probe: $(objects)
	$(LD) $(LINKFLAGS) $(objects) -o $(target)

$(objects): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: install clean

install:
	cp $(target) $(INS)

clean:
	rm -rf $(objects) $(target) $(SYM_STUB).*
