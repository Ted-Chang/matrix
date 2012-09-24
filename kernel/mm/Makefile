# Makefile for mm module

include ../Makefile.inc

OBJ := ../../bin/obji386

TARGETOBJ := $(OBJ)/vector.o \
	$(OBJ)/mmu.o \
	$(OBJ)/kmem.o \
	$(OBJ)/page.o \
	$(OBJ)/malloc.o \
	$(OBJ)/slab.o \


.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -m32 -D_X86_ -D__KERNEL__ -I../include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)
