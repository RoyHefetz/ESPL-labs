all: hexeditplus counter

hexeditplus: hexeditplus.o
	gcc -g -Wall -m32 -o hexeditplus hexeditplus.o

hexeditplus.o: hexeditplus.c
	gcc -g -Wall -m32 -c -o hexeditplus.o hexeditplus.c

counter: counter.o
	gcc -m32 -fno-pie -fno-stack-protector -o counter counter.o

counter.o: counter.c
	gcc -m32 -fno-pie -fno-stack-protector -c -o counter.o counter.c

.PHONY: clean

clean:
	rm -f *.o hexeditplus counter
