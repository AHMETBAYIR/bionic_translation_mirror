#include "common.h"

int bionic_pthread_rwlockattr_init(struct _bionic_rwlockattr_t* attr)
{
	*attr = (struct _bionic_rwlockattr_t){0};
	return 0;
}

int bionic_pthread_rwlockattr_destroy(struct _bionic_rwlockattr_t* attr)
{
	return 0;
}

int bionic_pthread_rwlockattr_getpshared(const struct _bionic_rwlockattr_t* attr, int* shared)
{
	*shared = attr->shared;
	return 0;
}

int bionic_pthread_rwlockattr_setpshared(struct _bionic_rwlockattr_t* attr, int shared)
{
	attr->shared = shared;
	return 0;
}

#ifdef __GLIBC__ /* musl doesn't have the these, for now let them fail to link */
int bionic_pthread_rwlockattr_getkind_np(const struct _bionic_rwlockattr_t* attr, int* kind){
	*kind = attr->kind;
	return 0;
}

int bionic_pthread_rwlockattr_setkind_np(struct _bionic_rwlockattr_t* attr, int kind)
{
	attr->kind = kind;
	return 0;
}
#endif
