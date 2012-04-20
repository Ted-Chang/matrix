# Makefile for matrix kernel. 
# This is the top level make file. You need to modified it when
# you added a new module.

VERSION = 0
PATCHLEVEL = 0
SUBLEVEL = 1
EXTRAVERSION = 

# *DOCUMENTATION*
# To see a list of typical targets execute "make help"
# More info can be found in ./README

# Include some global definition
include Makefile.inc

# The following is a list of non-source files that are part of the distribution.
AUXFILES := Makefile README

# Define modules in the project.
MODULES := hal libc mm fs kernel init drivers

# Define target file
TARGET := bin/matrix

# Define object file directory
OBJ := bin/obji386

# Definition of LDFLAGS
LDFLAGS := -melf_i386 -TLink.ld

.PHONY: clean help

# Add the default or global target
all: $(TARGET)
	@echo "Making default target."

$(TARGET): hal_module libc_module mm_module fs_module kernel_module drivers_module init_module tests_module
	$(LD) $(LDFLAGS) -Map bin/matrix.map -o $(TARGET) $(OBJ)/*.o

hal_module:
	@cd hal && $(MAKE) $(MAKEFLAGS)

libc_module:
	@cd libc && $(MAKE) $(MAKEFLAGS)

mm_module:
	@cd mm && $(MAKE) $(MAKEFLAGS)

fs_module:
	@cd fs && $(MAKE) $(MAKEFLAGS)

kernel_module:
	@cd kernel && $(MAKE) $(MAKEFLAGS)

drivers_module:
	@cd drivers && $(MAKE) $(MAKEFLAGS)

init_module:
	@cd init && $(MAKE) $(MAKEFLAGS)

tests_module:
	@cd tests && $(MAKE) $(MAKEFLAGS)

clean:
	for d in $(MODULES); do (cd $$d; $(MAKE) clean); done; $(RM) $(TARGET) bin/matrix.map

help:
	@echo "Available make targets:"
	@echo
	@echo "all      - build matrix"
	@echo "clean    - remove all object files, dependency files"
	@echo "help     - print this help info"