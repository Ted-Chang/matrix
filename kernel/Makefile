# Makefile for kernel module

include ../Makefile.inc

OBJ := ../bin/obji386

TARGETOBJ := $(OBJ)/debug.o \
	$(OBJ)/exceptn.o \
	$(OBJ)/process.o \
	$(OBJ)/misc.o \
	$(OBJ)/timer.o \
	$(OBJ)/initrd.o \
	$(OBJ)/syscall.o \
	$(OBJ)/clock.o \
	$(OBJ)/sched.o \
	$(OBJ)/system.o \
	$(OBJ)/util.o \


.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D_DEBUG_SCHED -DBITS_PER_LONG=32 -m32 -I../include $(CFLAGS_global) -c -o $@ $<

$(OBJ)/misc.o:
	nasm $(ASFLAGS) -o $(OBJ)/misc.o misc.s

clean:
	$(RM) $(TARGETOBJ)
