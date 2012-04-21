# Makefile for init module

include ../Makefile.inc

OBJ := ../bin/obji386

TARGETOBJ := $(OBJ)/entry.o \
	$(OBJ)/main.o

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -m32 -g $(CFLAGS_global) -I../include -c -o $@ $<

$(OBJ)/entry.o:
	nasm $(ASFLAGS) -o $(OBJ)/entry.o entry.s

clean:
	$(RM) $(TARGETOBJ)
