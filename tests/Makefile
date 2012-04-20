# Makefile for tests module
include ../Makefile.inc

OBJ := ../bin/obji386

TARGETOBJ := $(OBJ)/print_test.o $(OBJ)/list_test.o

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -m32 -D_X86_ -I../include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)