#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "shmem.h"

void *shmaddr;

void free_at_exit(int sig)
{
    (void)sig;
    shmdt(shmaddr);
    exit(EXIT_SUCCESS);
}

int main()
{
    key_t shmkey = ftok(FNAME, FTOK_ID); 
    if (shmkey == -1)
    {
        if (errno == ENOENT)
        {
            fprintf(stderr, "[reader]: you should execute \"writer\" prog before!\n");
        }
        exit(EXIT_FAILURE);
    }

    int shmid = shmget(shmkey, BUFSIZE, 0);
    shmaddr = shmat(shmid, shmaddr, SHM_RDONLY);

    signal(SIGINT, free_at_exit);
    signal(SIGTERM, free_at_exit);
    
    for (;;)
    {
        time_t cur_time = time(NULL);
        printf("[reader]: cur pid = %i, time = %s", getpid(), ctime(&cur_time));

        char wrbuf[BUFSIZE];
        memcpy(wrbuf, shmaddr, BUFSIZE);
        printf("[reader]: got string: %s", wrbuf);
        sleep(1);
    }
}
