# Makefile for kernel module

include ../Makefile.inc

OBJ := ../bin/obji386

TARGETOBJ := $(OBJ)/debug.o \
	$(OBJ)/exceptn.o \
	$(OBJ)/initrd.o \
	$(OBJ)/kd.o \
	$(OBJ)/cpu.o \
	$(OBJ)/util.o \
	$(OBJ)/mutex.o \
	$(OBJ)/process.o \
	$(OBJ)/clock.o \
	$(OBJ)/rtc.o \
	$(OBJ)/timer.o \
	$(OBJ)/sched.o \
	$(OBJ)/thread.o \


.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -DBITS_PER_LONG=32 -D_X86_ -g -m32 -I../include $(CFLAGS_global) -c -o $@ $<

#$(OBJ)/process.o:
#	nasm $(ASFLAGS) -o $(OBJ)/process.o process.s

clean:
	$(RM) $(TARGETOBJ)
