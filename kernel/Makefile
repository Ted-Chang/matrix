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


.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D_X86_ -g -m32 -I../include $(CFLAGS_global) -c -o $@ $<

$(OBJ)/process.o:
	nasm $(ASFLAGS) -o $(OBJ)/process.o process.s

clean:
	$(RM) $(TARGETOBJ)
