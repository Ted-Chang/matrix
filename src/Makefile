# Makefile for matrix kernel
# The C and C++ rules are already setup by default.
# The only one that needs changing is the assembler
# rule, as we use nasm instead of GNU as

SOURCES=boot.o main.o hal.o util.o interrupt.o isr.o dscptr.o stdio.o string.o

CFLAGS=-m32 -Iinc -nostdlib -nostdinc -fno-builtin -fno-stack-protector
LDFLAGS=-TLink.ld
ASFLAGS=-felf

all: $(SOURCES) link

clean:
	-rm *.o matrix

link:
	ld -melf_i386 $(LDFLAGS) -o matrix $(SOURCES)

.s.o:
	nasm $(ASFLAGS) $<