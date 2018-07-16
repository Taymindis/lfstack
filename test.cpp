#include <algorithm>
#include <string>
#include <gtest/gtest.h>
#include <fstream>
#include <chrono>
#include <vector>
#include <atomic>
#include <thread>
#include "lfstack.h"


lfstack_t results;

void *worker(void *arg)
{
    int i = 10000;
    long long *int_data;
    while (i--) {
        int_data = (long long*) malloc(sizeof(long long));
        assert(int_data != NULL);
        *int_data = i;
        while (lfstack_push(&results, int_data) != 1) ;

        while ( (int_data =(long long*) lfstack_pop(&results)) == NULL);

        free(int_data);

    }

    return NULL;
}

TEST(lfstackTest, CROSS_THREAD_MAIN_CALL) {
    int nthreads = sysconf(_SC_NPROCESSORS_ONLN); // Linux
    int i;

    lfstack_init(&results);

    /* Spawn threads. */
    pthread_t threads[nthreads];
    printf("Using %d thread%s.\n", nthreads, nthreads == 1 ? "" : "s");
    for (i = 0; i < nthreads; i++)
        pthread_create(threads + i, NULL, worker, NULL);


    for (i = 0; i < nthreads; i++)
        pthread_join(threads[i], NULL);

    lfstack_clear(&results);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
