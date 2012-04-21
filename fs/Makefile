# Makefile for fs module

include ../Makefile.inc

OBJ := ../bin/obji386

TARGETOBJ := $(OBJ)/fs.o

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D_X86_ -m32 -g -I../include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)