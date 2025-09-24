.PHONY: all clean

FLAGS = -Wall -Wextra -O2 -g

all: mycat mygrep

mycat: mycat.o
	gcc mycat.o -o mycat

mycat.o: mycat.c
	gcc mycat.c -c ${FLAGS}

mygrep: mygrep.o
	gcc mygrep.o -o mygrep

mygrep.o: mygrep.c
	gcc mygrep.c -c ${FLAGS}

clean:
	rm *.o mycat mygrep
