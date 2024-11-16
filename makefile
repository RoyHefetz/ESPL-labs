all: encoder

encoder: encoder.c
	gcc -Wall -g -o encoder encoder.c

clean:
	rm -f encoder *.o