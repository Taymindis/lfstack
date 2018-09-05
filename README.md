# lfstack
lock-free FIFO queue by C native built it, easy built cross platform(no extra dependencies needed) , guarantee thread safety memory management ever!


## API 
```c

extern int   lfstack_init(lfstack_t *lfstack);
extern int   lfstack_push(lfstack_t *lfstack, void *value);
extern void* lfstack_pop(lfstack_t *lfstack);
extern void* lfstack_single_pop(lfstack_t *lfstack);
extern void lfstack_destroy(lfstack_t *lfstack);
extern size_t lfstack_size(lfstack_t *lfstack);
extern void lfstack_usleep(unsigned int usec);

```


## Example

```c

int* int_data;
lfstack_t mystack;

if (lfstack_init(&mystack) == -1)
	return -1;

/** Wrap This scope in other threads **/
int_data = (int*) malloc(sizeof(int));
assert(int_data != NULL);
*int_data = i++;
/*PUSH*/
 while (lfstack_push(&mystack, int_data) == -1) {
    printf("ENQ Full ?\n");
}

/** Wrap This scope in other threads **/
/*POP*/
while  ( (int_data = lfstack_pop(&mystack)) == NULL) {
    printf("POP EMPTY ..\n");
}

// printf("%d\n", *(int*) int_data );
free(int_data);
/** End **/

lfstack_destroy(&mystack);

```


#### If you are using single thread POP. Please use `lfstack_single_pop` to get better result


## Build and Installation

For linux OS, you may use cmake build, for other platforms, please kindly include the source code and header file into the project, e.g. VS2017, DEV-C++, Xcode

```bash
mkdir build

cd build

cmake ..

make

./lfstack-example

valgrind --tool=memcheck --leak-check=full ./lfstack-example

sudo make install


```

## continuously Test 

For continuously test until N number, if you having any issue while testing, please kindly raise an issue

```bash

./keep-testing.sh

```


## Uninstallation

```bash
cd build

sudo make uninstall

```


## You may also like lock free queue FIFO

[lfqueue](https://github.com/Taymindis/lfqueue)