.PHONY: all clean

FLAGS = -Wall -Wextra -g

all: archiver

archiver: archiver.o
	gcc archiver.o -o archiver

archiver.o: archiver.c
	gcc archiver.c -c ${FLAGS}

clean:
	rm *.o archiver
