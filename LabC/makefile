all: myshell mypipeline looper

myshell: myshell.o LineParser.o
	gcc -m32 -g -Wall -o myshell myshell.o LineParser.o

mypipeline: mypipeline.o
	gcc -m32 -g -Wall -o mypipeline mypipeline.o

myshell.o: myshell.c LineParser.h
	gcc -m32 -g -Wall -c -o myshell.o myshell.c

LineParser.o: LineParser.c LineParser.h
	gcc -m32 -g -Wall -c -o LineParser.o LineParser.c

mypipeline.o: mypipeline.c
	gcc -m32 -g -Wall -c -o mypipeline.o mypipeline.c

looper: looper.o 
	gcc -m32 -g -Wall -o looper looper.o

looper.o: looper.c 
	gcc -m32 -g -Wall -c -o looper.o looper.c

.PHONY: clean

clean:
	rm -f *.o myshell mypipeline looper
