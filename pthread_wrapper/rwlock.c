#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "common.h"

#define BIONIC_PTHREAD_RWLOCK_INITIALIZER (bionic_rwlock_t){ .bionic = {{0}} }

int bionic_pthread_rwlock_destroy(bionic_rwlock_t *rwlock)
{
	assert(rwlock);
	int ret = 0;
	if (IS_MAPPED(rwlock)) {
		ret = pthread_rwlock_destroy(rwlock->glibc);
		munmap(rwlock->glibc, sizeof(*rwlock->glibc));
	}
	return ret;

}

static void default_rwlock_init(bionic_rwlock_t *rwlock)
{
	if (memcmp(&rwlock->bionic, &BIONIC_PTHREAD_RWLOCK_INITIALIZER, sizeof(rwlock->bionic))) {
		fprintf(stderr, "%s: lazy init failed, content of the struct doesn't match a known static initializer", __func__);
		exit(1);
	}

	assert(rwlock);
	rwlock->glibc = mmap(NULL, sizeof(*rwlock->glibc), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	*rwlock->glibc = (pthread_rwlock_t)PTHREAD_RWLOCK_INITIALIZER;
}

int bionic_pthread_rwlock_init(bionic_rwlock_t *restrict rwlock, const bionic_rwlockattr_t *restrict attr)
{
	assert(rwlock);
	rwlock->glibc = mmap(NULL, sizeof(*rwlock->glibc), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	return pthread_rwlock_init(rwlock->glibc, (pthread_rwlockattr_t *)attr);
}

/* rdlock */

int bionic_pthread_rwlock_rdlock(bionic_rwlock_t *rwlock)
{
	assert(rwlock);
	INIT_IF_NOT_MAPPED(rwlock, default_rwlock_init);
	return pthread_rwlock_rdlock(rwlock->glibc);
}

int bionic_pthread_rwlock_tryrdlock(bionic_rwlock_t *rwlock)
{
	assert(rwlock);
	INIT_IF_NOT_MAPPED(rwlock, default_rwlock_init);
	return pthread_rwlock_tryrdlock(rwlock->glibc);
}

int bionic_pthread_rwlock_clockrdlock(bionic_rwlock_t *restrict rwlock, clockid_t clock, const struct timespec *restrict abs_timeout)
{
	assert(rwlock);
	INIT_IF_NOT_MAPPED(rwlock, default_rwlock_init);
	if (unlikely(clock != CLOCK_MONOTONIC && clock != CLOCK_REALTIME))
		return EINVAL;
	int ret = check_timespec(abs_timeout);
	if (unlikely(ret))
		return ret;

#ifdef __GLIBC__
	return pthread_rwlock_clockrdlock(rwlock->glibc, clock, abs_timeout);
#else
	struct timespec converted_abs_timeout;
	realtime_time_from_monotonic_time(&converted_abs_timeout, abs_timeout);
	return pthread_rwlock_timedrdlock(rwlock->glibc, &converted_abs_timeout);
#endif
}


int bionic_pthread_rwlock_timedrdlock(bionic_rwlock_t *restrict rwlock, const struct timespec *restrict abs_timeout)
{
	assert(rwlock);
	INIT_IF_NOT_MAPPED(rwlock, default_rwlock_init);
	int ret = check_timespec(abs_timeout);
	if (unlikely(ret))
		return ret;

	return pthread_rwlock_timedrdlock(rwlock->glibc, abs_timeout);
}

int bionic_pthread_rwlock_timedrdlock_monotonic_np(bionic_rwlock_t *restrict rwlock, const struct timespec *restrict abs_timeout)
{
	return bionic_pthread_rwlock_clockrdlock(rwlock, CLOCK_MONOTONIC, abs_timeout);
}

/* wrlock */

int bionic_pthread_rwlock_wrlock(bionic_rwlock_t *rwlock)
{
	assert(rwlock);
	INIT_IF_NOT_MAPPED(rwlock, default_rwlock_init);
	return pthread_rwlock_wrlock(rwlock->glibc);
}

int bionic_pthread_rwlock_trywrlock(bionic_rwlock_t *rwlock)
{
	assert(rwlock);
	INIT_IF_NOT_MAPPED(rwlock, default_rwlock_init);
	return pthread_rwlock_trywrlock(rwlock->glibc);
}

int bionic_pthread_rwlock_clockwrlock(bionic_rwlock_t *restrict rwlock, clockid_t clock, const struct timespec *restrict abs_timeout)
{
	assert(rwlock);
	INIT_IF_NOT_MAPPED(rwlock, default_rwlock_init);
	int ret = check_timespec(abs_timeout);
	if (unlikely(ret))
		return ret;

#ifdef __GLIBC__
	return pthread_rwlock_clockwrlock(rwlock->glibc, clock, abs_timeout);
#else
	struct timespec converted_abs_timeout;
	realtime_time_from_monotonic_time(&converted_abs_timeout, abs_timeout);
	return pthread_rwlock_timedwrlock(rwlock->glibc, &converted_abs_timeout);
#endif

}

int bionic_pthread_rwlock_timedwrlock(bionic_rwlock_t *restrict rwlock, const struct timespec *restrict abs_timeout)
{
	assert(rwlock);
	INIT_IF_NOT_MAPPED(rwlock, default_rwlock_init);
	int ret = check_timespec(abs_timeout);
	if (unlikely(ret))
		return ret;

	return pthread_rwlock_timedwrlock(rwlock->glibc, abs_timeout);
}

int bionic_pthread_rwlock_timedwrlock_monotonic_np(bionic_rwlock_t *restrict rwlock, const struct timespec *restrict abs_timeout)
{
	return bionic_pthread_rwlock_clockwrlock(rwlock, CLOCK_MONOTONIC, abs_timeout);
}

/* unlock */

int bionic_pthread_rwlock_unlock(bionic_rwlock_t *rwlock)
{
	assert(rwlock);
	INIT_IF_NOT_MAPPED(rwlock, default_rwlock_init);
	return pthread_rwlock_unlock(rwlock->glibc);
}
