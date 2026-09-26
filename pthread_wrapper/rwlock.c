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

int bionic_pthread_rwlock_rdlock(bionic_rwlock_t *rwlock)
{
	assert(rwlock);
	INIT_IF_NOT_MAPPED(rwlock, default_rwlock_init);
	return pthread_rwlock_rdlock(rwlock->glibc);
}

int bionic_pthread_rwlock_unlock(bionic_rwlock_t *rwlock)
{
	assert(rwlock);
	INIT_IF_NOT_MAPPED(rwlock, default_rwlock_init);
	return pthread_rwlock_unlock(rwlock->glibc);
}

int bionic_pthread_rwlock_wrlock(bionic_rwlock_t *rwlock)
{
	assert(rwlock);
	INIT_IF_NOT_MAPPED(rwlock, default_rwlock_init);
	return pthread_rwlock_wrlock(rwlock->glibc);
}
