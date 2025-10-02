#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>


pid_t pid;

void print_pid_at_exit()
{
    printf("[myfork]: %d exits\n", getpid());
}

void catch_sigint(int _)
{
    printf("[myfork]: process %d interrupted with SIGINT signal! Abort\n", getpid()); 
    if (pid)
    {
        kill(pid, SIGINT);
    }
    exit(EXIT_FAILURE);
}

void catch_sigterm(int _)
{
    printf("[myfork]: process %d interrupted with SIGTERM signal! Abort\n", getpid()); 
    if (pid)
    {
        kill(pid, SIGTERM);
    }
    exit(EXIT_FAILURE);
}


int main(int argc, char **argv)
{
    const int sleep_dur = 10;

    atexit(print_pid_at_exit);

    struct sigaction sa;
    sa.sa_handler = catch_sigterm;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        perror("[myfork]: sigaction");
        exit(EXIT_FAILURE);
    }

    if ((pid = fork()) == -1)
    {
        perror("[myfork]: error");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, catch_sigint);

    if (pid == 0)  // child
    {
        printf("[myfork]: process id - %d\n", getpid());
        printf("[myfork]: parent process id - %d\n", getppid());
        printf("[myfork]: child will be sleeping for %ds\n", sleep_dur);
        sleep(sleep_dur);
    }
    else   // parent
    {
        int wstatus; 

        printf("[myfork]: process id - %d\n", getpid());
        printf("[myfork]: child process id - %d\n", pid);

        wait(&wstatus);
        printf(
            "[myfork]: child is exited with code %d\n", WEXITSTATUS(wstatus));
    }
    return 0;
}



