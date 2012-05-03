# Makefile for mm module

include ../Makefile.inc

OBJ := ../bin/obji386

TARGETOBJ := $(OBJ)/vector.o \
	$(OBJ)/fixed_heap.o \
	$(OBJ)/mmu.o \
	$(OBJ)/page.o \
	$(OBJ)/phys.o \
	$(OBJ)/kmem.o \
	$(OBJ)/malloc.o \
	$(OBJ)/vaspace.o \


.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -m32 -D_X86_ -g -I../include $(CFLAGS_global) -c -o $@ $<

$(OBJ)/phys.o:
	nasm $(ASFLAGS) -o $(OBJ)/phys.o phys.s

clean:
	$(RM) $(TARGETOBJ)
