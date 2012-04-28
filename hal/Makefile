# Makefile for hal module

include ../Makefile.inc

OBJ := ../bin/obji386

TARGETOBJ := $(OBJ)/hal.o \
	$(OBJ)/dscptr.o \
	$(OBJ)/interrupt.o \
	$(OBJ)/isr.o \
	$(OBJ)/spinlock.o \
	$(OBJ)/lapic.o \


.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -DBITS_PER_LONG=32 -D_X86_ -m32 -g -I../include $(CFLAGS_global) -c -o $@ $<

$(OBJ)/dscptr.o:
	nasm $(ASFLAGS) -o $(OBJ)/dscptr.o dscptr.s

$(OBJ)/interrupt.o:
	nasm $(ASFLAGS) -o $(OBJ)/interrupt.o interrupt.s

clean:
	$(RM) $(TARGETOBJ)
