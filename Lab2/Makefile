all: myshell mypipe looper

myshell: myshell.o LineParser.o
	gcc -m32 -g -Wall -o myshell myshell.o LineParser.o

mypipe: mypipe.o
	gcc -m32 -g -Wall -o mypipe mypipe.o

myshell.o: myshell.c LineParser.h
	gcc -m32 -g -Wall -c -o myshell.o myshell.c

LineParser.o: LineParser.c LineParser.h
	gcc -m32 -g -Wall -c -o LineParser.o LineParser.c

mypipe.o: mypipe.c
	gcc -m32 -g -Wall -c -o mypipe.o mypipe.c

looper: looper.o 
	gcc -m32 -g -Wall -o looper looper.o

looper.o: looper.c 
	gcc -m32 -g -Wall -c -o looper.o looper.c

.PHONY: clean

clean:
	rm -f *.o myshell mypipe looper