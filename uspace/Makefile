# Makefile for userspace utilities

CFLAGS := -Wall -nostdlib -nostdinc -fno-builtin -fno-stack-protector

# The linker script will link crtlib in
LDFLAGS := -melf_i386 -TLink.ld

TARGETDIR := ../bin

TARGETS := \
	$(TARGETDIR)/init \
	$(TARGETDIR)/echo \
	$(TARGETDIR)/crond \
	$(TARGETDIR)/ls \
	$(TARGETDIR)/cat \
	$(TARGETDIR)/clear \
	$(TARGETDIR)/shutdown \
	$(TARGETDIR)/mkdir \
	$(TARGETDIR)/date \
	$(TARGETDIR)/mount \
	$(TARGETDIR)/umount \
	$(TARGETDIR)/dd \
	$(TARGETDIR)/mknod \
	$(TARGETDIR)/lsmod \
	$(TARGETDIR)/sh \
	$(TARGETDIR)/unit_test \
	$(TARGETDIR)/process_test \

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

$(TARGETDIR)/clear: $(OBJ)/clear.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/clear.map -o $(TARGETDIR)/clear $(OBJ)/clear.o

$(TARGETDIR)/shutdown: $(OBJ)/shutdown.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/shutdown.map -o $(TARGETDIR)/shutdown $(OBJ)/shutdown.o

$(TARGETDIR)/mkdir: $(OBJ)/mkdir.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/mkdir.map -o $(TARGETDIR)/mkdir $(OBJ)/mkdir.o

$(TARGETDIR)/date: $(OBJ)/date.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/date.map -o $(TARGETDIR)/date $(OBJ)/date.o

$(TARGETDIR)/mount: $(OBJ)/mount.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/mount.map -o $(TARGETDIR)/mount $(OBJ)/mount.o

$(TARGETDIR)/umount: $(OBJ)/umount.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/umount.map -o $(TARGETDIR)/umount $(OBJ)/umount.o

$(TARGETDIR)/dd: $(OBJ)/dd.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/dd.map -o $(TARGETDIR)/dd $(OBJ)/dd.o

$(TARGETDIR)/mknod: $(OBJ)/mknod.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/mknod.map -o $(TARGETDIR)/mknod $(OBJ)/mknod.o

$(TARGETDIR)/lsmod: $(OBJ)/lsmod.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/lsmod.map -o $(TARGETDIR)/lsmod $(OBJ)/lsmod.o

$(TARGETDIR)/sh: $(OBJ)/sh.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/sh.map -o $(TARGETDIR)/sh $(OBJ)/sh.o

$(TARGETDIR)/process_test: $(OBJ)/process_test.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/process_test.map -o $(TARGETDIR)/process_test $(OBJ)/process_test.o

$(OBJ)/%.o: %.c
	$(CC) $(CFLAGS) -m32 -g -I../sdk/include -c -o $@ $<

clean:
	for f in $(TARGETS); do rm $$f.map; done;
	$(RM) -f $(TARGETS)
	$(RM) -f $(OBJ)/*

help:
	@echo "Available make targets:"
	@echo 
	@echo "all	- build all userspace binaries"
	@echo "clean	- remove all object files and binaries"
	@echo "help	- print this help"
