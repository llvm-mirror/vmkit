	.section	__DATA,__data
	.globl	_trampoline_mask, _trampoline_offset
	.globl _trampoline_generic, _trampoline_generic_method, _trampoline_generic_resolver, _trampoline_generic_end
	
_trampoline_mask:
	.quad	0
_trampoline_offset:
	.quad 0
	
_trampoline_generic:
	.byte  0x48, 0xb8     			/* mov _trampoline_save, %rax (absolute adressing, what is the mnemonic?) */
	.quad  _trampoline_save
	callq		*%rax
	mov			%rsp,  184(%rax)
	.byte 0x48, 0xbe  					/* mov _trampoline_generic_method, %rsi */
_trampoline_generic_method:
	.quad 0
	.byte 0x48, 0xb8  					/* mov _trampoline_generic_resolver, %rax */
_trampoline_generic_resolver:	
	.quad 0
	jmpq *%rax
_trampoline_generic_end:	

	.section	__TEXT,__text,regular,pure_instructions
	.globl _trampoline_save, _trampoline_restart
	.align	4
	
	/* compute the adress of the save zone area */
	/* and return the adress in %rax */
_trampoline_get_save_zone:	
	push 		%rbx
	mov 		%rsp, %rax
	movq 		_trampoline_mask(%rip), %rbx
	and 		%rbx, %rax
	movq 		_trampoline_offset(%rip), %rbx
	add			%rbx, %rax
	pop			%rbx
	ret
	
_trampoline_save:
	call    _trampoline_get_save_zone

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

	ret

	/* %rdi contains the function */
	/* %rsi contains the save zone area */
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
