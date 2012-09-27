# Makefile for userspace utilities

CFLAGS := -Wall -nostdlib -nostdinc -fno-builtin -fno-stack-protector

# The linker script will link crtlib in
LDFLAGS := -melf_i386 -TLink.ld

TARGETS := \
	../bin/init

OBJ := ./obji386

.PHONY: clean help

all: $(TARGETS)
	@echo "Making default applications"

$(TARGETS): $(OBJ)/init.o
	$(LD) $(LDFLAGS) -Map ../bin/$(TARGETS).map -o $(TARGETS) $(OBJ)/init.o

$(OBJ)/%.o: %.c
	$(CC) $(CFLAGS) -m32 -g -I../sdk/include -c -o $@ $<

clean:
	$(RM) $(TARGETS)

help:
	@echo "Available make targets:"
	@echo 
	@echo "help	- print this help"