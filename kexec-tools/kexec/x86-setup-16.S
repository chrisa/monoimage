.data
	.globl setup16_start, setup16_end, setup16_size, setup16_align
	.globl setup16_regs
setup16_start:
_reloc = .
	.balign 16
	.code32
	/* Compute where I am running at */
	call	1f
1:	popl	%ebx
	subl	$(1b - _reloc), %ebx

	/* Make certain the special registers are in a sane state.
	 * The kernel should have done this but...
	 */
	call	x86_setup_state
	
	/* Fixup my real mode segment */
	movl	%ebx, %eax
	shrl	$4, %eax
	movw	%ax, 2 + realptr - _reloc(%ebx)

	/* Fixup the gdt */
	movl	%ebx, %eax
	shll	$16, %eax
	
	movl	%ebx, %ecx
	shrl	$16, %ecx
	andl	$0xff, %ecx

	movl	%ebx, %edx
	andl	$0xff000000, %edx
	orl	%edx, %ecx
	
	addl	%ebx, gdtaddr - _reloc(%ebx)
	orl	%eax, 0x08 + gdt - _reloc(%ebx)
	orl	%ecx, 0x0c + gdt - _reloc(%ebx)
	orl	%eax, 0x10 + gdt - _reloc(%ebx)
	orl	%ecx, 0x14 + gdt - _reloc(%ebx)

	/* Setup the classic BIOS interrupt table at 0x0 */
	lidt	idtptr - _reloc(%ebx)
	
	/* Provide us with 16bit segments that we can use */
	lgdt	gdtptr - _reloc(%ebx)

	/* Note we don't disable the a20 line, (this shouldn't be required)
	 * The code to do it is in kexec_test and it is a real pain.
	 * I will worry about that when I need it.
	 */
	
	/* Load 16bit data segments, to ensure the segment limits are set */
	movl	$0x10, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %ss
	movl	%eax, %fs
	movl	%eax, %gs

	/* switch to 16bit mode */
	
	ljmp	$0x08, $2f - _reloc
2:
	.code16
	/* Disable Paging and protected mode */
	/* clear the PG & PE bits of CR0 */
	movl	%cr0,%eax
	andl	$~((1 << 31)|(1<<0)),%eax
	movl	%eax,%cr0

	/* make intersegment jmp to flush the processor pipeline
	 * and reload %cs:%eip (to clear upper 16 bits of %eip).
	 */
	ljmp	*(realptr - _reloc)
3:
	/* we are in real mode now
	 * set up the real mode segment registers : %ds, $ss, %es
	 */
	/* Setup the data segment */
	movw	%cs, %ax
	movw	%ax, %ds

	/* Load the registers */
	movl	eax - _reloc, %eax
	movl	ebx - _reloc, %ebx
	movl	ecx - _reloc, %ecx
	movl	edx - _reloc, %edx
	movl	esi - _reloc, %esi
	movl	edi - _reloc, %esi
	movl	esp - _reloc, %esp
	movl	ebp - _reloc, %ebp
	movw	es - _reloc, %es
	movw	ss - _reloc, %ss
	movw	fs - _reloc, %fs
	movw	gs - _reloc, %gs
	movw	ds - _reloc, %ds

	/* Jump to the kernel entrypoint */
	ljmp	%cs:*(realdest - _reloc)

setup16_regs:	
eax:	.long	0x00000000
ebx:	.long	0x00000000
ecx:	.long	0x00000000
edx:	.long	0x00000000
esi:	.long	0x00000000
edi:	.long	0x00000000
esp:	.long	0x00000000
ebp:	.long	0x00000000
ds:	.word	0x0000
es:	.word	0x0000
ss:	.word	0x0000
fs:	.word	0x0000
gs:	.word	0x0000
realdest:
ip:	.word	0x0000
cs:	.word	0x0000

	.balign 16
realptr:		
	.word	3b - _reloc
	.word	0x0000
	
idtptr:
	/* 256 entry idt at 0 */
	.word	0x400 - 1
	.word	0, 0

gdtptr:
	.word	gdt_end - gdt - 1
gdtaddr:
	.long	gdt - _reloc

gdt:
	/* dummy */
	.word 0, 0, 0, 0

	/* 16 bit real mode code segment */
	.word	0xffff, 0x0000
	.byte	0x00, 0x9b, 0x00, 0x00

	/* 16 bit real mode data segment */
	.word	0xffff, 0x0000
	.byte	0x00, 0x93, 0x00, 0x00
gdt_end:
#include "x86-setup-state.S"
#if 0
#include "x86-setup-legacy-pic.S"
#include "x86-setup-legacy-timer.S"
#endif
setup16_end:
setup16_size:
	.long setup16_end - setup16_start
setup16_align:
	.long 16
