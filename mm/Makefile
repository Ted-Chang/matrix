# Makefile for mm module

include ../Makefile.inc

OBJ := ../bin/obji386

TARGETOBJ := $(OBJ)/vector.o \
	$(OBJ)/mmgr.o \
	$(OBJ)/kheap.o \
	$(OBJ)/fixed_heap.o \


.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -m32 -D_X86_ -I../include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)
