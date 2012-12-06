# Makefile for tests module
CFLAGS := -Wall -nostdlib -nostdinc -fno-builtin -fno-stack-protector

OBJ := ../bin/obji386

TARGETOBJ := \
	$(OBJ)/print_test.o \
	$(OBJ)/list_test.o \
	$(OBJ)/lock_test.o \
	$(OBJ)/mem_test.o \
	$(OBJ)/avltree_test.o \
	$(OBJ)/tmr_test.o \
	$(OBJ)/tests.o \


.PHONY: clean help

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) $(CFLAGS) -m32 -D_X86_ -g -I../sdk/inc -I../kernel/inc -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)