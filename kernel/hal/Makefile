# Makefile for hal module

include ../Makefile.inc

OBJ := ../../bin/obji386

TARGETOBJ := $(OBJ)/hal.o \
	$(OBJ)/dscptr.o \
	$(OBJ)/interrupt.o \
	$(OBJ)/isr.o \
	$(OBJ)/spinlock.o \
	$(OBJ)/core.o \
	$(OBJ)/lapic.o \

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D__KERNEL__ -D_X86_ -DBITS_PER_LONG=32 -m32 -I../include -I../../sdk/include $(CFLAGS_global) -c -o $@ $<

$(OBJ)/dscptr.o:
	nasm $(ASFLAGS) -o $(OBJ)/dscptr.o dscptr.s

$(OBJ)/interrupt.o:
	nasm $(ASFLAGS) -o $(OBJ)/interrupt.o interrupt.s

clean:
	$(RM) $(TARGETOBJ)
