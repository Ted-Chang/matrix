# Makefile for sys module

include ../Makefile.inc

OBJ := ../../bin/obji386

TARGETOBJ := \
	$(OBJ)/rtc.o \
	$(OBJ)/platform.o \
	$(OBJ)/pit.o \

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D__KERNEL__ -D_X86_ -DBITS_PER_LONG=32 -m32 -I../inc -I../../sdk/inc $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)
