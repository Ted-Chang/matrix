# Makefile for sys module

include ../Makefile.inc

OBJ := ../../bin/obji386

TARGETOBJ := \
	$(OBJ)/exceptn.o \
	$(OBJ)/timer.o \
	$(OBJ)/initrd.o \
	$(OBJ)/syscall.o \
	$(OBJ)/system.o \
	$(OBJ)/util.o \

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D_DEBUG_SCHED -D__KERNEL__ -D_X86_ -DBITS_PER_LONG=32 -m32 -I../include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)
