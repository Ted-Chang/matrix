# Makefile for librt module

include ../Makefile.inc

OBJ := ../../bin/obji386

TARGETOBJ := \
	$(OBJ)/elf.o \
	$(OBJ)/module.o \

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -DBITS_PER_LONG=32 -m32 -I../include -I../../sdk/include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)
