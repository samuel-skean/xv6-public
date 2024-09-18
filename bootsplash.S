.code16
.globl start
start:

# get cursor position into DH and DL
movb $0x03, %ah
int $0x10


# Setting the extended segment to the same as %ds, the default segment for data accesses.
movw %ds,   %ax
mov  %ax,   %es
movb $0x13, %ah # Picking the routine: Write string
movb $1,    %al # Write mode - chars only, moving cursor 
movb $0x0,  %bh # Trying a video page number of 0
movb $0x0f, %bl # Color
movw $12,   %cx # Length of the string
mov $hello, %bp # With a dollar, this loads the actual address at that label.
int $0x10

stop:
jmp stop

hello:
.string "Hello World!"

.org 0x1fe
.word 0xAA55
