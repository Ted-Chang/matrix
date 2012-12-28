# Make file for drivers module

include ../kernel/Makefile.inc

OBJ := ../bin/obji386

TARGETOBJ := \
	$(OBJ)/keyboard.o \
	$(OBJ)/floppy.o \
	$(OBJ)/cmos.o \
	$(OBJ)/pci.o \

.PHONY: clean

all: $(TARGETOBJ)

$(OBJ)/%.o: %.c
	$(CC) -D__KERNEL__ -m32 -I../kernel/include -I../sdk/include $(CFLAGS_global) -c -o $@ $<

clean:
	$(RM) $(TARGETOBJ)