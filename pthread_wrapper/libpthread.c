#include <assert.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

#include "common.h"

bool is_mapped(void *mem, const size_t sz)
{
	const size_t ps = sysconf(_SC_PAGESIZE);
	assert(ps > 0);
	unsigned char vec[(sz + ps - 1) / ps];
	return !mincore(mem, sz, vec);
}

#ifndef __GLIBC__ // musl
/* musl doesn't have any way to use mononotic time, not even the POSIX 2024 way.
 * To be able to use the realtime functions instead, we need to convert the abs_timeout
 * argument. */
/* based on monotonic_time_from_realtime_time from bionic */
#define NS_PER_S 1'000'000'000LL
void realtime_time_from_monotonic_time(struct timespec *realtime_time, const struct timespec *monotonic_time) {
	*realtime_time = *monotonic_time;


	struct timespec cur_realtime_time;
	clock_gettime(CLOCK_REALTIME, &cur_realtime_time);

	struct timespec cur_monotonic_time;
	clock_gettime(CLOCK_MONOTONIC, &cur_monotonic_time);

	realtime_time->tv_nsec -= cur_monotonic_time.tv_nsec;
	realtime_time->tv_nsec += cur_realtime_time.tv_nsec;
	if (realtime_time->tv_nsec >= NS_PER_S) {
		realtime_time->tv_nsec -= NS_PER_S;
		realtime_time->tv_sec += 1;
	} else if (realtime_time->tv_nsec < 0) {
		realtime_time->tv_nsec += NS_PER_S;
		realtime_time->tv_sec -= 1;
	}
	realtime_time->tv_sec -= cur_monotonic_time.tv_sec;
	realtime_time->tv_sec += cur_realtime_time.tv_sec;

}
#endif

/* misc */

int bionic_pthread_create(bionic_pthread_t *thread, const bionic_attr_t *attr, void* (*start)(void*), void *arg)
{
	assert(thread && (!attr || IS_MAPPED(attr)));
	return pthread_create((pthread_t*)thread, (attr ? attr->glibc : NULL), start, arg);
}

int bionic_pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
	return pthread_atfork(prepare, parent, child);
}

/* cleanup */

void bionic___pthread_cleanup_push(struct bionic_pthread_cleanup_t *c, void (*routine)(void*), void *arg)
{
	assert(c && routine);
#ifdef __GLIBC__
	c->glibc = mmap(NULL, sizeof(*c->glibc), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	c->routine = routine;
	c->arg = arg;

	int not_first_call;
	if ((not_first_call = sigsetjmp((struct __jmp_buf_tag*)(void*)c->glibc->__cancel_jmp_buf, 0))) {
		routine(arg);
		__pthread_unwind_next(c->glibc);
	}

	__pthread_register_cancel(c->glibc);
#else
	c->musl = mmap(NULL, sizeof(struct __ptcb), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	c->routine = routine;
	c->arg = arg;
	_pthread_cleanup_push(c->musl, routine, arg);
#endif
}

void bionic___pthread_cleanup_pop(struct bionic_pthread_cleanup_t *c, int execute)
{
#ifdef __GLIBC__
	assert(c && IS_MAPPED(c)); // TODO - analogically for musl?
	__pthread_unregister_cancel(c->glibc);

	if (execute)
		c->routine(c->arg);

	munmap(c->glibc, sizeof(*c->glibc));
#else
	_pthread_cleanup_pop(c->musl, execute);
	munmap(c->musl, sizeof(struct __ptcb));
#endif
}
