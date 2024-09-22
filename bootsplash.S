.code16
.globl start
start:

xor %ax, %ax
# Zero out my segment registers.
mov %ax, %ds
mov %ax, %es
mov %ax, %ss

movb %dl, 0x10 # Save the boot drive number.

# get cursor position into %dh and %dl
movb $0x03, %ah
int $0x10


movb $0x13, %ah # Picking the routine: Write string
movb $1,    %al # Write mode - chars only, moving cursor
movb $0x0,  %bh # Trying a video page number of 0
movb $0x0f, %bl # Color
movw $45,   %cx # Length of the string
movw $welcome, %bp # With a dollar, this loads the actual address of that label.
int $0x10

# Wait for a keypress.
movb $0x00, %ah
int $0x16

# Change video mode:
movb $0x0, %ah
movb $0x13, %al
int $0x10

movb 0x10, %dl # Load the boot drive number.

# Reading from disk:
movb $0x42, %ah
# %dl already holds the boot drive number
movw $dap, %si # Apparently I shoulda used lea.
int $0x13


stop:
jmp stop

welcome:
.string "Welcome to xv6@UIC. Press any key to continue"

dap: # Disk Access Packet
.byte 0x10 # Size of this packet
.byte 0x0 # Reserved, should be 0
.word 125 # The number of blocks to read
.long 0xa0000000 # Segment:offset of graphics memory
# This is always the graphics memory when using this graphics mode.
.quad 0x1


.org 0x1fe
.word 0xAA55
