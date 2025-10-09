.PHONY: all clean

FLAGS = -Wall -Wextra -g

all: mychmod

mychmod: mychmod.o
	gcc mychmod.o -o mychmod

mychmod.o: mychmod.c
	gcc mychmod.c -c ${FLAGS}

clean:
	rm *.o mychmod
