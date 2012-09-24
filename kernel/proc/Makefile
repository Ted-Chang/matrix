# Makefile for proc module

include ../Makefile.inc

OBJ := ../../bin/obji386

TARGETOBJ := \
	$(OBJ)/process.o \
	$(OBJ)/misc.o \
	$(OBJ)/timer.o \
	$(OBJ)/clock.o \
	$(OBJ)/sched.o \
	$(OBJ)/fd.o \
	$(OBJ)/elf.o \
	$(OBJ)/misc.o \

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D_DEBUG_SCHED -D__KERNEL__ -D_X86_ -DBITS_PER_LONG=32 -m32 -I../include $(CFLAGS_global) -c -o $@ $<

$(OBJ)/misc.o:
	nasm $(ASFLAGS) -o $(OBJ)/misc.o misc.s

clean:
	$(RM) $(TARGETOBJ)
