#include <assert.h>
#include <string.h>
#include <sys/mman.h>

#include "common.h"

int bionic_pthread_condattr_destroy(bionic_condattr_t *attr)
{
	assert(attr);
	int ret = 0;
	if (IS_MAPPED(attr)) {
		ret = pthread_condattr_destroy(attr->glibc);
		munmap(attr->glibc, sizeof(*attr->glibc));
	}
	return ret;
}

int bionic_pthread_condattr_init(bionic_condattr_t *attr)
{
	*attr = (bionic_condattr_t){0};
	attr->glibc = mmap(NULL, sizeof(*attr->glibc), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	return pthread_condattr_init(attr->glibc);
}

int bionic_pthread_condattr_setclock(bionic_condattr_t *attr, clockid_t clock_id)
{
	assert(attr && IS_MAPPED(attr));
	return pthread_condattr_setclock(attr->glibc, clock_id);
}
