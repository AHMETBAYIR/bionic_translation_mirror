#include <assert.h>
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
	memset(sem->glibc, 0, sizeof(*sem->glibc));
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

int bionic_sem_timedwait(bionic_sem_t *sem, const struct timespec *abs_timeout)
{
	assert(sem && abs_timeout);
	INIT_IF_NOT_MAPPED(sem, default_sem_init);
	return sem_timedwait(sem->glibc, abs_timeout);
}
