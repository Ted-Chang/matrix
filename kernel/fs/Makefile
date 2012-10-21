# Makefile for fs module
include ../Makefile.inc

OBJ := ../../bin/obji386

TARGETOBJ := \
	$(OBJ)/vfs.o \
	$(OBJ)/initrd.o \

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D__KERNEL__ -m32 $(CFLAGS_global) -g -I../include -I../../sdk/include -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)