objects := elf.o util.o
target := elf

$(objects): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
