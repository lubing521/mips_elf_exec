default: regen_symbol_table converter

regen_symbol_table:
	grep " [TBSADGR] " ../symbol_table.txt > ../symbol_table.txt.temp
	mv ../symbol_table.txt.temp ../symbol_table.txt

converter:
	gcc -o converter -Wall converter.c

clean:
	rm -rf converter *.o *.exe
