# Makefile for matrix kernel. 
# This is the top level makefile of the kernel. You need to modified it when
# you added a new module.

VERSION = 0
PATCHLEVEL = 0
SUBLEVEL = 1
EXTRAVERSION = 1

# *DOCUMENTATION*
# To see a list of typical targets execute "make help"
# More info can be found in ../README

# Include some global definition
include Makefile.inc

# The following is a list of non-source files that are part of the distribution.
AUXFILES := Makefile

# Define modules in the project.
MODULES := hal libc rtl loader mm sys fs proc time init ../drivers dbgk

# Define target file
TARGET := ../bin/matrix

# Define object file directory
OBJ := ../bin/obji386

# Definition of LDFLAGS
LDFLAGS := -melf_i386 -TLink.ld

.PHONY: all clean help

all: $(MODULES)
	$(LD) $(LDFLAGS) -Map ../bin/matrix.map -o $(TARGET) $(OBJ)/*.o

$(MODULES): Makefile
	cd $@ && $(MAKE) $(MFLAGS)

clean:
	for d in $(MODULES); do (cd $$d; $(MAKE) clean); done; $(RM) $(TARGET) ../bin/matrix.map

help:
	@echo "Available make targets:"
	@echo
	@echo "all      - build matrix"
	@echo "clean    - remove all object files, dependency files"
	@echo "help     - print this help info"
