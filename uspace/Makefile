# Makefile for userspace utilities

CFLAGS := -Wall -nostdlib -nostdinc -fno-builtin -fno-stack-protector

# The linker script will link crtlib in
LDFLAGS := -melf_i386 -TLink.ld

TARGETDIR := ../bin

TARGETS := \
	$(TARGETDIR)/init \
	$(TARGETDIR)/echo \
	$(TARGETDIR)/crond \
	$(TARGETDIR)/unit_test \
	$(TARGETDIR)/ls \
	$(TARGETDIR)/cat \

OBJ := ./obji386

.PHONY: clean help

all: $(TARGETS)
	@echo "Making default applications"

$(TARGETDIR)/init: $(OBJ)/init.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/init.map -o $(TARGETDIR)/init $(OBJ)/init.o

$(TARGETDIR)/echo: $(OBJ)/echo.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/echo.map -o $(TARGETDIR)/echo $(OBJ)/echo.o

$(TARGETDIR)/crond: $(OBJ)/crond.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/crond.map -o $(TARGETDIR)/crond $(OBJ)/crond.o

$(TARGETDIR)/unit_test: $(OBJ)/unit_test.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/unit_test.map -o $(TARGETDIR)/unit_test $(OBJ)/unit_test.o

$(TARGETDIR)/ls: $(OBJ)/ls.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/ls.map -o $(TARGETDIR)/ls $(OBJ)/ls.o

$(TARGETDIR)/cat: $(OBJ)/cat.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/cat.map -o $(TARGETDIR)/cat $(OBJ)/cat.o

$(OBJ)/%.o: %.c
	$(CC) $(CFLAGS) -m32 -g -I../sdk/include -c -o $@ $<

clean:
	$(RM) -f $(TARGETS)
	$(RM) -f $(OBJ)/*

help:
	@echo "Available make targets:"
	@echo 
	@echo "help	- print this help"