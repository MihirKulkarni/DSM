all: dsm.o test.o
	gcc -m32 -g -lpthread dsm.o test.o -o test

dsm.o: dsm.c
	gcc -m32 -g dsm.c -c -o dsm.o

test.o: test.c
	gcc -m32 -g test.c -c -o test.o
