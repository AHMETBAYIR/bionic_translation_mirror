#include <assert.h>
#include <string.h>
#include <sys/mman.h>

#include "common.h"

int bionic_pthread_mutexattr_settype(bionic_mutexattr_t *attr, int type)
{
	assert(attr && IS_MAPPED(attr));
	return pthread_mutexattr_settype(attr->glibc, type);
}

int bionic_pthread_mutexattr_destroy(bionic_mutexattr_t *attr)
{
	assert(attr);
	int ret = 0;
	if (IS_MAPPED(attr)) {
		ret = pthread_mutexattr_destroy(attr->glibc);
		munmap(attr->glibc, sizeof(*attr->glibc));
	}
	return ret;
}

int bionic_pthread_mutexattr_init(bionic_mutexattr_t *attr)
{
	assert(attr);
	// From PTHREAD_MUTEXATTR_INIT(3)
	// The results of initializing an already initialized mutex attributes object are undefined.
	*attr = (bionic_mutexattr_t){0};
	attr->glibc = mmap(NULL, sizeof(*attr->glibc), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	return pthread_mutexattr_init(attr->glibc);
}
