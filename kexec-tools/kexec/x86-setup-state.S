	.code32
	/* Make certain the special registers are in a sane state.
	 * The kernel should have done this but...
	 */
x86_setup_state:
	/* Don't modify any registers... */
	pushl	%eax


	/* clear special bits in %cr4 */
	movl	%cr4, %eax
	andl	$0, %eax
	movl	%eax, %cr4

	popl	%eax
	ret