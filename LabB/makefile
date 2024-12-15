all: AntiVirus

AntiVirus: AntiVirus.o
	gcc -g -Wall -o AntiVirus AntiVirus.o

AntiVirus.o: AntiVirus.c
	gcc -g -Wall -c -o AntiVirus.o AntiVirus.c

.PHONY: clean

clean:
	rm -f *.o AntiVirus
