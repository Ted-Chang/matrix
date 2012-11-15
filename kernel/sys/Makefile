# Makefile for sys module

include ../Makefile.inc

OBJ := ../../bin/obji386

TARGETOBJ := \
	$(OBJ)/exceptn.o \
	$(OBJ)/timer.o \
	$(OBJ)/syscall.o \
	$(OBJ)/util.o \
	$(OBJ)/fd.o \
	$(OBJ)/notifier.o \
	$(OBJ)/mutex.o \
	$(OBJ)/semaphore.o \
	$(OBJ)/elf.o \
	$(OBJ)/object.o \
	$(OBJ)/avltree.o \

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D__KERNEL__ -D_X86_ -DBITS_PER_LONG=32 -m32 -I../include -I../../sdk/include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)
