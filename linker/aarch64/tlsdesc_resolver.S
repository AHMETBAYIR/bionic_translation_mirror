
/* uintptr_t bionic___tls_get_addr(struct tls_index *idx); */
.globl bionic___tls_get_addr

#define SAVE_GPR_PAIR(x, y, slot)		\
	stp x, y, [sp, #((slot) * 8)];		\
	.cfi_rel_offset x, (slot) * 8;		\
	.cfi_rel_offset y, ((slot) + 1) * 8;	\

#define SAVE_VEC_PAIR(x, y, slot)		\
	stp x, y, [sp, #((slot) * 8)];		\
	.cfi_rel_offset x, (slot) * 8;		\
	.cfi_rel_offset y, ((slot) + 2) * 8;	\

#define RESTORE_REG_PAIR(x, y, slot)		\
	ldp x, y, [sp, #((slot) * 8)];		\
	.cfi_restore x;				\
	.cfi_restore y;				\

/*
 * uintptr_t tlsdesc_resolver_dynamic(struct aarch64_tls_descriptor *desc);
 *
 * This function is not allowed to clobber any registers and shall return x0
 * with an offset relative to tpidr_el0 to the library-specific tls storage.
 * The application will add the value of tpidr_el0 back and get the pointer.
 *
 * This implementation does roughly this:
 *
 * uintptr_t tlsdesc_resolver_dynamic(struct aarch64_tls_descriptor *desc)
 * {
 *	uintptr_t storage = bionic___tls_get_addr(desc->index);
 *	return (storage - thread_pointer);
 * }
 */
.global tlsdesc_resolver_dynamic
tlsdesc_resolver_dynamic:
	.cfi_startproc

	sub sp, sp, #(8 * 84)
	.cfi_def_cfa_offset (8 * 84)
	SAVE_GPR_PAIR(x29, x30, 0)
	mov x29, sp
	
	SAVE_GPR_PAIR(x1,  x2,  2)
	SAVE_GPR_PAIR(x3,  x4,  4)
	SAVE_GPR_PAIR(x5,  x6,  6)
	SAVE_GPR_PAIR(x7,  x8,  8)
	SAVE_GPR_PAIR(x9,  x10, 10)
	SAVE_GPR_PAIR(x11, x12, 12)
	SAVE_GPR_PAIR(x13, x14, 14)
	SAVE_GPR_PAIR(x15, x16, 16)
	SAVE_GPR_PAIR(x17, x18, 18)
	
	SAVE_VEC_PAIR(q0,  q1,  20)
	SAVE_VEC_PAIR(q2,  q3,  24)
	SAVE_VEC_PAIR(q4,  q5,  28)
	SAVE_VEC_PAIR(q6,  q7,  32)
	SAVE_VEC_PAIR(q8,  q9,  36)
	SAVE_VEC_PAIR(q10, q11, 40)
	SAVE_VEC_PAIR(q12, q13, 44)
	SAVE_VEC_PAIR(q14, q15, 48)
	SAVE_VEC_PAIR(q16, q17, 52)
	SAVE_VEC_PAIR(q18, q19, 56)
	SAVE_VEC_PAIR(q20, q21, 60)
	SAVE_VEC_PAIR(q22, q23, 64)
	SAVE_VEC_PAIR(q24, q25, 68)
	SAVE_VEC_PAIR(q26, q27, 72)
	SAVE_VEC_PAIR(q28, q29, 76)
	SAVE_VEC_PAIR(q30, q31, 80)
	
	ldr x0, [x0, #8]		/* x0 = desc->index */
	bl bionic___tls_get_addr	/* x0 = bionic___tls_get_addr(desc->index) */
	mrs x1, tpidr_el0		/* x1 = thread_pointer */
	sub x0, x0, x1			/* x0 -= thread_pointer */
	
	RESTORE_REG_PAIR(q30, q31, 80)
	RESTORE_REG_PAIR(q28, q29, 76)
	RESTORE_REG_PAIR(q26, q27, 72)
	RESTORE_REG_PAIR(q24, q25, 68)
	RESTORE_REG_PAIR(q22, q23, 64)
	RESTORE_REG_PAIR(q20, q21, 60)
	RESTORE_REG_PAIR(q18, q19, 56)
	RESTORE_REG_PAIR(q16, q17, 52)
	RESTORE_REG_PAIR(q14, q15, 48)
	RESTORE_REG_PAIR(q12, q13, 44)
	RESTORE_REG_PAIR(q10, q11, 40)
	RESTORE_REG_PAIR(q8,  q9,  36)
	RESTORE_REG_PAIR(q6,  q7,  32)
	RESTORE_REG_PAIR(q4,  q5,  28)
	RESTORE_REG_PAIR(q2,  q3,  24)
	RESTORE_REG_PAIR(q0,  q1,  20)
	
	RESTORE_REG_PAIR(x17, x18, 18)
	RESTORE_REG_PAIR(x15, x16, 16)
	RESTORE_REG_PAIR(x13, x14, 14)
	RESTORE_REG_PAIR(x11, x12, 12)
	RESTORE_REG_PAIR(x9,  x10, 10)
	RESTORE_REG_PAIR(x7,  x8,  8)
	RESTORE_REG_PAIR(x5,  x6,  6)
	RESTORE_REG_PAIR(x3,  x4,  4)
	RESTORE_REG_PAIR(x1,  x2,  2)
	
	RESTORE_REG_PAIR(x29, x30, 0)
	add sp, sp, #(8 * 84)
	.cfi_def_cfa_offset 0
	ret

	.cfi_endproc
