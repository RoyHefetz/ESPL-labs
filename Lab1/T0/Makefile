all: Count

Count: count-words.o
	gcc -g -Wall -m32 -o Count count-words.o

count-words.o: count-words.c
	gcc -g -Wall -m32 -c -o count-words.o count-words.c

.PHONY: clean

clean:
	rm -f *.o count-words