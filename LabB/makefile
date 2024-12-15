all: AntiVirus

virusDetector: AntiVirus.o
	gcc -g -Wall -o AntiVirus AntiVirus.o

virusDetector.o: virusDetector.c
	gcc -g -Wall -c -o AntiVirus.o AntiVirus.c

.PHONY: clean

clean:
	rm -f *.o AntiVirus
