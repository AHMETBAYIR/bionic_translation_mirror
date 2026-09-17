#ifndef LINKER_TLS_H
#define LINKER_TLS_H

#include <stddef.h>
#include <stdint.h>

struct tls_index
{
	size_t module;
	size_t offset;
};

#ifdef __aarch64__
struct aarch64_tls_descriptor
{
	uintptr_t (*resolver_func)(struct aarch64_tls_descriptor *);
	struct tls_index *index;
};

uintptr_t tlsdesc_resolver_dynamic(struct aarch64_tls_descriptor *desc);
#endif

size_t __tls_register_module(void *template_base, size_t template_size, size_t size, int align);

#endif
