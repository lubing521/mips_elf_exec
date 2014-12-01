default: utils parser converter

utils:
	gcc -o utils.o -Wall -c utils.c

parser:
	gcc -o parser -Wall parser.c utils.o

converter:
	gcc -o converter -Wall converter.c utils.o

clean:
	rm -rf parser converter utils.o
