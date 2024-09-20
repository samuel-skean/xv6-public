.code16
.globl start
start:

xor %ax, %ax
# Zero out my segment registers.
mov %ax, %ds
mov %ax, %es
mov %ax, %ss


# get cursor position into %dh and %dl
movb $0x03, %ah
int $0x10


movb $0x13, %ah # Picking the routine: Write string
movb $1,    %al # Write mode - chars only, moving cursor 
movb $0x0,  %bh # Trying a video page number of 0
movb $0x0f, %bl # Color
movw $45,   %cx # Length of the string
movw $welcome, %bp # With a dollar, this loads the actual address at that label.
int $0x10

# Wait for a keypress.
movb $0x00, %ah
int $0x16

# Print it again, just so we know we got the keypress:
# get cursor position into %dh and %dl
movb $0x03, %ah
int $0x10

movb $0x13, %ah # Picking the routine: Write string
movb $1,    %al # Write mode - chars only, moving cursor 
movb $0x0,  %bh # Trying a video page number of 0
movb $0x0f, %bl # Color
movw $45,   %cx # Length of the string
movw $welcome, %bp # With a dollar, this loads the actual address at that label.
int $0x10

stop:
jmp stop

welcome:
.string "Welcome to xv6@UIC. Press any key to continue"

.org 0x1fe
.word 0xAA55
