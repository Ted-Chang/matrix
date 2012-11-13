# Makefile for debug module

include ../Makefile.inc

OBJ := ../../bin/obji386

TARGETOBJ := $(OBJ)/debug.o \
	$(OBJ)/kd.o \
	$(OBJ)/dbgheap.o \
	$(OBJ)/symbol.o \

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D_DEBUG_SCHED -D__KERNEL__ -D_X86_ -DBITS_PER_LONG=32 -m32 -I../include -I../../sdk/include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)
