all: addresses

addresses: addresses.o
	gcc -g -Wall -m32 -o addresses addresses.o

addresses.o: addresses.c
	gcc -g -Wall -m32 -c -o addresses.o addresses.c

.PHONY: clean

clean:
	rm -f *.o addresses