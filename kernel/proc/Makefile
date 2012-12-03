
# Makefile for proc module

include ../Makefile.inc

OBJ := ../../bin/obji386

TARGETOBJ := \
	$(OBJ)/process.o \
	$(OBJ)/sched.o \
	$(OBJ)/switch.o \
	$(OBJ)/thread.o \
	$(OBJ)/signal.o \

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D__KERNEL__ -D_X86_ -DBITS_PER_LONG=32 -m32 -I../include -I../../sdk/include $(CFLAGS_global) -c -o $@ $<

$(OBJ)/switch.o:
	nasm $(ASFLAGS) -o $(OBJ)/switch.o switch.s

clean:
	$(RM) $(TARGETOBJ)
