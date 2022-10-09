# Makefile for matrix kernel. 
# This is the top level makefile of the kernel. You need to modified it
#  when you added a new module.
MATRIX_ROOT_DIR := $(abspath $(CURDIR)/..)
include $(MATRIX_ROOT_DIR)/kernel/Makefile.inc

VERSION = 0
PATCHLEVEL = 0
SUBLEVEL = 1
EXTRAVERSION = 1

DIRS-y = hal libc rtl sys mm fs loader proc time init dbgk ../drivers

TARGET := $(MATRIX_ROOT_DIR)/bin/matrix
MAP := $(MATRIX_ROOT_DIR)/bin/matrix.map

MATRIX_LIB_LIST = init hal proc dbgk time sys rtl mm loader cstd drivers fs
LIBS += $(MATRIX_LIB_LINKER_ARGS)

# *DOCUMENTATION*
# To see a list of typical targets execute "make help"
# More info can be found in ../README

# The following is a list of non-source files that are part of the
# distribution.
AUXFILES := Makefile

# Definition of LDFLAGS
LDFLAGS := -melf_i386 -TLink.ld

.PHONY: all clean $(DIRS-y) help

all: $(TARGET)
	@:

empty_rule:
	@:

$(TARGET): $(DIRS-y)
	$(LD) $(LDFLAGS) -Map $(MAP) -o $(TARGET) $(OBJS) $(LIBS)

clean: $(DIRS-y)
	$(CLEAN_C) $(TARGET) $(MAP)

$(DIRS-y):
	$(Q)$(MAKE) -C $@ S=$S$(S:%=/)$@ $(MAKECMDGOALS) $(MAKESUBDIRFLAGS)

help:
	@echo "Available make targets:"
	@echo
	@echo "all      - build matrix"
	@echo "clean    - remove all object files, dependency files"
	@echo "help     - print this help info"

