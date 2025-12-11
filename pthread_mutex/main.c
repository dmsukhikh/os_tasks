#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define THREAD_COUNT 10
#define STRSIZE      1024

// shared data
char str[STRSIZE];
pthread_mutex_t str_mx = PTHREAD_MUTEX_INITIALIZER;

void *reader_func(void *args)
{
    for (;;)
    {
        pthread_mutex_lock(&str_mx);

        if (strcmp(str, "done") == 0)
	{
            pthread_mutex_unlock(&str_mx);
	    return NULL;
	}

        pthread_t tid = *(pthread_t *)args;
        printf("tid: %ld, string: \'%s\'\n", tid, str);

        pthread_mutex_unlock(&str_mx);
        sleep(1);
    }
    return NULL;
}

void *writer_func(void *args)
{
    for (;;)
    {
    	pthread_mutex_lock(&str_mx);

    	int *num = (int *)args;
    	(*num)++;
    	sprintf(str, "worker wrote num=%d", *num);

	if (*num == 5 + 1) 
	{
	    sprintf(str, "done");
    	    pthread_mutex_unlock(&str_mx);
	    return NULL;
	}

    	pthread_mutex_unlock(&str_mx);
    	sleep(1);

    }
    return NULL;
}

int main()
{
    int cnt = 0;

    pthread_attr_t thread_attrs;
    if (pthread_attr_init(&thread_attrs) != 0)
    {
        perror("pthread_attr_init");    
	exit(EXIT_FAILURE);
    }

    pthread_t thread_pool[THREAD_COUNT];
    pthread_t thread_writer;

    // writer
    pthread_create(&thread_writer, &thread_attrs, writer_func, &cnt);

    // readers
    for (int i = 0; i < THREAD_COUNT; ++i)
    {
	// ?
        int code = 
		pthread_create(&thread_pool[i], &thread_attrs, reader_func, &thread_pool[i]);
	if (code != 0)
	{
	    perror("pthread_create");
	    exit(EXIT_FAILURE);
	}
    }

    pthread_attr_destroy(&thread_attrs);

    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        int code = pthread_join(thread_pool[i], NULL); 
	if (code != 0)
	{
	    perror("thread_join");
	    exit(EXIT_FAILURE);
	}
    }
    pthread_join(thread_writer, NULL);
}
