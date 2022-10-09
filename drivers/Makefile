# Make file for drivers module
MATRIX_ROOT_DIR := $(abspath $(CURDIR)/..)
include $(MATRIX_ROOT_DIR)/kernel/Makefile.inc

C_SRCS = keyboard.c floppy.c cmos.c pci.c null.c zero.c
LIB := $(call matrix_lib_list_to_static_libs,drivers)

.PHONY: clean help

all: $(LIB)
	@:

clean:
	$(CLEAN_C) $(LIB)

$(LIB): $(OBJS)
	$(LIB_C)
