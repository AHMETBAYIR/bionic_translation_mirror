#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

// when __GLIBC__ is not defined, we're assuming musl; can't check to be sure because 🤡

// these are not defined on musl; FIXME: don't rely on these, call pthread_mutex_init
#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP {{PTHREAD_MUTEX_RECURSIVE}}
#endif

#ifndef PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP {{PTHREAD_MUTEX_ERRORCHECK}}
#endif

/* These structs have static initializers, so we need to wrap anything that touches them
 * and potentially lazy-init them. */

typedef struct {
	union {
#if defined(__LP64__)
		int32_t __private[10];
#else
		int32_t __private[1];
#endif
		pthread_mutex_t *glibc;
	};
} bionic_mutex_t;

typedef struct {
	union {
#if defined(__LP64__)
		int32_t __private[12];
#else
		int32_t __private[1];
#endif
		pthread_cond_t *glibc;
	};
} bionic_cond_t;

typedef struct {
	union {
		#if defined(__LP64__)
		  int32_t __private[14];
		#else
		  int32_t __private[10];
		#endif
		pthread_rwlock_t *glibc;
	};
} bionic_rwlock_t;

/* These structs don't have static initializers, but zero-initialing them happens to work
 * on bionic and some apps seem to unwittingly depend on that, so we need to wrap these as well */

typedef struct {
	union {
		struct {
			unsigned int count;
#ifdef __LP64__
			int __reserved[3];
#endif
		} bionic;
		sem_t *glibc;
	};
} bionic_sem_t;

/* why do we wrap these again...?
 * */

typedef struct {
	union {
		struct {
			uint32_t flags;
			void* stack_base;
			size_t stack_size;
			size_t guard_size;
			int32_t sched_policy;
			int32_t sched_priority;
#ifdef __LP64__
			char __reserved[16];
#endif
		} bionic;
		pthread_attr_t *glibc;
	};
} bionic_attr_t;

typedef struct {
	union {
		long __private;
		pthread_mutexattr_t *glibc;
	};
} bionic_mutexattr_t;

typedef struct {
	union {
		long __private;
		pthread_condattr_t *glibc;
	};
} bionic_condattr_t;

/* leaky implementation details, see usage */

struct bionic_pthread_cleanup_t {
	union {
		struct bionic_pthread_cleanup_t *prev;
#ifdef __GLIBC__
		__pthread_unwind_buf_t *glibc;
#else
		struct __ptcb *musl;
#endif
	};
	void (*routine)(void*);
	void *arg;
};

typedef int bionic_key_t;
_Static_assert(sizeof(bionic_key_t) == sizeof(pthread_key_t), "bionic_key_t and pthread_key_t size mismatch");

typedef int bionic_once_t;
_Static_assert(sizeof(bionic_once_t) == sizeof(pthread_once_t), "bionic_once_t and pthread_once_t size mismatch");

typedef long bionic_pthread_t;
_Static_assert(sizeof(bionic_pthread_t) == sizeof(pthread_t), "bionic_pthread_t and pthread_t size mismatch");

typedef uint64_t bionic_rwlockattr_t;
_Static_assert(sizeof(bionic_rwlockattr_t) == sizeof(pthread_rwlockattr_t), "bionic_rwlockattr_t and pthread_rwlockattr_t size mismatch");


#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

// For checking, if our glibc version is mapped to memory.
// Used for sanity checking and static initialization below.
#define IS_MAPPED(x) is_mapped(x->glibc, sizeof(*x))

// For handling static initialization.
#define INIT_IF_NOT_MAPPED(x, init) do { if (!IS_MAPPED(x)) init(x); } while(0)

bool is_mapped(void *mem, const size_t sz);
/* needed by cond.c, not just mutex.c */
void default_pthread_mutex_init(bionic_mutex_t *mutex);
