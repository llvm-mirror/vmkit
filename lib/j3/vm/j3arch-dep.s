	.section	__DATA,__data
	.globl _trampoline_offset, __ZN5vmkit15ThreadAllocator6_magicE
	.globl _trampoline_generic, _trampoline_generic_method, _trampoline_generic_resolver, _trampoline_generic_end
	
_trampoline_generic:
	movabsq $_trampoline_save, %rax   /* absolute to avoid any problem of relocation */
	callq		*%rax
	.byte 0x48, 0xbe  					      /* movabsq _trampoline_generic_method, %rsi (i.e., replace second arg) */
_trampoline_generic_method:
	.quad 0
	.byte 0x48, 0xb8  					      /* movabsq _trampoline_generic_resolver, %rax */
_trampoline_generic_resolver:	
	.quad 0
	jmpq *%rax
_trampoline_generic_end:	

	.section	__TEXT,__text,regular,pure_instructions
	.globl _trampoline_restart
	.align	4
	
_trampoline_save:
	mov 		%rsp, %rax
	and 		__ZN5vmkit15ThreadAllocator6_magicE(%rip), %rax
	add 		_trampoline_offset(%rip), %rax

	mov 		%xmm0, 0(%rax)
	mov 		%xmm1, 16(%rax)
	mov 		%xmm2, 32(%rax)
	mov 		%xmm3, 48(%rax)
	mov 		%xmm4, 64(%rax)
	mov 		%xmm5, 80(%rax)
	mov 		%xmm6, 96(%rax)
	mov 		%xmm7, 112(%rax)
	mov		 	%rdi,  128(%rax)
	mov 		%rsi,  136(%rax)
	mov 		%rdx,  144(%rax)
	mov 		%rcx,  152(%rax)
	mov 		%r8,   160(%rax)
	mov 		%r9,   168(%rax)
	mov 		%rbp,  176(%rax)
	mov     %rsp,  184(%rax)
	addq		$8, 184(%rax)    /* my own call :) */
	
	ret

	/* void trampoline_restart(void* ptr, void* saveZone); */
	/* %rsi: saveZone */
	/* %rdi: pointer to function */
_trampoline_restart:
	mov     %rdi, %rax

	mov 		0(%rsi), %xmm0
	mov 		16(%rsi), %xmm1
	mov 		32(%rsi), %xmm2
	mov 		48(%rsi), %xmm3
	mov 		64(%rsi), %xmm4
	mov 		80(%rsi), %xmm5
	mov 		96(%rsi), %xmm6
	mov 		112(%rsi), %xmm7
	mov		 	128(%rsi), %rdi
	mov 		144(%rsi), %rdx
	mov 		152(%rsi), %rcx
	mov 		160(%rsi), %r8
	mov 		168(%rsi), %r9
	mov			176(%rsi), %rbp
	mov			184(%rsi), %rsp
	
	mov 		136(%rsi), %rsi

	jmpq		*%rax
