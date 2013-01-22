# Makefile for ddk module

CFLAGS := -Wall -nostdlib -nostdinc -fno-builtin -fno-stack-protector

ASFLAGS := -felf

OBJ := ../bin/ddk

TARGETOBJ := \
	$(OBJ)/device.o \

.PHONY: clean help

all:	$(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) $(CFLAGS) -m32 -Iinclude -I../sdk/include -I../kernel/include -c -o $@ $<

clean:
	$(RM) $(OBJ)/*.o