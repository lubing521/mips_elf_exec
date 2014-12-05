default: regen_symbol_table utils parser converter

regen_symbol_table:
	grep " [TBSADGR] " ../symbol_table.txt > ../symbol_table.txt.temp
	mv ../symbol_table.txt.temp ../symbol_table.txt

utils:
	gcc -o utils.o -Wall -c utils.c

parser:
	gcc -o parser -Wall parser.c utils.o

converter:
	gcc -o converter -Wall converter.c utils.o

clean:
	rm -rf parser converter utils.o
