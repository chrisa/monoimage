	.text
	.code32
setup_legacy_pic:
	/* Load the legacy dos settings into the 8259A pic */
	movb	$0xff, %al
	outb	%al, $0x21	/* mask all of 8259A-1 */
	outb	%al, $0xa1	/* mask all of 8259A-1 */

	movb	$0x11, %al
	outb	%al, $0x20	/* ICW1: select 8259A-1 init */
	outb	%al, $0x80	/* A short delay */
	movb	$0x08, %al
	outb	%al, $0x21	/* ICW2: 8259A-1 IR0-7 mappend to 0x8-0xf */
	outb	%al, $0x80	/* A short delay */
	movb	$01, %al
	outb	%al, $0x21	/* Normal 8086 auto EOI mode */
	outb	%al, $0x80	/* A short delay */
	

	movb	$0x11, %al
	outb	%al, $0xA0	/* ICW1: select 8259A-2 init */
	outb	%al, $0x80	/* A short delay */
	movb	$0x70, %al
	outb	%al, $0xA1	/* ICW2: 8259A-2 IR0-7 mappend to 0x70-0x77 */
	outb	%al, $0x80	/* A short delay */
	movb	$01, %al
	outb	%al, $0xA1	/* Normal 8086 auto EOI mode */
	outb	%al, $0x80	/* A short delay */
	
	movb	$0, %al
	outb	%al, $0x21	/* Unmask all of 8259A-1 */
	outb	%al, $0xa1	/* Unmask all of 8259A-2 */

	ret
