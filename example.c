#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>
#include <stdio.h>
#include "lfstack.h"


void one_push_and_multi_pop(pthread_t *threads);
void one_pop_and_multi_push(pthread_t *threads);
void multi_push_pop(pthread_t *threads);
void*  worker_push_pop(void *);
void*  worker_push(void *);
void*  worker_pop(void *);
void*  worker_single_pop(void *);

struct timeval  tv1, tv2;
#define total_put 50000
int nthreads = 8; //sysconf(_SC_NPROCESSORS_ONLN); // Linux
int one_thread = 1;
int nthreads_exited = 0;
lfstack_t *mystack;

void*  worker_pop(void *arg) {
	int i = 0;
	int *int_data;
	int total_loop = total_put * (*(int*)arg);
	while (i++ < total_loop) {
		while ((int_data = lfstack_pop(mystack)) == NULL) {

		}
		//	printf("%d\n", *int_data);

		free(int_data);
	}
	__sync_add_and_fetch(&nthreads_exited, 1);
	return 0;
}

void*  worker_single_pop(void *arg) {
	int i = 0;
	int *int_data;
	int total_loop = total_put * (*(int*)arg);
	while (i++ < total_loop) {
		while ((int_data = lfstack_single_pop(mystack)) == NULL) {

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

		while (lfstack_push(mystack, int_data)) {
			// printf("PUSH FULL?\n");
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
		while (lfstack_push(mystack, int_data)) {
			printf("PUSH FULL?\n");
		}

		while ((int_data = lfstack_pop(mystack)) == NULL) {
			// printf("POP EMPTY? %zu\n", lfstack_size(mystack));
		}
		// printf("%d\n", *int_data);
		free(int_data);
	}
	__sync_add_and_fetch(&nthreads_exited, 1);
	return 0;
}

#define join_threads \
for (i = 0; i < nthreads; i++)\
pthread_join(threads[i], NULL);\
printf("current size= %d\n", (int) lfstack_size(mystack) )

#define detach_thread_and_loop \
for (i = 0; i < nthreads; i++)\
pthread_detach(threads[i]);\
while ( nthreads_exited < nthreads ) \
	lfstack_usleep(2000);\
if(lfstack_size(mystack) != 0){\
lfstack_usleep(2000);\
printf("current size= %zu\n", lfstack_size(mystack) );\
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

	worker_single_pop(&nthreads);

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
int main(void)
{
	int n;

	mystack = malloc(sizeof	(lfstack_t));

	if (lfstack_init(mystack) == -1)
		return -1;

	for (n = 0; n < 100; n++) {
		printf("Current running at =%d, ", n);
		nthreads_exited = 0;

		/* Spawn threads. */
		pthread_t threads[nthreads];
		printf("Using %d thread%s.\n", nthreads, nthreads == 1 ? "" : "s");
		printf("Total requests %d \n", total_put);
		gettimeofday(&tv1, NULL);

		   one_push_and_multi_pop(threads);

		//   one_pop_and_multi_push(threads);
		// multi_push_pop(threads);

		gettimeofday(&tv2, NULL);
		printf ("Total time = %f seconds\n",
		        (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
		        (double) (tv2.tv_sec - tv1.tv_sec));

		//getchar();
		lfstack_usleep(1000);
		assert ( 0 == lfstack_size(mystack) && "Error, all element should be pop out but not");
	}

	printf("Take a 4 seconds sleep \n");
	sleep(4);
	printf("Flush all the inactive memory \n");
	lfstack_flush(mystack);
	printf("Check the memory usage, it should all flushed, press any key to exit \n");
	getchar();
	lfstack_destroy(mystack);
	free(mystack);


	return 0;
}

