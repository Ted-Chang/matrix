# Makefile for libc module
include ../Makefile.inc

OBJ := ../bin/obji386

TARGETOBJ := $(OBJ)/stdio.o $(OBJ)/string.o $(OBJ)/util.o

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -m32 -I../include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)
