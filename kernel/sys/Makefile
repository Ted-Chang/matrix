# Makefile for sys module

include ../Makefile.inc

OBJ := ../../bin/obji386

TARGETOBJ := \
	$(OBJ)/timer.o \
	$(OBJ)/syscall.o \
	$(OBJ)/util.o \
	$(OBJ)/mutex.o \
	$(OBJ)/semaphore.o \
	$(OBJ)/terminal.o \
	$(OBJ)/unittest.o \
	$(OBJ)/platform.o \
	$(OBJ)/ioctx.o \
	$(OBJ)/device.o \
	$(OBJ)/acpi.o \
	$(OBJ)/smp.o \

.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -DCONFIG_SERIAL_PORT=2 -DBITS_PER_LONG=32 -m32 -I../include -I../../sdk/include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)
