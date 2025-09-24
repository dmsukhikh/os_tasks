.PHONY: all clean

FLAGS = -Wall -Wextra -O2

all: mycat

mycat: mycat.o
	gcc mycat.o -o mycat

mycat.o: mycat.c
	gcc mycat.c -c ${FLAGS}

clean:
	rm *.o mycat
