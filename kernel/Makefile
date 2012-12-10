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
MODULES := hal libc rtl loader mm sys fs proc time init ../drivers dbgk

# Define target file
TARGET := ../bin/matrix

# Define object file directory
OBJ := ../bin/obji386

# Definition of LDFLAGS
LDFLAGS := -melf_i386 -TLink.ld

.PHONY: clean help

# Add the default or global target
all: $(TARGET)
	@echo "Making default target."

$(TARGET): hal_ libc_ rtl_ loader_ dbgk_ mm_ sys_ proc_ drivers_ fs_ time_ init_
	$(LD) $(LDFLAGS) -Map ../bin/matrix.map -o $(TARGET) $(OBJ)/*.o

hal_:
	@cd hal && $(MAKE) $(MAKEFLAGS)

libc_:
	@cd libc && $(MAKE) $(MAKEFLAGS)

rtl_:
	@cd rtl && $(MAKE) $(MAKEFLAGS)

loader_:
	@cd loader && $(MAKE) $(MAKEFLAGS)

dbgk_:
	@cd dbgk && $(MAKE) $(MAKEFLAGS)

mm_:
	@cd mm && $(MAKE) $(MAKEFLAGS)

sys_:
	@cd sys && $(MAKE) $(MAKEFLAGS)

fs_:
	@cd fs && $(MAKE) $(MAKEFLAGS)

drivers_:
	@cd ../drivers && $(MAKE) $(MAKEFLAGS)

proc_:
	@cd proc && $(MAKE) $(MAKEFLAGS)

time_:
	@cd time && $(MAKE) $(MAKEFLAGS)

init_:
	@cd init && $(MAKE) $(MAKEFLAGS)

clean:
	for d in $(MODULES); do (cd $$d; $(MAKE) clean); done; $(RM) $(TARGET) bin/matrix.map

help:
	@echo "Available make targets:"
	@echo
	@echo "all      - build matrix"
	@echo "clean    - remove all object files, dependency files"
	@echo "help     - print this help info"