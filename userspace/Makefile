# Makefile for userspace utilities

CFLAGS := -Wall -nostdlib -nostdinc -fno-builtin -fno-stack-protector

# The linker script will link crtlib in
LDFLAGS := -melf_i386 -TLink.ld

TARGETDIR := ../bin

TARGETS := \
	../bin/init \
	../bin/echo \

OBJ := ./obji386

.PHONY: clean help

all: $(TARGETS)
	@echo "Making default applications"

$(TARGETDIR)/init: $(OBJ)/init.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/init.map -o $(TARGETDIR)/init $(OBJ)/init.o

$(TARGETDIR)/echo: $(OBJ)/echo.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/echo.map -o $(TARGETDIR)/echo $(OBJ)/echo.o

$(OBJ)/%.o: %.c
	$(CC) $(CFLAGS) -m32 -g -I../sdk/include -c -o $@ $<

clean:
	$(RM) -rf $(TARGETS) $(TARGETS).map

help:
	@echo "Available make targets:"
	@echo 
	@echo "help	- print this help"