# Make file for drivers module

include ../Makefile.inc

OBJ := ../bin/obji386

TARGETOBJ := $(OBJ)/keyboard.o $(OBJ)/floppy.o $(OBJ)/cmos.o

.PHONY: clean

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -m32 -g -I../include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)