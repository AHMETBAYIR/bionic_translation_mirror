#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/mman.h>

#include "common.h"

int bionic_pthread_attr_destroy(bionic_attr_t *attr)
{
	assert(attr);
	int ret = 0;
	if (IS_MAPPED(attr)) {
		ret = pthread_attr_destroy(attr->glibc);
		munmap(attr->glibc, sizeof(*attr->glibc));
	}
	return ret;
}

int bionic_pthread_attr_init(bionic_attr_t *attr)
{
	assert(attr);
	// From PTHREAD_ATTR_INIT(3)
	// Calling `pthread_attr_init` on a thread attributes object that has already been initialized results in ud.
	*attr = (bionic_attr_t){0};
	attr->glibc = mmap(NULL, sizeof(*attr->glibc), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	return pthread_attr_init(attr->glibc);
}

int bionic_pthread_getattr_np(bionic_pthread_t thread, bionic_attr_t *attr)
{
	assert(thread && attr);
	*attr = (bionic_attr_t){0};
	attr->glibc = mmap(NULL, sizeof(*attr->glibc), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	return pthread_getattr_np((pthread_t)thread, attr->glibc);
}

int bionic_pthread_attr_settstack(bionic_attr_t *attr, void *stackaddr, size_t stacksize)
{
	assert(attr && IS_MAPPED(attr));
	return pthread_attr_setstack(attr->glibc, stackaddr, stacksize);
}

int bionic_pthread_attr_getstack(const bionic_attr_t *attr, void *stackaddr, size_t *stacksize)
{
	assert(attr && IS_MAPPED(attr));
	return pthread_attr_getstack(attr->glibc, stackaddr, stacksize);
}

#if defined(__LP64__)
#define BIONIC_PTHREAD_STACK_MIN 16384
#else
#define BIONIC_PTHREAD_STACK_MIN 8192
#endif

int bionic_pthread_attr_setstacksize(bionic_attr_t *attr, size_t stacksize)
{
	assert(attr && IS_MAPPED(attr));
	
	if (stacksize < BIONIC_PTHREAD_STACK_MIN)
		return EINVAL;

	if (stacksize < PTHREAD_STACK_MIN)
		stacksize = PTHREAD_STACK_MIN;

	return pthread_attr_setstacksize(attr->glibc, stacksize);
}

int bionic_pthread_attr_getstacksize(const bionic_attr_t *attr, size_t *stacksize)
{
	assert(attr && IS_MAPPED(attr));
	return pthread_attr_getstacksize(attr->glibc, stacksize);
}

int bionic_pthread_attr_setschedpolicy(bionic_attr_t *attr, int policy)
{
	assert(attr && IS_MAPPED(attr));
	return pthread_attr_setschedpolicy(attr->glibc, policy);
}

int bionic_pthread_attr_getschedpolicy(bionic_attr_t *attr, int *policy)
{
	assert(attr && IS_MAPPED(attr));
	return pthread_attr_getschedpolicy(attr->glibc, policy);
}

int bionic_pthread_attr_setschedparam(bionic_attr_t *attr, const struct sched_param *param)
{
#ifndef __GLIBC__ //musl
	assert(0 && "bionic_pthread_attr_setschedparam: FIXME: struct sched_param needs wrapping on musl");
#endif
	assert(attr && IS_MAPPED(attr));
	return pthread_attr_setschedparam(attr->glibc, param);
}

int bionic_pthread_attr_getschedparam(bionic_attr_t *attr, struct sched_param *param)
{
#ifndef __GLIBC__ //musl
	assert(0 && "bionic_pthread_attr_getschedparam: FIXME: struct sched_param needs wrapping on musl");
#endif
	assert(attr && IS_MAPPED(attr));
	return pthread_attr_getschedparam(attr->glibc, param);
}

static_assert(PTHREAD_CREATE_DETACHED == 1);
static_assert(PTHREAD_CREATE_JOINABLE == 0);

int bionic_pthread_attr_setdetachstate(bionic_attr_t *attr, int detachstate)
{
	assert(attr && IS_MAPPED(attr));
	return pthread_attr_setdetachstate(attr->glibc, detachstate);
}

int bionic_pthread_attr_getdetachstate(bionic_attr_t *attr, int *detachstate)
{
	assert(attr && IS_MAPPED(attr));
	return pthread_attr_getdetachstate(attr->glibc, detachstate);
}

int bionic_pthread_attr_getguardsize(const bionic_attr_t *attr, size_t *guardsize)
{
	assert(attr && IS_MAPPED(attr));
	return pthread_attr_getguardsize(attr->glibc, guardsize);
}

int bionic_pthread_attr_setguardsize(bionic_attr_t *attr, int guardsize)
{
	assert(attr && IS_MAPPED(attr));
	return pthread_attr_setguardsize(attr->glibc, guardsize);
}

#define BIONIC_PTHREAD_EXPLICIT_SCHED 0
#define BIONIC_PTHREAD_INHERIT_SCHED 1

int bionic_pthread_attr_getinheritsched(const bionic_attr_t *restrict attr, int *restrict inherit)
{
	assert(attr && IS_MAPPED(attr));
	int glibc_inherit;
	int ret = pthread_attr_getinheritsched(attr->glibc, &glibc_inherit);
	switch (glibc_inherit) {
		case PTHREAD_EXPLICIT_SCHED:
			*inherit = BIONIC_PTHREAD_EXPLICIT_SCHED;
			break;
		case PTHREAD_INHERIT_SCHED:
			*inherit = BIONIC_PTHREAD_INHERIT_SCHED;
			break;
		default:
			*inherit = glibc_inherit;
			break;
	}
	return ret;
}

int bionic_pthread_attr_setinheritsched(bionic_attr_t *attr, int inherit)
{
	int glibc_inherit;
	assert(attr && IS_MAPPED(attr));
	switch (inherit) {
		case BIONIC_PTHREAD_EXPLICIT_SCHED:
			glibc_inherit = PTHREAD_EXPLICIT_SCHED;
			break;
		case BIONIC_PTHREAD_INHERIT_SCHED:
			glibc_inherit = PTHREAD_INHERIT_SCHED;
			break;
		default:
			glibc_inherit = inherit;
			break;
	}
	return pthread_attr_setinheritsched(attr->glibc, glibc_inherit);
}

static_assert(PTHREAD_SCOPE_SYSTEM == 0);
static_assert(PTHREAD_SCOPE_PROCESS == 1);

int bionic_pthread_attr_getscope(const bionic_attr_t *restrict attr, int *restrict scope)
{
	assert(attr && IS_MAPPED(attr));
	return pthread_attr_getscope(attr->glibc, scope);
}

int bionic_pthread_attr_setscope(bionic_attr_t *attr, int scope)
{
	assert(attr && IS_MAPPED(attr));
	return pthread_attr_setscope(attr->glibc, scope);
}

