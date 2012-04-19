# Makefile for kernel module

include ../Makefile.inc

OBJ := ../bin/obji386

TARGETOBJ := $(OBJ)/debug.o \
	$(OBJ)/exceptn.o \
	$(OBJ)/process.o \
	$(OBJ)/task.o \
	$(OBJ)/timer.o \
	$(OBJ)/initrd.o \
	$(OBJ)/syscall.o \
	$(OBJ)/clock.o \
	$(OBJ)/sched.o \
	$(OBJ)/system.o \
	$(OBJ)/kd.o \
	$(OBJ)/cpu.o \
	$(OBJ)/util.o \


.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D_DEBUG_SCHED -D_X86_ -m32 -I../include $(CFLAGS_global) -c -o $@ $<

$(OBJ)/process.o:
	nasm $(ASFLAGS) -o $(OBJ)/process.o process.s

clean:
	$(RM) $(TARGETOBJ)
