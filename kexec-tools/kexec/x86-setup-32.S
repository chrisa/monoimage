.data
.code32
	.globl setup32_start, setup32_end, setup32_size
	.globl setup32_regs
setup32_start:
_reloc = .
	/* Compute where I am running at */
	call	1f
1:	popl	%ebx
	subl	$(1b - _reloc), %ebx

	/* align the code, I feel better when word load & store instructions
	 * are aligned.
	 */
	nop

	/* Relocate the code */
	addl	%ebx, reloc1 - _reloc(%ebx)
	addl	%ebx, reloc2 - _reloc(%ebx)
	addl	%ebx, reloc3 - _reloc(%ebx)

	/* Make certain the special registers are in a sane state.
	 * The kernel should have done this but...
	 */
	call	x86_setup_state

	/* Setup a gdt that should be preserved */
	/* This also acts as a serializing instruction ensuring
	 * my self modifying code works.
	 */
	lgdt	gdt - _reloc(%ebx)

	/* load the data segments */
	movl	$0x18, %eax	/* data segment */
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %ss
	movl	%eax, %fs
	movl	%eax, %gs

	/* load the code segment */
	leal	1f - _reloc(%ebx), %eax
	pushl	$0x10		/* code segment */
	pushl	%eax		/* destination address */
	lret
1:	

	/* Load the registers */
	movl	eax - _reloc(%ebx), %eax
	movl	ecx - _reloc(%ebx), %ecx
	movl	edx - _reloc(%ebx), %edx
	movl	esi - _reloc(%ebx), %esi
	movl	edi - _reloc(%ebx), %edi
	movl	esp - _reloc(%ebx), %esp
	movl	ebp - _reloc(%ebx), %ebp

	.byte 0x8b, 0x1d	# movl ebx,  %ebx
reloc1:	.long ebx - _reloc

	nop
	nop

	.byte 0xff, 0x25	# jmpl *(eip)
reloc2:	.long eip - _reloc

	.balign 4
setup32_regs:
eax:	.long 0x00000000
ebx:	.long 0x00000000
ecx:	.long 0x00000000
edx:	.long 0x00000000
esi:	.long 0x00000000
edi:	.long 0x00000000
esp:	.long 0x00000000
ebp:	.long 0x00000000
eip:	.long 0x00000000

gdt:
	/* 0x00 unusable segment so used as the gdt ptr */
	.word	gdt_end - gdt - 1
reloc3:
	.long	gdt - _reloc
	.word	0

	/* 0x08 dummy */
	.word	0x0000, 0x0000, 0x0000, 0x000

	/* Documented linux kernel segments */
	/* 0x10 4GB flat code segment */
	.word	0xFFFF, 0x0000, 0x9A00, 0x00CF
	/* 0x18 4GB flat data segment */
	.word	0xFFFF, 0x0000, 0x9200, 0x00CF

	/* 0x20 dummy */
	.word	0x0000, 0x0000, 0x0000, 0x000
	/* 0x28 dummy */
	.word	0x0000, 0x0000, 0x0000, 0x000
	/* 0x30 dummy */
	.word	0x0000, 0x0000, 0x0000, 0x000
	/* 0x38 dummy */
	.word	0x0000, 0x0000, 0x0000, 0x000
	/* 0x40 dummy */
	.word	0x0000, 0x0000, 0x0000, 0x000
	/* 0x48 dummy */
	.word	0x0000, 0x0000, 0x0000, 0x000
	/* 0x50 dummy */
	.word	0x0000, 0x0000, 0x0000, 0x000
	/* 0x58 dummy */
	.word	0x0000, 0x0000, 0x0000, 0x000
	
	/* Segments used by the 2.5.x kernel */
	/* 0x60 4GB flat code segment */
	.word	0xFFFF, 0x0000, 0x9A00, 0x00CF
	/* 0x68 4GB flat data segment */
	.word	0xFFFF, 0x0000, 0x9200, 0x00CF
gdt_end:
#include "x86-setup-state.S"	
setup32_end:
setup32_size:
	.long setup32_end - setup32_start
