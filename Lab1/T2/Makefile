all: base

base: base.o
	gcc -g -Wall -m32 -o base base.o

base.o: base.c
	gcc -g -Wall -m32 -c -o base.o base.c

.PHONY: clean

clean:
	rm -f *.o base