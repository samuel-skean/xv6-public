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
movw $welcome, %bp # With a dollar, this loads the actual address of that label.
int $0x10

# Wait for a keypress.
movb $0x00, %ah
int $0x16

# Change video mode:
movb $0x0, %ah
movb $0x13, %al
int $0x10

# Put a white pixel on the screen:
movw $0xa000, %ax  # How many 0s should it have? A: 3, because it will be multiplied by 16
movw %ax, %ds  # In i8086 you cannot move imm directly to a *s register.
movb $0xf, %al # A white pixel
movw $0x7da0, %di   # The first pixel at the top-left corner of the screen
movb %al, %ds:(%di) # After this instruction you can see the tiny white dot there. (maximize window to make it more visible)


stop:
jmp stop

welcome:
.string "Welcome to xv6@UIC. Press any key to continue"

.org 0x1fe
.word 0xAA55
