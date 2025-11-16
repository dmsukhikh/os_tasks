#include <stdio.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFSIZE  1024
#define SLEEPDUR 5

int main()
{
    mkfifo("./tempfifo", 0666);
    int fd = open("./tempfifo", O_RDONLY);
    printf("[reader]: open \"tempfifo\" for reading\n");
    printf("[reader]: sleep for %i seconds...\n", SLEEPDUR);
    sleep(SLEEPDUR);

    char buf[BUFSIZE];
    read(fd, buf, BUFSIZE);
    
    printf("[reader]: read from fifo, string: %s\n", buf);
    close(fd);
    remove("tempfifo");
}
