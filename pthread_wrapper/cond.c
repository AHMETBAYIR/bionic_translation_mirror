#include <assert.h>
#include <string.h>
#include <sys/mman.h>

#include "common.h"

static void default_pthread_cond_init(bionic_cond_t *cond)
{
	assert(cond);
	cond->glibc = mmap(NULL, sizeof(*cond->glibc), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	memset(cond->glibc, 0, sizeof(*cond->glibc));
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

int
bionic_pthread_cond_wait(bionic_cond_t *cond, bionic_mutex_t *mutex) {
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

int bionic_pthread_cond_timedwait_relative_np(bionic_cond_t *cond, bionic_mutex_t *mutex, const struct timespec *reltime)
{
	assert(cond && mutex && reltime);
	struct timespec tv;
	clock_gettime(CLOCK_REALTIME, &tv);
	tv.tv_sec += reltime->tv_sec;
	tv.tv_nsec += reltime->tv_nsec;
	if (tv.tv_nsec >= 1000000000) {
		++tv.tv_sec;
		tv.tv_nsec -= 1000000000;
	}
	return bionic_pthread_cond_timedwait(cond, mutex, &tv);
}

int bionic_pthread_cond_timedwait_monotonic_np(bionic_cond_t *cond, bionic_mutex_t *mutex, const struct timespec *abstime)
{
	assert(cond && mutex && abstime);
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	tv.tv_sec += abstime->tv_sec;
	tv.tv_nsec += abstime->tv_nsec;
	if (tv.tv_nsec >= 1000000000) {
		++tv.tv_sec;
		tv.tv_nsec -= 1000000000;
	}
	return bionic_pthread_cond_timedwait(cond, mutex, &tv);
}

int bionic_pthread_cond_timedwait_monotonic(bionic_cond_t *cond, bionic_mutex_t *mutex, const struct timespec *abstime)
{
	return bionic_pthread_cond_timedwait_monotonic_np(cond, mutex, abstime);
}
