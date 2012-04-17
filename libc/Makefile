# Makefile for libc module
include ../Makefile.inc

OBJ := ../bin/obji386

TARGETOBJ := $(OBJ)/stdio.o $(OBJ)/string.o  $(OBJ)/div64.o $(OBJ)/vsprintf.o $(OBJ)/util.o

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -m32 -DBITS_PER_LONG=32 -D_X86_ -I../include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)
