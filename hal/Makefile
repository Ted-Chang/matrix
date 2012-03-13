# Makefile for hal module

include ../Makefile.inc

OBJ := ../obj

TARGETOBJ := $(OBJ)/hal.o $(OBJ)/dscptr.o $(OBJ)/interrupt.o $(OBJ)/isr.o

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -m32 -I../include $(CFLAGS_global) -c -o $@ $<

$(OBJ)/dscptr.o:
	nasm $(ASFLAGS) -o $(OBJ)/dscptr.o dscptr.s

$(OBJ)/interrupt.o:
	nasm $(ASFLAGS) -o $(OBJ)/interrupt.o interrupt.s

clean:
	$(RM) $(TARGETOBJ)
