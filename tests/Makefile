# Makefile for tests module
include ../Makefile.inc

OBJ := ../bin/obji386

TARGETOBJ := $(OBJ)/print_test.o \
	$(OBJ)/list_test.o \
	$(OBJ)/lock_test.o \
	$(OBJ)/mem_test.o \
	$(OBJ)/tests.o \


.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -m32 -D_X86_ -g -I../include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)