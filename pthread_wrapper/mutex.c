#include <assert.h>
#include <string.h>
#include <sys/mman.h>

#include "common.h"

static const struct {
	bionic_mutex_t bionic;
	pthread_mutex_t glibc;
} bionic_mutex_init_map[] = {
	{ .bionic = {{{ ((PTHREAD_MUTEX_NORMAL & 3) << 14) }}}, .glibc = PTHREAD_MUTEX_INITIALIZER },
	{ .bionic = {{{ ((PTHREAD_MUTEX_RECURSIVE & 3) << 14) }}}, .glibc = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP },
	{ .bionic = {{{ ((PTHREAD_MUTEX_ERRORCHECK & 3) << 14) }}}, .glibc = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP },
};

void default_pthread_mutex_init(bionic_mutex_t *mutex)
{
	assert(mutex);

	for (size_t i = 0; i < ARRAY_SIZE(bionic_mutex_init_map); i++) {
		if (memcmp(&bionic_mutex_init_map[i].bionic, mutex, sizeof(*mutex)))
			continue;

		mutex->glibc = mmap(NULL, sizeof(*mutex->glibc), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		memcpy(mutex->glibc, &bionic_mutex_init_map[i].glibc, sizeof(bionic_mutex_init_map[i].glibc));
		return;
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

int
bionic_pthread_mutex_lock(bionic_mutex_t *mutex)
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
