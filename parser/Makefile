all: libparser.so

libparser.so: libparser.o
	gcc -shared -o ../libparser.so LinkedListAPI.o GEDCOMparser.o GEDCOMutilities.o

libparser.o: src/LinkedListAPI.c src/GEDCOMparser.c src/GEDCOMutilities.c
	gcc -Wall -g -std=c11 -Iinclude -c -fpic src/LinkedListAPI.c
	gcc -Wall -g -std=c11 -Iinclude -c -fpic src/GEDCOMparser.c
	gcc -Wall -g -std=c11 -Iinclude -c -fpic src/GEDCOMutilities.c

clean:

