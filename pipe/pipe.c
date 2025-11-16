#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define BUFSIZE  1024
#define FD_WRITE 1
#define FD_READ  0
#define SLEEPDUR 5

int main()
{
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t pid;

    switch (pid = fork())
    {
        case 0:
            printf("[child]: child process, pid = %d\n", getpid());
            close(pipe_fds[FD_WRITE]);
            printf("[child]: child sleeps for %d sec...\n", SLEEPDUR);

            sleep(SLEEPDUR);

            printf("[child]: child has just woke up!\n");
            char rdbuf[BUFSIZE+1];
            read(pipe_fds[FD_READ], rdbuf, BUFSIZE);
            printf("[child]: gotten string is: %s\n", rdbuf);
            break;

        case -1:
            perror("error!");
            exit(EXIT_FAILURE);
            break;

        default:
            printf("[parent]: parent process, pid = %d\n", getpid());
            close(pipe_fds[FD_READ]);

            char wrbuf[BUFSIZE+1]; 
            memset(wrbuf, 0, BUFSIZE+1);
            sprintf(wrbuf, "parent pid is %d, ", getpid());
            strcat(wrbuf, "cur time is: ");
            time_t cur_time = time(NULL);
            strcat(wrbuf, ctime(&cur_time));
            write(pipe_fds[FD_WRITE], wrbuf, BUFSIZE); 

            printf("[parent]: string has been written! Waiting...\n");
            int w_stat;
            wait(&w_stat);
            printf("[parent]: done\n");
            close(pipe_fds[FD_WRITE]);

            break;
    }
}
