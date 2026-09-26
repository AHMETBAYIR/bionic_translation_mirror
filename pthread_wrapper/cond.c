#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "common.h"

#define BIONIC_PTHREAD_COND_INITIALIZER (bionic_cond_t){ .bionic = {{0}} }
#define BIONIC_PTHREAD_COND_INITIALIZER_MONOTONIC_NP (bionic_cond_t){ .bionic = {{1<<1}} }

static void default_pthread_cond_init(bionic_cond_t *cond)
{
	assert(cond);

	enum {
		DEFAULT,
		MONOTONIC,
	} matched_static_initializer;

	if (!memcmp(&cond->bionic, &BIONIC_PTHREAD_COND_INITIALIZER, sizeof(cond->bionic))) {
		/* posix standard static initializer, equivalent to pthread_cond_init with NULL attr */
		matched_static_initializer = DEFAULT;
	} else if (!memcmp(&cond->bionic, &BIONIC_PTHREAD_COND_INITIALIZER_MONOTONIC_NP, sizeof(cond->bionic))) {
		/* non-standard static initializer (not present in glibc or musl) */
		matched_static_initializer = MONOTONIC;
	} else {
		fprintf(stderr, "%s: lazy init failed, content of the struct doesn't match a known static initializer", __func__);
		exit(1);
	}

	cond->glibc = mmap(NULL, sizeof(*cond->glibc), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	switch (matched_static_initializer) {
		case DEFAULT: {
			/* the standard static initializer is suppported by glibc and musl, can use that */
			*cond->glibc = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
			break;
		}
		case MONOTONIC: {
			/* need to do dynamic init here */
			pthread_condattr_t attr;
			pthread_condattr_init(&attr);
			pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
			pthread_cond_init(cond->glibc, &attr);
			break;
		}
	}
}

int bionic_pthread_cond_destroy(bionic_cond_t *cond)
{
	assert(cond);
	int ret = 0;
	if (IS_MAPPED(cond)) {
		ret = pthread_cond_destroy(cond->glibc);
		munmap(cond->glibc, sizeof(*cond->glibc));
	}
	return ret;
}

int bionic_pthread_cond_init(bionic_cond_t *cond, const bionic_condattr_t *attr)
{
	// SUS // assert(cond && (!attr || IS_MAPPED(attr)));
	// From PTHREAD_COND_INIT(3)
	// Attempting to initialize an already initialized mutex result in undefined behavior.
	*cond = (bionic_cond_t){0};
	cond->glibc = mmap(NULL, sizeof(*cond->glibc), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	return pthread_cond_init(cond->glibc, (attr ? attr->glibc : NULL));
}

int bionic_pthread_cond_broadcast(bionic_cond_t *cond)
{
	assert(cond);
	INIT_IF_NOT_MAPPED(cond, default_pthread_cond_init);
	return pthread_cond_broadcast(cond->glibc);
}

int bionic_pthread_cond_signal(bionic_cond_t *cond)
{
	assert(cond);
	INIT_IF_NOT_MAPPED(cond, default_pthread_cond_init);
	return pthread_cond_signal(cond->glibc);
}

int bionic_pthread_cond_wait(bionic_cond_t *cond, bionic_mutex_t *mutex) {
	assert(cond && mutex);
	INIT_IF_NOT_MAPPED(cond, default_pthread_cond_init);
	INIT_IF_NOT_MAPPED(mutex, default_pthread_mutex_init);
	return pthread_cond_wait(cond->glibc, mutex->glibc);
}

int bionic_pthread_cond_clockwait(bionic_cond_t *cond, bionic_mutex_t *mutex, clockid_t clock_id, const struct timespec *abs_timeout) {
	assert(cond && mutex);
	INIT_IF_NOT_MAPPED(cond, default_pthread_cond_init);
	INIT_IF_NOT_MAPPED(mutex, default_pthread_mutex_init);
#ifdef __GLIBC__
	return pthread_cond_clockwait(cond->glibc, mutex->glibc, clock_id, abs_timeout);
#else /* musl */
	/*
	 * FIXME: This is a bit ugly but musl doesn't implement _clockwait yet.
	 * Instead we just patch the cond manually to update the clock type,
	 * which is a TERRIBLE idea but what else can we do...
	 */
	clockid_t old_clock = cond->glibc->__u.__i[4]; /* _c_clock */
	cond->glibc->__u.__i[4] = clock_id;
	int ret = pthread_cond_timedwait(cond->glibc, mutex->glibc, abs_timeout);
	cond->glibc->__u.__i[4] = old_clock;
	return ret;
#endif
}

int bionic_pthread_cond_timedwait(bionic_cond_t *cond, bionic_mutex_t *mutex, const struct timespec *abs_timeout)
{
	assert(cond && mutex);
	INIT_IF_NOT_MAPPED(cond, default_pthread_cond_init);
	INIT_IF_NOT_MAPPED(mutex, default_pthread_mutex_init);
	return pthread_cond_timedwait(cond->glibc, mutex->glibc, abs_timeout);
}

int bionic_pthread_cond_timedwait_monotonic_np(bionic_cond_t *cond, bionic_mutex_t *mutex, const struct timespec *abstime)
{
	return bionic_pthread_cond_clockwait(cond, mutex, CLOCK_MONOTONIC, abstime);
}

/* seems this name was accidentally shipped instead of _np, it was confined to 32bit ABIs when 64bit platform support was added */
#if !defined(__LP64__)
int bionic_pthread_cond_timedwait_monotonic(bionic_cond_t *cond, bionic_mutex_t *mutex, const struct timespec *abstime)
{
	return bionic_pthread_cond_timedwait_monotonic_np(cond, mutex, abstime);
}
#endif

int bionic_pthread_cond_timedwait_relative_np(bionic_cond_t *cond, bionic_mutex_t *mutex, const struct timespec *reltime)
{
	assert(cond && mutex && reltime);
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	tv.tv_sec += reltime->tv_sec;
	tv.tv_nsec += reltime->tv_nsec;
	if (tv.tv_nsec >= 1000000000) {
		++tv.tv_sec;
		tv.tv_nsec -= 1000000000;
	}
	return bionic_pthread_cond_timedwait_monotonic_np(cond, mutex, &tv);
}
