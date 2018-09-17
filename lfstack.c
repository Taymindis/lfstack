/*
*
* BSD 2-Clause License
*
* Copyright (c) 2018, Taymindis Woon
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#if defined __GNUC__ || defined __CYGWIN__ || defined __MINGW32__ || defined __APPLE__

#include <sys/time.h>
#include <unistd.h> // for usleep
#include <sched.h>

#define __LFS_VAL_COMPARE_AND_SWAP __sync_val_compare_and_swap
#define __LFS_BOOL_COMPARE_AND_SWAP __sync_bool_compare_and_swap
#define __LFS_FETCH_AND_ADD __sync_fetch_and_add
#define __LFS_ADD_AND_FETCH __sync_add_and_fetch
#define __LFS_YIELD_THREAD sched_yield
#define __LFS_SYNC_MEMORY __sync_synchronize

#else

#include <Windows.h>
#include <time.h>
#ifdef _WIN64
inline BOOL __SYNC_BOOL_CAS(LONG64 volatile *dest, LONG64 input, LONG64 comparand) {
	return InterlockedCompareExchangeNoFence64(dest, input, comparand) == comparand;
}
#define __LFS_VAL_COMPARE_AND_SWAP(dest, comparand, input) \
    InterlockedCompareExchangeNoFence64((LONG64 volatile *)dest, (LONG64)input, (LONG64)comparand)
#define __LFS_BOOL_COMPARE_AND_SWAP(dest, comparand, input) \
    __SYNC_BOOL_CAS((LONG64 volatile *)dest, (LONG64)input, (LONG64)comparand)
#define __LFS_FETCH_AND_ADD InterlockedExchangeAddNoFence64
#define __LFS_ADD_AND_FETCH InterlockedAddNoFence64
#define __LFS_SYNC_MEMORY MemoryBarrier

#else
#ifndef asm
#define asm __asm
#endif
inline BOOL __SYNC_BOOL_CAS(LONG volatile *dest, LONG input, LONG comparand) {
	return InterlockedCompareExchangeNoFence(dest, input, comparand) == comparand;
}
#define __LFS_VAL_COMPARE_AND_SWAP(dest, comparand, input) \
    InterlockedCompareExchangeNoFence((LONG volatile *)dest, (LONG)input, (LONG)comparand)
#define __LFS_BOOL_COMPARE_AND_SWAP(dest, comparand, input) \
    __SYNC_BOOL_CAS((LONG volatile *)dest, (LONG)input, (LONG)comparand)
#define __LFS_FETCH_AND_ADD InterlockedExchangeAddNoFence
#define __LFS_ADD_AND_FETCH InterlockedAddNoFence
#define __LFS_SYNC_MEMORY() asm mfence

#endif
#include <windows.h>
#define __LFS_YIELD_THREAD SwitchToThread
#endif

#include "lfstack.h"
#define DEF_LFS_ASSIGNED_SPIN 2048

#if defined __GNUC__ || defined __CYGWIN__ || defined __MINGW32__ || defined __APPLE__
#define lfs_time_t long
#define lfs_get_curr_time(_time_sec) \
struct timeval _time_; \
gettimeofday(&_time_, NULL); \
*_time_sec = _time_.tv_sec
#define lfs_diff_time(_etime_, _stime_) _etime_ - _stime_
#else
#define lfs_time_t time_t
#define lfs_get_curr_time(_time_sec) time(_time_sec)
#define lfs_diff_time(_etime_, _stime_) difftime(_etime_, _stime_)
#endif

struct lfstack_cas_node_s {
	void * value;
	struct lfstack_cas_node_s *prev, *nextfree;
	lfs_time_t _deactivate_tm;
};

static void __lfs_recycle_free(lfstack_t *, lfstack_cas_node_t*);
static void _lfs_check_free(lfstack_t *);
static void *_pop(lfstack_t *);
static void *_single_pop(lfstack_t *);
static int _push(lfstack_t *, void* );
static inline void* _lfstack_malloc(void* pl, size_t sz) {
	return malloc(sz);
}
static inline void _lfstack_free(void* pl, void* ptr) {
	free(ptr);
}

static void *
_pop(lfstack_t *lfs) {
	lfstack_cas_node_t *head, *prev;
	void *val;

	for (;;) {
		head = lfs->head;
		__LFS_SYNC_MEMORY();
		/** ABA PROBLEM? in order to solve this, I use time free to avoid realloc the same aligned address **/
		if (lfs->head == head) {
			prev = head->prev;
			if (prev) {
				if (__LFS_BOOL_COMPARE_AND_SWAP(&lfs->head, head, prev)) {
					val = head->value;
					break;
				}
			} else {
				/** Empty **/
				val = NULL;
				goto _done;
			}
		}
	}
	__lfs_recycle_free(lfs, head);
	__LFS_YIELD_THREAD();
_done:
// __asm volatile("" ::: "memory");
	__LFS_SYNC_MEMORY();
	_lfs_check_free(lfs);
	return val;
}

static void *
_single_pop(lfstack_t *lfs) {
	lfstack_cas_node_t *head, *prev;
	void *val;

	for (;;) {
		head = lfs->head;
		__LFS_SYNC_MEMORY();
		if (lfs->head == head) {
			prev = head->prev;
			if (prev) {
				if (__LFS_BOOL_COMPARE_AND_SWAP(&lfs->head, head, prev)) {
					val = head->value;
					lfs->_free(lfs->pl, head);
					break;
				}
			} else {
				/** Empty **/
				return NULL;
			}
		}
	}
	return val;
}

static int
_push(lfstack_t *lfs, void* value) {
	lfstack_cas_node_t *head, *new_head;
	new_head = (lfstack_cas_node_t*) lfs->_malloc(lfs->pl, sizeof(lfstack_cas_node_t));
	if (new_head == NULL) {
		perror("malloc");
		return errno;
	}
	new_head->value = value;
	new_head->nextfree = NULL;
	for (;;) {
		__LFS_SYNC_MEMORY();
		new_head->prev = head = lfs->head;
		if (__LFS_BOOL_COMPARE_AND_SWAP(&lfs->head, head, new_head)) {
			// always check any free value
			_lfs_check_free(lfs);
			return 0;
		}
	}

	/*It never be here*/
	return -1;
}

static void
__lfs_recycle_free(lfstack_t *lfs, lfstack_cas_node_t* freenode) {
	lfstack_cas_node_t *freed;
	do {
		freed = lfs->move_free;
	} while (!__LFS_BOOL_COMPARE_AND_SWAP(&freed->nextfree, NULL, freenode) );

	lfs_get_curr_time(&freenode->_deactivate_tm);

	__LFS_BOOL_COMPARE_AND_SWAP(&lfs->move_free, freed, freenode);
}

static void
_lfs_check_free(lfstack_t *lfs) {
	lfs_time_t curr_time;
	if (__LFS_BOOL_COMPARE_AND_SWAP(&lfs->in_free_mode, 0, 1)) {
		lfs_get_curr_time(&curr_time);
		lfstack_cas_node_t *rtfree = lfs->root_free, *nextfree;
		while ( rtfree && (rtfree != lfs->move_free) ) {
			nextfree = rtfree->nextfree;
			if ( lfs_diff_time(curr_time, rtfree->_deactivate_tm) > 2) {
				//	printf("%p\n", rtfree);
				lfs->_free(lfs->pl, rtfree);
				rtfree = nextfree;
			} else {
				break;
			}
		}
		lfs->root_free = rtfree;
		__LFS_BOOL_COMPARE_AND_SWAP(&lfs->in_free_mode, 1, 0);
	}
	__LFS_SYNC_MEMORY();
}

int
lfstack_init(lfstack_t *lfs) {
	return lfstack_init_mf(lfs, NULL, _lfstack_malloc, _lfstack_free);
}

int
lfstack_init_mf(lfstack_t *lfs, void* pl, lfstack_malloc_fn lfs_malloc, lfstack_free_fn lfs_free) {
	lfs->_malloc = lfs_malloc;
	lfs->_free = lfs_free;
	lfs->pl = pl;

	lfstack_cas_node_t *base = lfs->_malloc(lfs->pl, sizeof(lfstack_cas_node_t));
	lfstack_cas_node_t *freebase = lfs->_malloc(lfs->pl, sizeof(lfstack_cas_node_t));
	if (base == NULL || freebase == NULL) {
		perror("malloc");
		return errno;
	}
	base->value = NULL;
	base->prev = NULL;
	base->nextfree = NULL;
	base->_deactivate_tm = 0;

	freebase->value = NULL;
	freebase->prev = NULL;
	freebase->nextfree = NULL;
	freebase->_deactivate_tm = 0;

	lfs->head = base; // Not yet to be free for first node only
	lfs->root_free = lfs->move_free = freebase; // Not yet to be free for first node only
	lfs->size = 0;
	lfs->in_free_mode = 0;

	return 0;
}

void
lfstack_destroy(lfstack_t *lfs) {
	void* p;
	while ((p = lfstack_pop(lfs))) {
		lfs->_free(lfs->pl, p);
	}
	// Clear the recycle chain nodes
	lfstack_cas_node_t *rtfree = lfs->root_free, *nextfree;
	while (rtfree && (rtfree != lfs->move_free) ) {
		nextfree = rtfree->nextfree;
		lfs->_free(lfs->pl, rtfree);
		rtfree = nextfree;
	}
	if (rtfree) {
		lfs->_free(lfs->pl, rtfree);
	}

	lfs->_free(lfs->pl, lfs->head); // Last free

	lfs->size = 0;
}

void lfstack_flush(lfstack_t *lfstack) {
	_lfs_check_free(lfstack);
}

int
lfstack_push(lfstack_t *lfs, void *value) {
	if (_push(lfs, value)) {
		return -1;
	}
	__LFS_ADD_AND_FETCH(&lfs->size, 1);
	return 0;
}

void*
lfstack_pop(lfstack_t *lfs) {
	void *v;
	if (//__LFS_ADD_AND_FETCH(&lfs->size, 0) &&
	    (v = _pop(lfs))
	) {

		__LFS_FETCH_AND_ADD(&lfs->size, -1);
		return v;
	}
	// Rest the thread for other thread, to avoid keep looping force
	lfstack_sleep(1);
	return NULL;
}

void*
lfstack_pop_must(lfstack_t *lfs) {
	void *v;
	while ( !(v = _pop(lfs)) ) {
		// Rest the thread for other thread, to avoid keep looping force
		lfstack_sleep(1);
	}
	__LFS_FETCH_AND_ADD(&lfs->size, -1);
	return v;
}

/**This is only applicable when only single thread consume only**/
void*
lfstack_single_pop(lfstack_t *lfs) {
	void *v;
	if (//__LFS_ADD_AND_FETCH(&lfs->size, 0) &&
	    (v = _single_pop(lfs))
	) {

		__LFS_FETCH_AND_ADD(&lfs->size, -1);
		return v;
	}
	// Rest the thread for other thread, to avoid keep looping force
	lfstack_sleep(1);
	return NULL;
}

void*
lfstack_single_pop_must(lfstack_t *lfs) {
	void *v;
	while ( !(v = _single_pop(lfs)) ) {
		// Rest the thread for other thread, to avoid keep looping force
		lfstack_sleep(1);
	}
	__LFS_FETCH_AND_ADD(&lfs->size, -1);
	return v;
}

size_t
lfstack_size(lfstack_t *lfs) {
	return __LFS_ADD_AND_FETCH(&lfs->size, 0);
}

void
lfstack_sleep(unsigned int milisec) {
#if defined __GNUC__ || defined __CYGWIN__ || defined __MINGW32__ || defined __APPLE__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
	usleep(milisec * 1000);
#pragma GCC diagnostic pop
#else
	Sleep(milisec);
#endif
}

#ifdef __cplusplus
}
#endif
