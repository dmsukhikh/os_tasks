.PHONY: all clean

FLAGS = -Wall -Wextra -O2 -g

all: reader writer

reader: reader.o
	gcc reader.o -o reader

reader.o: reader.c
	gcc reader.c -c ${FLAGS}

writer: writer.o
	gcc writer.o -o writer

writer.o: writer.c
	gcc writer.c -c ${FLAGS}

clean:
	rm *.o reader writer
