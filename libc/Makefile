# Makefile for libc module
include ../Makefile.inc

OBJ := ../bin/obji386

TARGETOBJ := $(OBJ)/stdio.o $(OBJ)/string.o $(OBJ)/vsprintf.o $(OBJ)/div64.o

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -m32 -DBITS_PER_LONG=32 -I../include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)
