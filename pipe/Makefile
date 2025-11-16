.PHONY: all clean

FLAGS = -Wall -Wextra -g

all: pipe write_fifo read_fifo

pipe: pipe.o
	gcc pipe.o -o pipe 

pipe.o: pipe.c
	gcc pipe.c -c ${FLAGS}

write_fifo: write_fifo.o
	gcc write_fifo.o -o write_fifo

write_fifo.o: write_fifo.c
	gcc write_fifo.c -c ${FLAGS}

read_fifo: read_fifo.o
	gcc read_fifo.o -o read_fifo

read_fifo.o: read_fifo.c
	gcc read_fifo.c -c ${FLAGS}

clean:
	rm *.o pipe write_fifo read_fifo tempfifo
