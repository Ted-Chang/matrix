# Makefile for fs module
include ../Makefile.inc

OBJ := ../../bin/obji386

TARGETOBJ := $(OBJ)/vfs.o

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D__KERNEL__ -m32 $(CFLAGS_global) -g -I../include -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)