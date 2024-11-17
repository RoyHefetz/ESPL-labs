all: Encode

Encode: encoder.o
	gcc -g -m32 -Wall -o Encode encoder.o

encoder.o: encoder.c
	gcc -g -Wall -m32 -c -o encoder.o encoder.c

.PHONY: clean

clean:
	rm -f *.o encoder