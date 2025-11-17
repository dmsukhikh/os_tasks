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

int shmid;
void *shmaddr;

void free_at_exit(int sig)
{
    (void)sig;
    shmdt(shmaddr);
    shmctl(shmid, IPC_RMID, NULL);
    unlink(FNAME);
    exit(EXIT_SUCCESS);
}

void write_string_in_shm()
{
    char wrbuf[BUFSIZE];
    memset(wrbuf, 0, BUFSIZE);
    sprintf(wrbuf, "hi from writer! writer pid is %d, ", getpid());
    strcat(wrbuf, "cur time is: ");
    time_t cur_time = time(NULL);
    strcat(wrbuf, ctime(&cur_time));
    memcpy(shmaddr, wrbuf, BUFSIZE);
    printf("[writer]: string has been written\n");
}

int main()
{
    // Создаем файл для записи shmid
    int fd = open(FNAME, O_CREAT | O_WRONLY);
    if (fd == -1)
    {
        if (errno == EEXIST)
        {
            fprintf(stderr, "[shmem]: only a single \"writer\" can be executed at the same time!\n");
        }
        else
        {
            perror("open");
        }
        exit(EXIT_FAILURE);
    }
    close(fd);
    
    key_t shmkey = ftok(FNAME, FTOK_ID);
    if (shmkey == -1)
    {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    // Создаем память для чтения и линкуем ее
    shmid = shmget(shmkey, BUFSIZE, IPC_CREAT | IPC_EXCL | 0660); 
    if (shmid == -1)
    {
        perror("schmget");
        exit(EXIT_FAILURE);
    }
    
    shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == (void *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Настраиваем обработку сигналов
    signal(SIGTERM, free_at_exit);
    signal(SIGINT, free_at_exit);
    memset(shmaddr, 0, BUFSIZE);

    // mainloop
    for (;;)
    {
        write_string_in_shm();
        sleep(3);
    }
}
