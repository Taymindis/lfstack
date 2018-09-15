#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>
#include <stdio.h>
#include "lfstack.h"


typedef void (*test_function)(pthread_t*);

void one_push_and_multi_pop(pthread_t *threads);
void one_pop_and_multi_push(pthread_t *threads);
void multi_push_pop(pthread_t *threads);
void*  worker_push_pop(void *);
void*  worker_push(void *);
void*  worker_pop(void *);
void*  worker_pushingle_c(void *);

/**Testing must**/
void one_push_and_multi_pop_must(pthread_t *threads);
void one_pop_must_and_multi_push(pthread_t *threads);
void multi_push_pop_must(pthread_t *threads);
void*  worker_push_pop_must(void *);
void*  worker_push_must(void *);
void*  worker_pop_must(void *);
void*  worker_single_pop_must(void *);

void running_test(test_function testfn);

struct timeval  tv1, tv2;
#define total_put 50000
#define total_running_loop 50
int nthreads = 4;
int one_thread = 1;
int nthreads_exited = 0;
lfstack_t *mystack;


void*  worker_pop_must(void *arg) {
	int i = 0;
	int *int_data;
	int total_loop = total_put * (*(int*)arg);
	while (i++ < total_loop) {
		/*Pop*/
		int_data = lfstack_pop_must(mystack);
		//	printf("%d\n", *int_data);

		free(int_data);
	}
	__sync_add_and_fetch(&nthreads_exited, 1);
	return 0;
}

void*  worker_single_pop_must(void *arg) {
	int i = 0;
	int *int_data;
	int total_loop = total_put * (*(int*)arg);
	while (i++ < total_loop) {
		/*Pop*/
		int_data = lfstack_single_pop_must(mystack);
		//	printf("%d\n", *int_data);

		free(int_data);
	}
	__sync_add_and_fetch(&nthreads_exited, 1);
	return 0;
}

void*  worker_push_pop_must(void *arg)
{
	int i = 0;
	int *int_data;
	while (i < total_put) {
		int_data = (int*)malloc(sizeof(int));
		assert(int_data != NULL);
		*int_data = i++;
		/*Push*/
		while (lfstack_push(mystack, int_data)) {
			printf("ENQ FULL?\n");
		}

		/*Pop*/
		int_data = lfstack_pop_must(mystack);
		// printf("%d\n", *int_data);
		free(int_data);
	}
	__sync_add_and_fetch(&nthreads_exited, 1);
	return 0;
}

void*  worker_pop(void *arg) {
	int i = 0;
	int *int_data;
	int total_loop = total_put * (*(int*)arg);
	while (i++ < total_loop) {
		/*Pop*/
		while ((int_data = lfstack_pop(mystack)) == NULL) {
			lfstack_sleep(1);
		}
		//	printf("%d\n", *int_data);

		free(int_data);
	}
	__sync_add_and_fetch(&nthreads_exited, 1);
	return 0;
}

void*  worker_pushingle_c(void *arg) {
	int i = 0;
	int *int_data;
	int total_loop = total_put * (*(int*)arg);
	while (i++ < total_loop) {
		/*Pop*/
		while ((int_data = lfstack_single_pop(mystack)) == NULL) {
			lfstack_sleep(1);
		}
		//	printf("%d\n", *int_data);

		free(int_data);
	}
	__sync_add_and_fetch(&nthreads_exited, 1);
	return 0;
}

/** Worker Keep Sending at the same time, do not try instensively **/
void*  worker_push(void *arg)
{
	int i = 0, *int_data;
	int total_loop = total_put * (*(int*)arg);
	while (i++ < total_loop) {
		int_data = (int*)malloc(sizeof(int));
		assert(int_data != NULL);
		*int_data = i;
		/*Push*/

		while (lfstack_push(mystack, int_data)) {
			// printf("ENQ FULL?\n");
		}
	}
	// __sync_add_and_fetch(&nthreads_exited, 1);
	return 0;
}

/** Worker Send And Consume at the same time **/
void*  worker_push_pop(void *arg)
{
	int i = 0;
	int *int_data;
	while (i < total_put) {
		int_data = (int*)malloc(sizeof(int));
		assert(int_data != NULL);
		*int_data = i++;
		/*Push*/
		while (lfstack_push(mystack, int_data)) {
			printf("ENQ FULL?\n");
		}

		/*Pop*/
		while ((int_data = lfstack_pop(mystack)) == NULL) {
			lfstack_sleep(1);
		}
		// printf("%d\n", *int_data);
		free(int_data);
	}
	__sync_add_and_fetch(&nthreads_exited, 1);
	return 0;
}

#define join_threads \
for (i = 0; i < nthreads; i++) {\
pthread_join(threads[i], NULL); \
}

#define detach_thread_and_loop \
for (i = 0; i < nthreads; i++)\
pthread_detach(threads[i]);\
while ( nthreads_exited < nthreads ) \
	lfstack_sleep(10);\
if(lfstack_size(mystack) != 0){\
lfstack_sleep(10);\
}

void multi_push_pop(pthread_t *threads) {
	printf("-----------%s---------------\n", "multi_push_pop");
	int i;
	for (i = 0; i < nthreads; i++) {
		pthread_create(threads + i, NULL, worker_push_pop, NULL);
	}

	join_threads;
	// detach_thread_and_loop;
}

void one_pop_and_multi_push(pthread_t *threads) {
	printf("-----------%s---------------\n", "one_pop_and_multi_push");
	int i;
	for (i = 0; i < nthreads; i++)
		pthread_create(threads + i, NULL, worker_push, &one_thread);

	worker_pushingle_c(&nthreads);

	join_threads;
	// detach_thread_and_loop;
}

void one_push_and_multi_pop(pthread_t *threads) {
	printf("-----------%s---------------\n", "one_push_and_multi_pop");
	int i;
	for (i = 0; i < nthreads; i++)
		pthread_create(threads + i, NULL, worker_pop, &one_thread);

	worker_push(&nthreads);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
	detach_thread_and_loop;
#pragma GCC diagnostic pop

}


void one_pop_must_and_multi_push(pthread_t *threads) {
	printf("-----------%s---------------\n", "one_pop_must_and_multi_push");
	int i;
	for (i = 0; i < nthreads; i++)
		pthread_create(threads + i, NULL, worker_push, &one_thread);

	worker_single_pop_must(&nthreads);

	join_threads;
	// detach_thread_and_loop;
}

void one_push_and_multi_pop_must(pthread_t *threads) {
	printf("-----------%s---------------\n", "one_push_and_multi_pop_must");
	int i;
	for (i = 0; i < nthreads; i++)
		pthread_create(threads + i, NULL, worker_pop_must, &one_thread);

	worker_push(&nthreads);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
	detach_thread_and_loop;
#pragma GCC diagnostic pop

}

void multi_push_pop_must(pthread_t *threads) {
	printf("-----------%s---------------\n", "multi_push_pop_must");
	int i;
	for (i = 0; i < nthreads; i++) {
		pthread_create(threads + i, NULL, worker_push_pop_must, NULL);
	}

	join_threads;
	// detach_thread_and_loop;
}

void running_test(test_function testfn) {
	int n;
	for (n = 0; n < total_running_loop; n++) {
		printf("Current running at =%d, ", n);
		nthreads_exited = 0;
		/* Spawn threads. */
		pthread_t threads[nthreads];
		printf("Using %d thread%s.\n", nthreads, nthreads == 1 ? "" : "s");
		printf("Total requests %d \n", total_put);
		gettimeofday(&tv1, NULL);

		testfn(threads);

		gettimeofday(&tv2, NULL);
		printf ("Total time = %f seconds\n",
		        (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
		        (double) (tv2.tv_sec - tv1.tv_sec));

		lfstack_sleep(10);
		assert ( 0 == lfstack_size(mystack) && "Error, all stack should be consumed but not");
	}
}

int main(void) {
	mystack = malloc(sizeof	(lfstack_t));
	if (lfstack_init(mystack) == -1)
		return -1;

	running_test(one_push_and_multi_pop);
	running_test(one_push_and_multi_pop_must);

	running_test(one_pop_and_multi_push);
	running_test(one_pop_must_and_multi_push);

	running_test(multi_push_pop);
	running_test(multi_push_pop_must);


	lfstack_destroy(mystack);
	// sleep(3);
	free(mystack);

	printf("Test Pass!\n");

	return 0;
}

