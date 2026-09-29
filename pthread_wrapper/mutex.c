#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "common.h"

#define BIONIC_PTHREAD_MUTEX_INITIALIZER(x) (bionic_mutex_t){ .bionic = {{ ((x & 3) << 14) }} }

void default_pthread_mutex_init(bionic_mutex_t *mutex)
{
	assert(mutex);

	enum {
		NORMAL,
		RECURSIVE,
		ERRORCHECK,
	} matched_static_initializer;

	if (!memcmp(&mutex->bionic, &BIONIC_PTHREAD_MUTEX_INITIALIZER(PTHREAD_MUTEX_NORMAL), sizeof(mutex->bionic))) {
		/* posix standard static initializer, equivalent to pthread_mutex_init with NULL attr */
		matched_static_initializer = NORMAL;
	} else if (!memcmp(&mutex->bionic, &BIONIC_PTHREAD_MUTEX_INITIALIZER(PTHREAD_MUTEX_RECURSIVE), sizeof(mutex->bionic))) {
		/* non-standard static initializer (present in glibc but not musl) */
		matched_static_initializer = RECURSIVE;
	} else if (!memcmp(&mutex->bionic, &BIONIC_PTHREAD_MUTEX_INITIALIZER(PTHREAD_MUTEX_ERRORCHECK), sizeof(mutex->bionic))) {
		/* non-standard static initializer (present in glibc but not musl) */
		matched_static_initializer = ERRORCHECK;
	} else {
		fprintf(stderr, "%s: lazy init failed, content of the struct doesn't match a known static initializer", __func__);
		exit(1);
	}

	mutex->glibc = mmap(NULL, sizeof(*mutex->glibc), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	switch (matched_static_initializer) {
		case NORMAL: {
			/* the standard static initializer is suppported by glibc and musl, can use that */
			*mutex->glibc = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
			break;
		}
		case RECURSIVE: {
#ifdef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
			*mutex->glibc = (pthread_mutex_t)PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#else /* musl */
			/* need to do dynamic init here */
			pthread_mutexattr_t attr;
			pthread_mutexattr_init(&attr);
			pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(mutex->glibc, &attr);
#endif
			break;
		}
		case ERRORCHECK: {
#ifdef PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
			*mutex->glibc = (pthread_mutex_t)PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
#else /* musl */
			/* need to do dynamic init here */
			pthread_mutexattr_t attr;
			pthread_mutexattr_init(&attr);
			pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
			pthread_mutex_init(mutex->glibc, &attr);
#endif
			break;

		}
	}

	// We might have been mapped by another thread in the meantime, otherwise fail
	assert(IS_MAPPED(mutex) && "no such default initializer???");
}

int bionic_pthread_mutex_destroy(bionic_mutex_t *mutex)
{
	assert(mutex);
	int ret = 0;
	if (IS_MAPPED(mutex)) {
		ret = pthread_mutex_destroy(mutex->glibc);
		munmap(mutex->glibc, sizeof(*mutex->glibc));
	}
	return ret;
}

int bionic_pthread_mutex_init(bionic_mutex_t *mutex, const bionic_mutexattr_t *attr)
{
	assert(mutex && (!attr || IS_MAPPED(attr)));
	// From PTHREAD_MUTEX_INIT(3)
	// Attempting to initialize an already initialized mutex result in undefined behavior.
	*mutex = (bionic_mutex_t){0};
	mutex->glibc = mmap(NULL, sizeof(*mutex->glibc), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	return pthread_mutex_init(mutex->glibc, (attr ? attr->glibc : NULL));
}

int bionic_pthread_mutex_lock(bionic_mutex_t *mutex)
{
	assert(mutex);
	INIT_IF_NOT_MAPPED(mutex, default_pthread_mutex_init);
	return pthread_mutex_lock(mutex->glibc);
}

int bionic_pthread_mutex_trylock(bionic_mutex_t *mutex)
{
	assert(mutex);
	INIT_IF_NOT_MAPPED(mutex, default_pthread_mutex_init);
	return pthread_mutex_trylock(mutex->glibc);
}

int bionic_pthread_mutex_unlock(bionic_mutex_t *mutex)
{
	assert(mutex);
	INIT_IF_NOT_MAPPED(mutex, default_pthread_mutex_init);
	return pthread_mutex_unlock(mutex->glibc);
}

int bionic_pthread_mutex_clocklock(bionic_mutex_t* mutex, clockid_t clock, const struct timespec* abs_timeout)
{
	assert(mutex);
	INIT_IF_NOT_MAPPED(mutex, default_pthread_mutex_init);
#ifdef __GLIBC__
	return pthread_mutex_clocklock(mutex->glibc, clock, abs_timeout);
#else
	struct timespec converted_abs_timeout;
	realtime_time_from_monotonic_time(&converted_abs_timeout, abs_timeout);
	return pthread_mutex_timedlock(mutex->glibc, &converted_abs_timeout);
#endif
}

int bionic_pthread_mutex_timedlock(bionic_mutex_t* mutex, const struct timespec* abs_timeout)
{
	assert(mutex);
	INIT_IF_NOT_MAPPED(mutex, default_pthread_mutex_init);
	return pthread_mutex_timedlock(mutex->glibc, abs_timeout);
}

int bionic_pthread_mutex_timedlock_monotonic_np(bionic_mutex_t* mutex, const struct timespec* abs_timeout)
{
	return bionic_pthread_mutex_clocklock(mutex, CLOCK_MONOTONIC, abs_timeout);
}
