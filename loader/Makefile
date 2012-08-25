# Makefile for loader module
include ../Makefile.inc

OBJ := ../bin/obji386

TARGETOBJ := $(OBJ)/syscalls.o

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -m32 -g -I../include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)