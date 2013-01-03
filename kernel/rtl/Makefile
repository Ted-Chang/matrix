# Makefile for librt module

include ../Makefile.inc

OBJ := ../../bin/obji386

TARGETOBJ := \
	$(OBJ)/avltree.o \
	$(OBJ)/notifier.o \
	$(OBJ)/object.o \
	$(OBJ)/bitmap.o \
	$(OBJ)/name.o \

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D__KERNEL__ -D_X86_ -DBITS_PER_LONG=32 -m32 -I../include -I../../sdk/include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)
