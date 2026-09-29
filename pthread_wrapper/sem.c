#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>

#include "common.h"

int bionic_sem_destroy(bionic_sem_t *sem)
{
	assert(sem);
	int ret = 0;
	if (IS_MAPPED(sem)) {
		ret = sem_destroy(sem->glibc);
		munmap(sem->glibc, sizeof(*sem->glibc));
	}
	return ret;
}

static void default_sem_init(bionic_sem_t *sem)
{
	// Apparently some android apps (hearthstone) do not call sem_init()
	assert(sem);
	sem->glibc = mmap(NULL, sizeof(*sem->glibc), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	/* on bionic, zero init is equivalent to this (technically also on glibc and musl, but we shouldn't rely on that) */
	sem_init(sem->glibc, 0, 0);
}

int bionic_sem_init(bionic_sem_t *sem, int pshared, unsigned int value)
{
	assert(sem);
	// From SEM_INIT(3)
	// Initializing a semaphore that has already been initialized results in underined behavior.
	*sem = (bionic_sem_t){0};
	sem->glibc = mmap(NULL, sizeof(*sem->glibc), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	return sem_init(sem->glibc, pshared, value);
}

int bionic_sem_getvalue(bionic_sem_t* sem, int* value)
{
	assert(sem);
	INIT_IF_NOT_MAPPED(sem, default_sem_init);
	return sem_getvalue(sem->glibc, value);
}

int bionic_sem_post(bionic_sem_t *sem)
{
	assert(sem);
	INIT_IF_NOT_MAPPED(sem, default_sem_init);
	return sem_post(sem->glibc);
}

int bionic_sem_wait(bionic_sem_t *sem)
{
	assert(sem);
	INIT_IF_NOT_MAPPED(sem, default_sem_init);
	return sem_wait(sem->glibc);
}

int bionic_sem_trywait(bionic_sem_t *sem)
{
	assert(sem);
	INIT_IF_NOT_MAPPED(sem, default_sem_init);
	return sem_trywait(sem->glibc);
}

int bionic_sem_clockwait(bionic_sem_t *restrict sem, clockid_t clock, const struct timespec *restrict abs_timeout)
{
	assert(sem && abs_timeout);
	INIT_IF_NOT_MAPPED(sem, default_sem_init);
#ifdef __GLIBC__
	return sem_clockwait(sem->glibc, clock, abs_timeout);
#else
	struct timespec converted_abs_timeout;
	realtime_time_from_monotonic_time(&converted_abs_timeout, abs_timeout);
	return sem_timedwait(sem->glibc, &converted_abs_timeout);
#endif
}

int bionic_sem_timedwait(bionic_sem_t *sem, const struct timespec *abs_timeout)
{
	assert(sem && abs_timeout);
	INIT_IF_NOT_MAPPED(sem, default_sem_init);
	return sem_timedwait(sem->glibc, abs_timeout);
}

int bionic_sem_timedwait_monotonic_np(bionic_sem_t* sem, const struct timespec* abs_timeout)
{
	return bionic_sem_clockwait(sem, CLOCK_MONOTONIC, abs_timeout);
}

/* bionic doesn't implement these, so we don't want to call the glibc/musl versions */
sem_t* bionic_sem_open(const char*, int, ...) {
	errno = ENOSYS;
	return SEM_FAILED;
}

int bionic_sem_close(sem_t*) {
	errno = ENOSYS;
	return -1;
}

int bionic_sem_unlink(const char*) {
	errno = ENOSYS;
	return -1;
}
