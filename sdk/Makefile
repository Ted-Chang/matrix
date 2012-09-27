# Makefile for SDK module

CFLAGS := -Wall -nostdlib -nostdinc -fno-builtin -fno-stack-protector

ASFLAGS := -felf

OBJ := ../bin/lib

TARGETOBJ := \
	$(OBJ)/crtlib.o \
	$(OBJ)/syscalls.o \

.PHONY: clean help

all:	$(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) $(CFLAGS) -m32 -Iinclude -c -o $@ $<

$(OBJ)/crtlib.o:
	nasm $(ASFLAGS) -o $(OBJ)/crtlib.o crtlib.s

clean:
	$(RM) $(TARGETOBJ)