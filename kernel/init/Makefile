# Makefile for init module

include ../Makefile.inc

OBJ := ../../bin/obji386

TARGETOBJ := $(OBJ)/entry.o \
	$(OBJ)/main.o

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D_UNIT_TEST -D__KERNEL__ -m32 $(CFLAGS_global) -I../inc -I../../sdk/inc -c -o $@ $<

$(OBJ)/entry.o:
	nasm $(ASFLAGS) -o $(OBJ)/entry.o entry.s

clean:
	$(RM) $(TARGETOBJ)
