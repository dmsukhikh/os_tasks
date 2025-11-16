#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#define BUFSIZE  1024
#define SLEEPDUR 15

int main()
{
    mkfifo("./tempfifo", 0666);
    int fd = open("./tempfifo", O_WRONLY);
    printf("[writer]: write string\n");

    char wrbuf[BUFSIZE];
    memset(wrbuf, 0, BUFSIZE);
    sprintf(wrbuf, "parent pid is %d, ", getpid());
    strcat(wrbuf, "cur time is: ");
    time_t cur_time = time(NULL);
    strcat(wrbuf, ctime(&cur_time));

    write(fd, wrbuf, BUFSIZE);

    printf("[writer]: string has been written!\n");
    close(fd);
}
