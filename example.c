#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>
#include "lfstack.h"

struct timeval  tv1, tv2;
lfstack_t results;

void *worker(void *);
void *worker(void *arg)
{
    long long i = 0;
    long long *int_data;
    while (i < 50000) {
        int_data = (long long*) malloc(sizeof(long long));
        assert(int_data != NULL);
        *int_data = i++;
        /*Push*/
        while (lfstack_push(&results, int_data) != 1) ;

        /*Pop*/
        while ( (int_data = lfstack_pop(&results)) == NULL);

        free(int_data);

    }

    return NULL;
}

#define join_threads \
for (i = 0; i < nthreads; i++)\
pthread_join(threads[i], NULL)


#define detach_thread_and_loop \
for (i = 0; i < nthreads; i++)\
pthread_detach(threads[i]);\
while (1) {\
sleep(2);\
printf("current size= %zu\n", lfstack_size(&results) );\
}

int main(void)
{
    int nthreads = sysconf(_SC_NPROCESSORS_ONLN); // Linux
    int i;

    lfstack_init(&results);

    /* Spawn threads. */
    pthread_t threads[nthreads];
    printf("Using %d thread%s.\n", nthreads, nthreads == 1 ? "" : "s");
    gettimeofday(&tv1, NULL);
    for (i = 0; i < nthreads; i++)
        pthread_create(threads + i, NULL, worker, NULL);

    join_threads;
    // detach_thread_and_loop;


    gettimeofday(&tv2, NULL);
    printf ("Total time = %f seconds\n",
            (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
            (double) (tv2.tv_sec - tv1.tv_sec));

    lfstack_clear(&results);
    return 0;
}

