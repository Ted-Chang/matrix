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

$(TARGETDIR)/init: init.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/init.map -o $(TARGETDIR)/init init.o

$(TARGETDIR)/echo: echo.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/echo.map -o $(TARGETDIR)/echo echo.o

$(TARGETDIR)/crond: crond.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/crond.map -o $(TARGETDIR)/crond crond.o

$(TARGETDIR)/unit_test: unit_test.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/unit_test.map -o $(TARGETDIR)/unit_test unit_test.o

$(TARGETDIR)/ls: ls.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/ls.map -o $(TARGETDIR)/ls ls.o

$(TARGETDIR)/cat: cat.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/cat.map -o $(TARGETDIR)/cat cat.o

$(TARGETDIR)/clear: clear.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/clear.map -o $(TARGETDIR)/clear clear.o

$(TARGETDIR)/shutdown: shutdown.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/shutdown.map -o $(TARGETDIR)/shutdown shutdown.o

$(TARGETDIR)/mkdir: mkdir.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/mkdir.map -o $(TARGETDIR)/mkdir mkdir.o

$(TARGETDIR)/date: date.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/date.map -o $(TARGETDIR)/date date.o

$(TARGETDIR)/mount: mount.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/mount.map -o $(TARGETDIR)/mount mount.o

$(TARGETDIR)/umount: umount.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/umount.map -o $(TARGETDIR)/umount umount.o

$(TARGETDIR)/dd: dd.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/dd.map -o $(TARGETDIR)/dd dd.o

$(TARGETDIR)/mknod: mknod.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/mknod.map -o $(TARGETDIR)/mknod mknod.o

$(TARGETDIR)/lsmod: lsmod.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/lsmod.map -o $(TARGETDIR)/lsmod lsmod.o

$(TARGETDIR)/sh: sh.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/sh.map -o $(TARGETDIR)/sh sh.o

$(TARGETDIR)/process_test: process_test.o
	$(LD) $(LDFLAGS) -Map $(TARGETDIR)/process_test.map -o $(TARGETDIR)/process_test process_test.o

%.o: %.c
	$(CC) $(CFLAGS) -m32 -g -I../sdk/include -c -o $@ $<

clean:
	for f in $(TARGETS); do $(RM) $$f.map; done;
	$(RM) $(TARGETS)
	$(RM) *.o

help:
	@echo "Available make targets:"
	@echo 
	@echo "all	- build all userspace binaries"
	@echo "clean	- remove all object files and binaries"
	@echo "help	- print this help"
