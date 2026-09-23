#include <assert.h>
#include <string.h>
#include <sys/mman.h>

#include "common.h"

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
	// Apparently some android apps/libs (Qt5) do not call pthread_rwlock_init()
	assert(rwlock);
	rwlock->glibc = mmap(NULL, sizeof(*rwlock->glibc), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	memset(rwlock->glibc, 0, sizeof(*rwlock->glibc));
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
