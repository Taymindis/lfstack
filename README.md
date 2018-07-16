# lfstack
it's a smallest library that provides lock-free LIFO stack by C gcc native Atomic BuiltIn, no more than 100 line of code that can achieve lock free mechanism. `__atomic` ...

## lfstack

```c
int* ret;

lfstack_t results;

lfstack_init(&results);

/** Wrap This scope in multithread testing **/
int *int_data = malloc(sizeof(int));
assert(int_data != NULL);
*int_data = 5;
while (lfstack_push(&results, int_data) != 1) ;

while ( (ret = lfstack_pop(&results)) == NULL);

free(ret);
/** End **/

lfstack_clear(&results);

```


## Installation

```bash
mkdir build

cd build

cmake ..

make

./lfstack-example

valgrind --tool=memcheck --leak-check=full ./lfstack-example

sudo make install


```


## Uninstallation

```bash
cd build

sudo make uninstall

```


## You may also like lock free queue FIFO

[lfqueue](https://github.com/Taymindis/lfqueue)