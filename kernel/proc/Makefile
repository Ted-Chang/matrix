# Makefile for proc module

include ../Makefile.inc

OBJ := ../../bin/obji386

TARGETOBJ := \
	$(OBJ)/process.o \
	$(OBJ)/misc.o \
	$(OBJ)/sched.o \
	$(OBJ)/switch.o \
	$(OBJ)/avltree.o \
	$(OBJ)/thread.o \

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D_DEBUG_SCHED -D__KERNEL__ -D_X86_ -DBITS_PER_LONG=32 -m32 -I../include -I../../sdk/include $(CFLAGS_global) -c -o $@ $<

$(OBJ)/misc.o:
	nasm $(ASFLAGS) -o $(OBJ)/switch.o switch.s

clean:
	$(RM) $(TARGETOBJ)
