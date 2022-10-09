# Makefile for SDK module
MATRIX_ROOT_DIR := $(abspath $(CURDIR)/..)

CFLAGS := -Wall -nostdlib -nostdinc -fno-builtin -fno-stack-protector

ASFLAGS := -felf

SDKDIR := $(MATRIX_ROOT_DIR)/bin/sdk

OBJS :=	$(SDKDIR)/crt1.o \
	$(SDKDIR)/libinit.o \
	$(SDKDIR)/syscalls.o \
	$(SDKDIR)/div64.o \
	$(SDKDIR)/stdio.o \
	$(SDKDIR)/string.o \
	$(SDKDIR)/vsprintf.o \
	$(SDKDIR)/sprintf.o \
	$(SDKDIR)/printf.o \
	$(SDKDIR)/format.o \
	$(SDKDIR)/time.o

.PHONY: clean help

all:	$(OBJS)
	@:

$(SDKDIR)/%.o: %.c
	$(CC) $(CFLAGS) -m32 -Iinclude -c -o $@ $<

$(SDKDIR)/%.o: libc/%.c
	$(CC) $(CFLAGS) -DBITS_PER_LONG=32 -m32 -Iinclude -c -o $@ $<

$(SDKDIR)/crt1.o:
	nasm $(ASFLAGS) -o $(SDKDIR)/crt1.o crt1.s

clean:
	$(RM) $(SDKDIR)/*.o
