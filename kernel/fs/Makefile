# Makefile for fs module
include ../Makefile.inc

OBJ := ../../bin/obji386

TARGETOBJ := \
	$(OBJ)/vfs.o \
	$(OBJ)/fd.o \
	$(OBJ)/initrd.o \
	$(OBJ)/devfs.o \
	$(OBJ)/procfs.o \

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -m32 $(CFLAGS_global) -g -I../include -I../../sdk/include -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)