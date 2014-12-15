default: regen_symbol_table utils converter

regen_symbol_table:
	grep " [TBSADGR] " ../symbol_table.txt > ../symbol_table.txt.temp
	mv ../symbol_table.txt.temp ../symbol_table.txt

utils:
	gcc -o utils.o -Wall -c utils.c

converter:
	gcc -o converter -Wall converter.c utils.o

clean:
	rm -rf converter utils.o
